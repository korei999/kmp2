#include "app.hh"
#include "play.hh"
#include "utils.hh"

namespace app
{

static const pw_stream_events streamEvents {
    .version = PW_VERSION_STREAM_EVENTS,
    .process = play::onProcess, /* attached playback function */
};

static std::mutex cursesLock;

void
Curses::updateUI()
{
    /* ncurses is not thread safe */
    std::lock_guard lock(cursesLock);

    int maxx = getmaxx(stdscr);

    drawTime();

    auto volumeStr = std::format("volume: {:.2f}\n", p->volume);
    move(2, 0);
    addstr(volumeStr.data());

    auto songNameStr = std::format("{} / {}: {}", p->currSongIdx + 1, p->songs.size(), p->currSongName());
    songNameStr.resize(maxx);
    move(3, 0);
    clrtoeol();
    addstr(songNameStr.data());

    drawPlaylist();

    refresh();
}

void
Curses::drawTime()
{
    int maxx = getmaxx(stdscr);

    size_t t = (p->pcmPos/sizeof(s16)) / p->pw.sampleRate;
    auto len = (p->pcmSize/sizeof(s16)) / p->pw.sampleRate;

    f64 mF = t / 60.0;
    size_t m = (size_t)mF;
    int frac = 60 * (mF - m);

    f64 mFMax = len / 60.0;
    size_t mMax = (size_t)mFMax;
    int fracMax = 60 * (mFMax - mMax);

    auto timeStr = std::format("{}:{:02d} / {}:{:02d} min", m, frac, mMax, fracMax);
    if (p->paused)
        timeStr = "(paused) " + timeStr;
    timeStr.resize(maxx);

    move(0, 0);
    clrtoeol();
    addstr(timeStr.data());
}

void
Curses::drawPlaylist()
{
}

PipeWirePlayer::PipeWirePlayer(int argc, char** argv)
{
    pw_init(&argc, &argv);
    term.p = this;

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
        .format = SPA_AUDIO_FORMAT_S16,
        .rate = app::def::sampleRate,
        .channels = app::def::channels
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
    {
        if (currSongIdx > (long)songs.size() - 1)
            break;

        playCurrent();
    }
}

void
PipeWirePlayer::playCurrent()
{
    auto wave = loadFileToCharArray(songs[currSongIdx]);
    pcmData = (s16*)wave.data();
    pcmSize = wave.size() / sizeof(s16);
    pcmPos = 0;

    /* TODO: move these to class fields maybe */
    pw.format = SPA_AUDIO_FORMAT_S16;
    pw.sampleRate = app::def::sampleRate;
    pw.channels = app::def::channels;

    setupPlayer(pw.format, pw.sampleRate, pw.channels);

    term.updateUI();
    pw_main_loop_run(pw.loop);

    /* in this order */
    pw_stream_destroy(pw.stream);
    pw_main_loop_destroy(pw.loop);

    if (next || repeatAll)
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
        finished = true;
}

} /* namespace app */
