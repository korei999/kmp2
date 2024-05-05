#include "app.hh"
#include "play.hh"
#include "utils.hh"

#include <cmath>

namespace app
{

static const pw_stream_events streamEvents {
    .version = PW_VERSION_STREAM_EVENTS,
    .process = play::onProcess, /* attached playback function */
};

static std::mutex cursesMtx;

void
Curses::updateUI()
{
    /* ncurses is not thread safe */
    std::lock_guard lock(cursesMtx);

    int maxy = getmaxy(stdscr), maxx = getmaxx(stdscr);

    if (maxy >= 5 && maxx >= 5)
    {
        if (update.time)     { update.time     = false; drawTime(); }
        if (update.volume)   { update.volume   = false; drawVolume(); }
        if (update.songName) { update.songName = false; drawSongName(); }
        if (update.playList) { update.playList = false; drawPlaylist(); }

        refresh();
    }
}

void
Curses::drawTime()
{
    int maxx = getmaxx(stdscr);

    u64 t = (p->pcmPos/sizeof(s16)) / p->pw.sampleRate;
    u64 maxT = (p->pcmSize/sizeof(s16)) / p->pw.sampleRate;

    f64 mF = t / 60.0;
    u64 m = u64(mF);
    u64 frac = 60 * (mF - m);

    f64 mFMax = maxT / 60.0;
    u64 mMax = u64(mFMax);
    u64 fracMax = 60 * (mFMax - mMax);

    auto timeStr = std::format("{}:{:02d} / {}:{:02d} min", m, frac, mMax, fracMax);
    if (p->paused) { timeStr = "(paused) " + timeStr; }
    timeStr.resize(maxx);

    move(0, 0);
    clrtoeol();
    addstr(timeStr.data());
}

void 
Curses::drawVolume()
{
    auto volumeStr = std::format("volume: {:.2f}\n", p->volume);
    move(2, 0);
    attron(A_BOLD | COLOR_PAIR(green));
    addstr(volumeStr.data());
    attroff(A_BOLD | COLOR_PAIR(green));
}

void
Curses::drawSongName()
{
    auto maxx = getmaxx(stdscr);
    auto songNameStr = std::format("{} / {}: {}", p->currSongIdx + 1, p->songs.size(), p->currSongName());
    if (p->repeatAll) { songNameStr = "(R) " + songNameStr; }
    songNameStr.resize(maxx);
    move(3, 0);
    clrtoeol();
    attron(A_BOLD | COLOR_PAIR(yellow));
    addstr(songNameStr.data());
    attroff(A_BOLD | COLOR_PAIR(yellow));
}

void
Curses::drawPlaylist()
{
    long maxy = getmaxy(stdscr), maxx = getmaxx(stdscr);

    long cursesY = listYPos;
    long sel = p->term.selected;

    long lastInList = firstInList + maxy - cursesY;

    if (goDown && selected > lastInList - 1)
    {
        goDown = false;

        if (selected < (long)p->songs.size())
            firstInList++;
    }
    else if (goUp && selected < firstInList)
    {
        goUp = false;
        firstInList--;
    }

    for (long i = firstInList; i < (long)p->songs.size(); i++, cursesY++)
    {
        if (cursesY < maxy)
        {
            auto lineStr = std::format("{}", p->songs[i]);
            size_t lastSlash = lineStr.find_last_of("/");
            lineStr = {lineStr.begin() + lastSlash + 1, lineStr.begin() + lineStr.size()};
            lineStr.resize(maxx);

            if (i == sel)
                attron(A_BOLD | A_REVERSE);
            if (i == p->currSongIdx)
                attron(COLOR_PAIR(app::Curses::yellow));

            move(cursesY, 0);
            clrtoeol();
            addstr(lineStr.data());

            attroff(A_REVERSE | A_BOLD | COLOR_PAIR(app::Curses::yellow));
        }
    }
}

void
Curses::updateAll()
{
    update.ui = update.time = update.volume = update.songName = update.playList = true;
}

PipeWirePlayer::PipeWirePlayer(int argc, char** argv)
{
    pw_init(&argc, &argv);
    term.p = this;
    term.firstInList = 0;
    term.listYPos = 5;

    for (int i = 1; i < argc; i++)
    {
        std::string s = argv[i];

        if (s.ends_with(".wav"))
            songs.push_back(std::move(s));
    }
}

PipeWirePlayer::~PipeWirePlayer()
{
    pw_deinit();
}

void
PipeWirePlayer::setupPlayer(enum spa_audio_format format, u32 sampleRate, u32 channels)
{
    u8* buffer[1024];
    const spa_pod* params[1] {};
    spa_pod_builder b = SPA_POD_BUILDER_INIT(buffer, sizeof(buffer));

    pw.loop = pw_main_loop_new(nullptr);

    pw.stream = pw_stream_new_simple(pw_main_loop_get_loop(pw.loop),
                                         "audio-src",
                                         pw_properties_new(PW_KEY_MEDIA_TYPE,
                                                           "Audio",
                                                           PW_KEY_MEDIA_CATEGORY,
                                                           "Playback",
                                                           PW_KEY_MEDIA_ROLE,
                                                           "Music",
                                                           nullptr),
                                         &streamEvents,
                                         this);


    spa_audio_info_raw info {
        .format = format,
        .rate = sampleRate,
        .channels = channels
    };

    params[0] = spa_format_audio_raw_build(&b, SPA_PARAM_EnumFormat, &info);

    pw_stream_connect(pw.stream,
                      PW_DIRECTION_OUTPUT,
                      PW_ID_ANY,
                      (enum pw_stream_flags)(PW_STREAM_FLAG_AUTOCONNECT |
                                             PW_STREAM_FLAG_MAP_BUFFERS |
                                             PW_STREAM_FLAG_ASYNC),
                      params,
                      1);
}

void
PipeWirePlayer::playAll()
{
    while (!finished)
        playCurrent();
}

void
PipeWirePlayer::playCurrent()
{
    /* TODO: read file headers somewhere here */
    auto wave = utils::loadFileToCharArray(songs[currSongIdx]);
    pcmData = (s16*)wave.data();
    pcmSize = wave.size() / sizeof(s16);
    pcmPos = 0;

    pw.format = SPA_AUDIO_FORMAT_S16;
    pw.sampleRate = app::def::sampleRate;
    pw.channels = app::def::channels;

    setupPlayer(pw.format, pw.sampleRate, pw.channels);

    term.updateAll();
    term.updateUI();
    pw_main_loop_run(pw.loop);

    /* in this order */
    pw_stream_destroy(pw.stream);
    pw_main_loop_destroy(pw.loop);

    if (newSongSelected)
    {
        newSongSelected = false;
        currSongIdx = term.selected;
    }
    else if (next)
    {
        next = false;
        currSongIdx++;
        if (currSongIdx > (long)songs.size() - 1)
            currSongIdx = 0;
    }
    else if (prev)
    {
        prev = false;
        currSongIdx--;
        if (currSongIdx < 0)
            currSongIdx = songs.size() - 1;
    }
    else
        currSongIdx++;

    if (currSongIdx > (long)songs.size() - 1)
    {
        if (repeatAll)
            currSongIdx = 0;
        else
            finished = true;
    }
}

} /* namespace app */
