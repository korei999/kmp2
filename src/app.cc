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
    init_pair(Curses::white, COLOR_WHITE, td);

    pPlayList = subwin(stdscr, getmaxy(stdscr) - listYPos - 1, getmaxx(stdscr), listYPos, 0);
}

void
Curses::drawUI()
{
    /* ncurses is not thread safe */
    std::lock_guard lock(mtx);

    int maxy = getmaxy(stdscr), maxx = getmaxx(stdscr);

    if (maxy >= 5 && maxx >= 5)
    {
        if (update.bTime)       { update.bTime       = false; drawTime(); }
        if (update.bVolume)     { update.bVolume     = false; drawVolume(); }
        if (update.bSongName)   { update.bSongName   = false; drawTitle(); drawPlayListCounter(); }
        if (update.bPlayList)   { update.bPlayList   = false; drawPlayList(); }
        if (update.bBottomLine) { update.bBottomLine = false; drawBottomLine(); }

        refresh();
    }
}

void
Curses::resizePlayListWindow()
{
    pPlayList = subwin(stdscr, getmaxy(stdscr) - listYPos - 1, getmaxx(stdscr), listYPos, 0);
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
    if (p->bPaused) { timeStr = "(paused) " + timeStr; }

    move(0, 0);
    clrtoeol();
    mvaddstr(0, 1, timeStr.data());
    move(1, 0);
    clrtoeol();
}

void 
Curses::drawVolume()
{
    auto volumeStr = std::format("volume: {:3.0f}%\n", 100.0 * p->volume);
    size_t seg = (p->volume / def::maxVolume) * std::size(volumeLevels);
    constexpr s8 volumeColors[] {color::green, color::yellow, color::red};
    size_t max = std::min(seg, std::size(volumeLevels));

    auto calcColor = [&](int i) -> int {
        /* normilize * max number (aka size(volumeColors) */
        return (((f32)(i) / std::size(volumeLevels))) * std::size(volumeColors);
    };

    int strColOff = max - 3;
    int strCol = calcColor(strColOff);
    if (strCol < 0) strCol = 0;

    move(2, 0);
    clrtoeol();
    attron(A_BOLD | COLOR_PAIR(volumeColors[strCol]));
    mvaddstr(2, 1, volumeStr.data());
    attroff(A_BOLD | COLOR_PAIR(volumeColors[strCol]));

    for (size_t i = 0; i < max + 1; i++)
    {
        long off = i - 3;
        if (off < 0) off = 0;

        int segCol = calcColor(off);

        attron(COLOR_PAIR(volumeColors[segCol]));
        mvaddwstr(2, i + 14, volumeLevels[i]);
        attroff(COLOR_PAIR(volumeColors[segCol]));
    }

    move(3, 0);
    clrtoeol();
}

void
Curses::drawPlayListCounter()
{
    auto songCounterStr = std::format("{} / {}", p->currSongIdx + 1, p->songs.size());
    if (p->bRepeatAfterLast) { songCounterStr += " (Repeat After Last)" ; }

    move(4, 0);
    clrtoeol();
    attron(A_ITALIC);
    mvaddstr(4, 1, songCounterStr.data());
    attroff(A_ITALIC);
}

void
Curses::drawTitle()
{
    auto ls = "playing: " + p->info.title;
    ls.resize(getmaxx(pPlayList) - 1);

    move(5, 0);
    clrtoeol();
    attron(A_BOLD | A_ITALIC | COLOR_PAIR(yellow));
    mvaddstr(5, 1, ls.data());
    attroff(A_BOLD | A_ITALIC | COLOR_PAIR(yellow));
}

void
Curses::drawPlayList()
{
    long maxy = getmaxy(pPlayList);

    long startFromY = 1; /* offset from border */
    long sel = p->term.selected;

    if ((long)(p->songs.size() - 1) - firstInList < maxy - 1)
        firstInList = (p->songs.size() - 1) - (maxy - 3);
    if ((long)p->songs.size() < maxy)
        firstInList = 0;
    if (firstInList < 0)
        firstInList = 0;

    long listSizeBound = firstInList + (maxy - 2); /* - 2*borders */

    if (selected > listSizeBound - 1)
        firstInList = selected - (maxy - 3); /* -3 == (2*borders - 1) */
    else if (selected < firstInList)
        firstInList = selected;

    for (long i = firstInList; i < (long)p->songs.size() && startFromY < maxy - 1; i++, startFromY++)
    {
        auto lineStr = utils::removePath(std::format("{}", p->songs[i]));
        lineStr.resize(getmaxx(pPlayList) - 1);

        if (i == sel)
            wattron(pPlayList, A_REVERSE);
        if (i == p->currSongIdx)
            wattron(pPlayList, A_BOLD | COLOR_PAIR(app::Curses::yellow));

        wmove(pPlayList, startFromY, getbegx(pPlayList) + 1);
        wclrtoeol(pPlayList);
        waddstr(pPlayList, lineStr.data());

        wattroff(pPlayList, A_REVERSE | A_BOLD | COLOR_PAIR(app::Curses::yellow));
    }

    drawBorder();
    touchwin(stdscr);
}

void
Curses::drawBottomLine()
{
    move(getmaxy(stdscr) - 1, 0);
    clrtoeol();
    move(getmaxy(stdscr) - 1, 1);
    if (!p->searchingNow.empty() && !p->foundIndices.empty())
    {
        auto ss = std::format(" [{}/{}]", p->currFoundIdx + 1, p->foundIndices.size());
        auto s = L"'" + p->searchingNow + L"'" + std::wstring(ss.begin(), ss.end());
        addwstr(s.data());
    }
}

void
Curses::drawBorder()
{
    cchar_t ls, rs, ts, bs, tl, tr, bl, br;
    setcchar(&ls, L"┃", 0, color::blue, nullptr);
    setcchar(&rs, L"┃", 0, color::blue, nullptr);
    setcchar(&ts, L"━", 0, color::blue, nullptr);
    setcchar(&bs, L"━", 0, color::blue, nullptr);
    setcchar(&tl, L"┏", 0, color::blue, nullptr);
    setcchar(&tr, L"┓", 0, color::blue, nullptr);
    setcchar(&bl, L"┗", 0, color::blue, nullptr);
    setcchar(&br, L"┛", 0, color::blue, nullptr);
    wborder_set(pPlayList, &ls, &rs, &ts, &bs, &tl, &tr, &bl, &br);
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
        bFinished = true;
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
                                     "kmpMusicPlayback",
                                     pw_properties_new(PW_KEY_MEDIA_TYPE, "Audio",
                                                       PW_KEY_MEDIA_CATEGORY, "Playback",
                                                       PW_KEY_MEDIA_ROLE, "Music",
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
                      params, std::size(params));
}

void
PipeWirePlayer::playAll()
{
    while (!bFinished)
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

    auto title = hSnd.getString(SF_STR_TITLE);
    if (title) info.title = title;
    else info.title = utils::removePath(currSongName());

    setupPlayer(pw.format, pw.sampleRate, pw.channels);

    term.updateAll();
    term.drawUI();
    pw_main_loop_run(pw.loop);

    /* in this order */
    pw_stream_destroy(pw.stream);
    pw_main_loop_destroy(pw.loop);

    if (bNewSongSelected)
    {
        bNewSongSelected = false;
        currSongIdx = term.selected;
    }
    else if (bNext)
    {
        bNext = false;
        currSongIdx++;
        if (currSongIdx > (long)songs.size() - 1)
            currSongIdx = 0;
    }
    else if (bPrev)
    {
        bPrev = false;
        currSongIdx--;
        if (currSongIdx < 0)
            currSongIdx = songs.size() - 1;
    }
    else
        currSongIdx++;

    if (currSongIdx > (long)songs.size() - 1)
    {
        if (bRepeatAfterLast)
            currSongIdx = 0;
        else
            bFinished = true;
    }
}

void
PipeWirePlayer::subStringSearch(enum search::dir direction)
{
    char firstChar = direction == search::dir::forward ? '/' : '?';

    wint_t wb[30] {};
    timeout(5000);
    echo();
    move(getmaxy(stdscr) - 1, 0);
    clrtoeol();
    addch(firstChar);

    getn_wstr(wb, 30);
    noecho();
    timeout(1000);

    if (wcsnlen((wchar_t*)wb, std::size(wb)) > 0)
        searchingNow = (wchar_t*)wb;

    if (!searchingNow.empty())
    {
        foundIndices = search::getIndexList(songs, searchingNow, direction);
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

        term.selected = foundIndices[currFoundIdx];
        centerOn(term.selected);
    }
}

void
PipeWirePlayer::centerOn(size_t i)
{
    term.selected = i;
    term.firstInList = (term.selected - (term.playListMaxY() - 3) / 2);
}

} /* namespace app */
