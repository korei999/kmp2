#include "app.hh"
#include "input.hh"
#include "play.hh"
#include "utils.hh"

#include <cmath>

namespace app
{

static void
ioChangedCB([[maybe_unused]] void* data,
            [[maybe_unused]] uint32_t id,
            [[maybe_unused]] void* area,
            [[maybe_unused]] uint32_t size)
{
    //
}

static const pw_stream_events streamEvents {
    .version = PW_VERSION_STREAM_EVENTS,
    .io_changed = ioChangedCB,
    .process = play::onProcessCB /* attached playback function */
};


std::mutex PipeWireData::mtx;
f32 PipeWirePlayer::chunk[16384] {};

Curses::Curses()
{
    initscr();
    start_color();
    use_default_colors();
    curs_set(0);
    noecho();
    cbreak();
    timeout(1000);
    keypad(stdscr, true);
    refresh();

    int td = (Curses::termdef);
    init_pair(Curses::green, COLOR_GREEN, td);
    init_pair(Curses::yellow, COLOR_YELLOW, td);
    init_pair(Curses::blue, COLOR_BLUE, td);
    init_pair(Curses::cyan, COLOR_CYAN, td);
    init_pair(Curses::red, COLOR_RED, td);

    pStd = stdscr;
}

void
Curses::drawUI()
{
    /* ncurses is not thread safe */
    std::lock_guard lock(mtx);

    int maxy = getmaxy(pStd), maxx = getmaxx(pStd);

    if (maxy >= 5 && maxx >= 5)
    {
        if (update.time)        { update.time     = false; drawTime(); }
        if (update.volume)      { update.volume   = false; drawVolume(); }
        if (update.songName)    { update.songName = false; drawSongName(); drawSongCounter(); }
        if (update.playList)    { update.playList = false; drawPlaylist(); }

        refresh();
    }
}

void
Curses::drawTime()
{
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
    limitStringToMaxX(&timeStr);

    move(0, 0);
    clrtoeol();
    addstr(timeStr.data());
}

void 
Curses::drawVolume()
{
    auto volumeStr = std::format("volume: {:.2f}\n", p->volume);
    limitStringToMaxX(&volumeStr);

    move(2, 0);
    attron(A_BOLD | COLOR_PAIR(green));
    addstr(volumeStr.data());
    attroff(A_BOLD | COLOR_PAIR(green));
}

void
Curses::drawSongCounter()
{
    auto songCounterStr = std::format("{} / {}", p->currSongIdx + 1, p->songs.size());
    if (p->repeatAfterLast) { songCounterStr += " (Repeat After Last)" ; }
    limitStringToMaxX(&songCounterStr);

    move(4, 0);
    clrtoeol();
    attron(A_ITALIC);
    addstr(songCounterStr.data());
    attroff(A_ITALIC);
}

void
Curses::drawSongName()
{
    auto songNameStr = std::format("{}", p->currSongName());
    songNameStr = "playing: " + utils::removePath(songNameStr);
    limitStringToMaxX(&songNameStr);

    move(5, 0);
    clrtoeol();
    attron(A_BOLD | A_ITALIC | COLOR_PAIR(yellow));
    addstr(songNameStr.data());
    attroff(A_BOLD | A_ITALIC | COLOR_PAIR(yellow));
}

void
Curses::drawPlaylist()
{
    long maxy = getmaxy(pStd);

    long cursesY = listYPos;
    long sel = p->term.selected;

    /* dirty fixes for out of range or incomplete draws at the end of the list */
    if ((p->songs.size() - 1) - firstInList < maxListSize())
        firstInList = (p->songs.size() - 1) - (maxListSize() - 1);
    if (p->songs.size() < maxListSize())
        firstInList = 0;
    if (firstInList < 0)
        firstInList = 0;

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

    for (long i = firstInList; i < (long)p->songs.size() && cursesY < maxy; i++, cursesY++)
    {
        auto lineStr = utils::removePath(std::format("{}", p->songs[i].data()));
        limitStringToMaxX(&lineStr);

        if (i == sel)
            attron(A_REVERSE);
        if (i == p->currSongIdx)
            attron(A_BOLD | COLOR_PAIR(app::Curses::yellow));

        move(cursesY, 0);
        clrtoeol();
        addstr(lineStr.data());

        attroff(A_REVERSE | A_BOLD | COLOR_PAIR(app::Curses::yellow));
    }
}

void
Curses::updateAll()
{
    update.ui = update.time = update.volume = update.songName = update.playList = true;
}

PipeWirePlayer::PipeWirePlayer(int argc, char** argv)
{
    term.p = this;

    pw_init(&argc, &argv);

    term.firstInList = 0;

    for (int i = 1; i < argc; i++)
    {
        std::string s = argv[i];

        if (s.ends_with(".wav")   ||
            s.ends_with(".opus")  ||
            s.ends_with(".ogg")   ||
            s.ends_with(".mp3")   ||
            s.ends_with(".flac"))
        {
            songs.push_back(std::move(s));
        }
    }

    if (songs.empty())
    {
        finished = true;
        return;
    }

    std::thread inputThread(input::read, this);
    inputThread.detach();
}

PipeWirePlayer::~PipeWirePlayer()
{
    pw_deinit();

    if (songs.empty())
        COUT("kmp: no input provided\n");
}

void
PipeWirePlayer::setupPlayer(enum spa_audio_format format, u32 sampleRate, u32 channels)
{
    u8* buffer[1024];
    const spa_pod* params[1] {};
    spa_pod_builder b = SPA_POD_BUILDER_INIT(buffer, sizeof(buffer));

    pw.loop = pw_main_loop_new(nullptr);

    pw.stream = pw_stream_new_simple(pw_main_loop_get_loop(pw.loop),
                                         "kmpSource",
                                         pw_properties_new(PW_KEY_MEDIA_TYPE, "Audio",
                                                           PW_KEY_MEDIA_CATEGORY, "Playback",
                                                           PW_KEY_MEDIA_ROLE, "Music",
                                                           PW_KEY_STREAM_LATENCY_MIN, "20000",
                                                           PW_KEY_STREAM_LATENCY_MAX, "40000",
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
                      params, LEN(params));
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
    hSnd = SndfileHandle(currSongName().data(), SFM_READ);

    pw.format = SPA_AUDIO_FORMAT_F32;
    pw.sampleRate = hSnd.samplerate();
    pw.channels = hSnd.channels();

    pcmPos = 0;
    pcmSize = hSnd.frames() * pw.channels;

    setupPlayer(pw.format, pw.sampleRate, pw.channels);

    term.updateAll();
    term.drawUI();
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
        if (repeatAfterLast)
            currSongIdx = 0;
        else
            finished = true;
    }
}

void
PipeWirePlayer::subStringSearch(enum search::dir direction)
{
    char buff[25] {};
    getnstr(buff, LEN(buff) - 1);
    std::string s {buff};

    if (!s.empty())
    {
        foundIndices = search::getIndexList(songs, buff, direction);
        currFoundIdx = 0;
    }
}

void
PipeWirePlayer::jumpToFound(enum search::dir direction)
{
    if (!foundIndices.empty())
    {
        int next = direction == search::dir::forward ? 1 : -1;
        currFoundIdx += next;

        if (currFoundIdx > (long)foundIndices.size() - 1)
            currFoundIdx = 0;
        else if (currFoundIdx < 0)
            currFoundIdx = foundIndices.size() - 1;

        auto newSel = foundIndices[currFoundIdx];

        if (newSel > term.firstInList + ((long)term.maxListSize() - 1))
            term.firstInList = newSel;
        else if (newSel < term.firstInList)
            term.firstInList = newSel;

        term.selected = newSel;
    }
}

} /* namespace app */
