#include "app.hh"
#include "input.hh"
#include "play.hh"
#include "utils.hh"

#include <cmath>
#include <thread>

namespace app
{

static void
drawBorders(WINDOW* pWin)
{
    cchar_t ls, rs, ts, bs, tl, tr, bl, br;
    setcchar(&ls, L"┃", 0, Curses::color::blue, nullptr);
    setcchar(&rs, L"┃", 0, Curses::color::blue, nullptr);
    setcchar(&ts, L"━", 0, Curses::color::blue, nullptr);
    setcchar(&bs, L"━", 0, Curses::color::blue, nullptr);
    setcchar(&tl, L"┏", 0, Curses::color::blue, nullptr);
    setcchar(&tr, L"┓", 0, Curses::color::blue, nullptr);
    setcchar(&bl, L"┗", 0, Curses::color::blue, nullptr);
    setcchar(&br, L"┛", 0, Curses::color::blue, nullptr);
    wborder_set(pWin, &ls, &rs, &ts, &bs, &tl, &tr, &bl, &br);
}

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
	.destroy {},
    .state_changed {},
    .control_info {},
    .io_changed = ioChangedCB,
    .param_changed {},
    .add_buffer {},
    .remove_buffer {},
    .process = play::onProcessCB,
    .drained {},
    .command {},
    .trigger_done {},
};


std::mutex PipeWireData::mtx;
f32 PipeWirePlayer::chunk[chunkSize] {};

Curses::Curses()
{
    initscr();
    start_color();
    use_default_colors();
    curs_set(0);
    noecho();
    cbreak();
    timeout(def::updateRate);
    keypad(stdscr, true);
    refresh();

    int td = (Curses::termdef);
    init_pair(Curses::green, COLOR_GREEN, td);
    init_pair(Curses::yellow, COLOR_YELLOW, td);
    init_pair(Curses::blue, COLOR_BLUE, td);
    init_pair(Curses::cyan, COLOR_CYAN, td);
    init_pair(Curses::red, COLOR_RED, td);
    init_pair(Curses::white, COLOR_WHITE, td);

    resizeWindows();
}

Curses::~Curses()
{
    endwin();

    if (p->songs.empty())
        COUT("kmp: no input provided\n");
}

void
Curses::drawUI()
{
    /* ncurses is not thread safe */
    std::lock_guard lock(mtx);

    int maxy = getmaxy(stdscr), maxx = getmaxx(stdscr);

    if (maxy > 6 && maxx > 6)
    {
        touchwin(stdscr);

        if (update.bStatus)     { update.bStatus     = false; drawStatus();     }
        if (update.bInfo)       { update.bInfo       = false; drawInfo();       }
        if (update.bBottomLine) { update.bBottomLine = false; drawBottomLine(); }
        if (update.bPlayList)   { update.bPlayList   = false; drawPlayList();   }
    }
}

void
Curses::resizeWindows()
{
    int maxy = getmaxy(stdscr), maxx = getmaxx(stdscr);

    pl.pBor = subwin(stdscr, maxy - listYPos - 1, maxx, listYPos, 0);
    pl.pCon = derwin(pl.pBor, getmaxy(pl.pBor) - 1, getmaxx(pl.pBor) - 2, 1, 1);

    int iXW = std::round(maxx*0.4);
    int iYP = std::round(maxx*0.6);
    int sYP = iXW;

    info.pBor = subwin(stdscr, listYPos, iYP, 0, iXW);
    info.pCon = derwin(info.pBor, getmaxy(info.pBor) - 1, getmaxx(info.pBor) - 2, 1, 1);

    status.pBor = subwin(stdscr, listYPos, sYP, 0, 0);
    status.pCon = derwin(status.pBor, getmaxy(status.pBor) - 1, getmaxx(status.pBor) - 2, 1, 1);
}

void
Curses::drawTime()
{
    u64 t = (p->pcmPos/p->pw.channels) / p->pw.sampleRate;
    u64 maxT = (p->pcmSize/p->pw.channels) / p->pw.sampleRate;

    f64 mF = t / 60.0;
    u64 m = u64(mF);
    f64 frac = 60 * (mF - m);

    f64 mFMax = maxT / 60.0;
    u64 mMax = u64(mFMax);
    u64 fracMax = 60 * (mFMax - mMax);

    auto timeStr = std::format("{}:{:02.0f} / {}:{:02d} (min/sec)", m, frac, mMax, fracMax);
    if (p->bPaused) { timeStr = "(paused) " + timeStr; }

    mvwaddnstr(status.pCon, 0, 0, timeStr.data(), getmaxx(status.pCon));
}

void 
Curses::drawVolume()
{
    auto volumeStr = std::format("volume: {:3.0f}%\n", 100.0 * p->volume);
    size_t seg = (p->volume / def::maxVolume) * std::size(volumeLevels);
    constexpr short volumeColors[] {color::green, color::yellow, color::red};
    constexpr short mutedColor = color::blue;
    size_t max = std::min(seg, std::size(volumeLevels));
    int maxx = getmaxx(status.pCon);

    /* normilize * max number (aka size(volumeColors) */
    auto calcColor = [&](int i) -> int {
        return (((f32)(i) / std::size(volumeLevels))) * std::size(volumeColors);
    };

    constexpr int colOff = 3;
    int strColOff = max - colOff;
    int strCol = calcColor(strColOff);
    if (strCol < 0) strCol = 0;

    int col = p->bMuted ? COLOR_PAIR(mutedColor) : A_BOLD | COLOR_PAIR(volumeColors[strCol]);
    wattron(status.pCon, col);
    mvwaddnstr(status.pCon, 1, 0, volumeStr.data(), maxx);
    wattroff(status.pCon, col);

    for (size_t i = 0; i < max + 1; i++)
    {
        long off = i - colOff;
        if (off < 0) off = 0;

        int segCol = calcColor(off);
        int imgCol = p->bMuted ? COLOR_PAIR(mutedColor) : COLOR_PAIR(volumeColors[segCol]);

        wattron(status.pCon, imgCol);
        mvwaddnwstr(status.pCon, 1, i + volumeStr.size(), volumeLevels[i], maxx - volumeStr.size());
        wattroff(status.pCon, imgCol);
    }

    drawBorders(status.pBor);
}

void
Curses::drawPlayListCounter()
{
    auto songCounterStr = std::format("total: {} / {}", p->currSongIdx + 1, p->songs.size());
    if (p->bRepeatAfterLast) { songCounterStr += " (Repeat After Last)" ; }

    mvwaddnstr(status.pCon, 3, 0, songCounterStr.data(), getmaxx(status.pCon));
}

void
Curses::drawTitle()
{
    auto ls = "playing: " + p->info.title;
    ls.resize(getmaxx(pl.pBor) - 1);

    move(5, 0);
    clrtoeol();
    attron(A_BOLD | A_ITALIC | COLOR_PAIR(yellow));
    mvaddstr(5, 1, ls.data());
    attroff(A_BOLD | A_ITALIC | COLOR_PAIR(yellow));
}

void
Curses::drawPlayList()
{
    long maxy = getmaxy(pl.pCon);

    long startFromY = 0; /* offset from border */
    long sel = selected;

    if ((long)(p->songs.size() - 1) - firstInList < (maxy))
        firstInList = (p->songs.size() - 1) - (maxy - 2);
    if ((long)p->songs.size() < maxy - 1)
        firstInList = 0;
    else if (firstInList < 0)
        firstInList = 0;

    long listSizeBound = firstInList + (maxy - 1); /* - 2*borders */

    if (selected > listSizeBound - 1)
        firstInList = selected - (maxy - 2); /* -3 == (2*borders - 1) */
    else if (selected < firstInList)
        firstInList = selected;

    werase(pl.pBor);
    for (long i = firstInList; i < (long)p->songs.size() && startFromY < maxy; i++, startFromY++)
    {
        auto lineStr = utils::removePath(p->songs[i]);
        lineStr.resize(getmaxx(pl.pCon) - 1);

        if (i == sel)
            wattron(pl.pCon, A_REVERSE);
        if (i == p->currSongIdx)
            wattron(pl.pCon, A_BOLD | COLOR_PAIR(app::Curses::yellow));

        mvwaddstr(pl.pCon, startFromY, getbegx(pl.pCon), lineStr.data());

        wattroff(pl.pCon, A_REVERSE | A_BOLD | COLOR_PAIR(app::Curses::yellow));
    }

    drawBorders(pl.pBor);
}

void
Curses::drawBottomLine()
{
    int maxy = getmaxy(stdscr);
    move(maxy - 1, 0);
    clrtoeol();
    move(maxy - 1, 1);
    if (!p->searchingNow.empty() && !p->foundIndices.empty())
    {
        auto ss = std::format(" [{}/{}]", p->currFoundIdx + 1, p->foundIndices.size());
        auto s = L"'" + p->searchingNow + L"'" + std::wstring(ss.begin(), ss.end());
        addwstr(s.data());
    }

    /* draw selected index */
    auto sel = std::format("{}\n", p->term.selected + 1);
    mvaddstr(maxy - 1, (getmaxx(stdscr) - 1) - sel.size(), sel.data());
}

void
Curses::drawInfo()
{
    int maxx = getmaxx(info.pCon);
    constexpr std::string_view sTi = "title: ";
    constexpr std::string_view sAl = "album: ";
    constexpr std::string_view sAr = "artist: ";

    werase(info.pBor);

    mvwaddnstr(info.pCon, 0, 0, sTi.data(), maxx*2);
    wattron(info.pCon, A_BOLD | A_ITALIC | COLOR_PAIR(color::yellow));
    mvwaddnstr(info.pCon, 0, sTi.size(), p->info.title.data(), (maxx*2) - sTi.size());
    wattroff(info.pCon, A_BOLD | A_ITALIC | COLOR_PAIR(color::yellow));

    mvwaddnstr(info.pCon, 2, 0, sAl.data(), maxx);
    wattron(info.pCon, A_BOLD);
    mvwaddnstr(info.pCon, 2, sAl.size(), p->info.album.data(), maxx);
    wattroff(info.pCon, A_BOLD);

    mvwaddnstr(info.pCon, 3, 0, sAr.data(), maxx);
    wattron(info.pCon, A_BOLD);
    mvwaddnstr(info.pCon, 3, sAr.size(), p->info.artist.data(), maxx);
    wattroff(info.pCon, A_BOLD);

    drawBorders(info.pBor);
}

void
Curses::drawStatus()
{
    werase(status.pBor);

    drawTime();
    drawVolume();
    drawPlayListCounter();

    drawBorders(status.pBor);
}

SongInfo::SongInfo(std::string_view _path, const SndfileHandle& h)
{
    const char* _title = h.getString(SF_STR_TITLE);
    const char* _copyright = h.getString(SF_STR_COPYRIGHT);
    const char* _software = h.getString(SF_STR_SOFTWARE);
    const char* _artist = h.getString(SF_STR_ARTIST);
    const char* _comment = h.getString(SF_STR_COMMENT);
    const char* _date = h.getString(SF_STR_DATE);
    const char* _album = h.getString(SF_STR_ALBUM);
    const char* _license = h.getString(SF_STR_LICENSE);
    const char* _tracknumber = h.getString(SF_STR_TRACKNUMBER);
    const char* _genre = h.getString(SF_STR_GENRE);

    if (_title) title = _title;
    else title = utils::removePath(_path);

    auto set = [](const char* s) -> std::string {
        if (s) return s;
        else return {};
    };

    copyright = set(_copyright);
    software = set(_software);
    artist = set(_artist);
    comment = set(_comment);
    date = set(_date);
    album = set(_album);
    license = set(_license);
    tracknumber = set(_tracknumber);
    genre = set(_genre);
}

PipeWirePlayer::PipeWirePlayer(int argc, char** argv)
{
    term.p = this;

    pw_init(&argc, &argv);

    term.firstInList = 0;

    for (int i = 1; i < argc; i++)
    {
        std::string s = argv[i];

        if (s.ends_with(".flac") ||
            s.ends_with(".opus") ||
            s.ends_with(".mp3")  ||
            s.ends_with(".ogg")  ||
            s.ends_with(".wav")  ||
            s.ends_with(".caf")  ||
            s.ends_with(".aif"))
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
        .flags {},
        .rate = sampleRate,
        .channels = channels,
        .position {}
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

    /* skip song on error */
    if (hSnd.error() == 0)
    {
        pw.format = SPA_AUDIO_FORMAT_F32;
        pw.sampleRate = hSnd.samplerate();
        pw.channels = hSnd.channels();

        pcmPos = 0;
        pcmSize = hSnd.frames() * pw.channels;

        info = SongInfo(currSongName(), hSnd);

        setupPlayer(pw.format, pw.sampleRate, pw.channels);

        term.updateAll();
        term.drawUI();
        refresh(); /* needed to avoid one second black screen before first getch update */

        pw_main_loop_run(pw.loop);

        /* in this order */
        pw_stream_destroy(pw.stream);
        pw_main_loop_destroy(pw.loop);
    }
    else
    {
        bNext = true;
    }

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

bool
PipeWirePlayer::subStringSearch(enum search::dir direction)
{
    const wchar_t* prefix = direction == search::dir::forward ? L"search: " : L"backwards-search: ";
    wint_t wb[30] {};

    timeout(5000);
    input::readWString(prefix, wb, std::size(wb));
    timeout(def::updateRate);

    if (wcsnlen((wchar_t*)wb, std::size(wb)) > 0)
        searchingNow = (wchar_t*)wb;
    else
        return false;


    if (!searchingNow.empty())
    {
        foundIndices = search::getIndexList(songs, searchingNow, direction);
        currFoundIdx = 0;

        return true;
    }

    return false;
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

void
PipeWirePlayer::setSeek()
{
    wint_t wb[10] {};

    timeout(5000);
    input::readWString(L"time: ", wb, std::size(wb));
    timeout(def::updateRate);

    if (wcsnlen((wchar_t*)wb, std::size(wb)) > 0)
    {
        std::lock_guard lock(pw.mtx);

        auto n = input::parseTimeString((wchar_t*)wb, this);
        if (n.has_value())
        {
            n.value() *= pw.sampleRate;
            hSnd.seek(n.value(), SEEK_SET);
            pcmPos = hSnd.seek(0, SEEK_CUR) * pw.channels;
        }
    }
}

void
PipeWirePlayer::jumpTo()
{
    wint_t wb[10] {};

    timeout(5000);
    input::readWString(L"select: ", wb, std::size(wb));
    timeout(def::updateRate);

    if (wcsnlen((wchar_t*)wb, std::size(wb)) > 0)
    {
        wchar_t* end;
        long num = wcstol((wchar_t*)wb, &end, 10);
        num = std::clamp((long)num, 1L, (long)songs.size());
        term.selected = num - 1;
    }
}


} /* namespace app */
