#include "app.hh"
#include "input.hh"
#include "play.hh"
#include "utils.hh"
#include "color.hh"

#include <algorithm>
#include <cmath>
#include <thread>

namespace app
{

static void
drawBorders(WINDOW* pWin, enum color::curses color = defaults::borderColor)
{
    cchar_t ls, rs, ts, bs, tl, tr, bl, br;
    setcchar(&ls, L"┃", 0, color, nullptr);
    setcchar(&rs, L"┃", 0, color, nullptr);
    setcchar(&ts, L"━", 0, color, nullptr);
    setcchar(&bs, L"━", 0, color, nullptr);
    setcchar(&tl, L"┏", 0, color, nullptr);
    setcchar(&tr, L"┓", 0, color, nullptr);
    setcchar(&bl, L"┗", 0, color, nullptr);
    setcchar(&br, L"┛", 0, color, nullptr);
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

static void
paramChangedCB([[maybe_unused]] void* data,
               [[maybe_unused]] uint32_t id,
               [[maybe_unused]] const spa_pod* param)
{
    //
}

static const pw_stream_events streamEvents {
    .version = PW_VERSION_STREAM_EVENTS,
    .destroy {},
    .state_changed {},
    .control_info {},
    .io_changed = ioChangedCB,
    .param_changed = paramChangedCB,
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
    set_escdelay(0);
    noecho();
    cbreak();
    timeout(defaults::updateRate);
    keypad(stdscr, true);
    refresh();

    int td = (color::curses::termdef);
    init_pair(color::curses::green, COLOR_GREEN, td);
    init_pair(color::curses::yellow, COLOR_YELLOW, td);
    init_pair(color::curses::blue, COLOR_BLUE, td);
    init_pair(color::curses::cyan, COLOR_CYAN, td);
    init_pair(color::curses::red, COLOR_RED, td);
    init_pair(color::curses::white, COLOR_WHITE, td);
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
    /* disallow drawing to ncurses from multiple threads */
    std::lock_guard lock(mtx);

    int maxy = getmaxy(stdscr), maxx = getmaxx(stdscr);

    if (maxy > 11 && maxx > 11)
    {
        touchwin(stdscr);

        if (update.bStatus)     { update.bStatus     = false; drawStatus();     }
        if (update.bInfo)       { update.bInfo       = false; drawInfo();       }
        if (update.bBottomLine) { update.bBottomLine = false; drawBottomLine(); }
        if (update.bPlayList)   { update.bPlayList   = false; drawPlayList();   }

        if (p->bDrawVisualizer && update.bVisualizer)
        {
            update.bVisualizer = false;
            drawVisualizer();
        }
    }
    else
    {
        erase();
        std::string tooSmall0 = FMT("too small ({}/{})", maxy, maxx);
        constexpr std::string_view tooSmall1 = ">= 11/11 needed";
        mvaddstr((maxy-1)/2 - 1, (maxx-tooSmall0.size()-1)/2, tooSmall0.data());
        mvaddstr((maxy-1)/2 + 1, (maxx-tooSmall1.size()-1)/2, tooSmall1.data());
    }
}

void
Curses::resizeWindows()
{
    int maxy = getmaxy(stdscr), maxx = getmaxx(stdscr);

    int visOff = 0;
    if (p->bDrawVisualizer)
        visOff = visualizerYSize;

    vis.pBor = subwin(stdscr, visualizerYSize, maxx, listYPos, 0);
    vis.pCon = derwin(vis.pBor, getmaxy(vis.pBor), getmaxx(vis.pBor) - 2, 0, 1);

    pl.pBor = subwin(stdscr, maxy - (listYPos + visOff + 1), maxx, listYPos + visOff, 0);
    pl.pCon = derwin(pl.pBor, getmaxy(pl.pBor) - 1, getmaxx(pl.pBor) - 2, 1, 1);

    int iXW = std::round(maxx*0.4);
    int iYP = std::round(maxx*0.6);
    int sYP = iXW;

    info.pBor = subwin(stdscr, listYPos, iYP, 0, iXW);
    info.pCon = derwin(info.pBor, getmaxy(info.pBor) - 1, getmaxx(info.pBor) - 2, 1, 1);

    status.pBor = subwin(stdscr, listYPos, sYP, 0, 0);
    status.pCon = derwin(status.pBor, getmaxy(status.pBor) - 1, getmaxx(status.pBor) - 2, 1, 1);

    redrawwin(stdscr);
    werase(stdscr);
    updateAll();
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

    auto timeStr = FMT("{}:{:02.0f} / {}:{:02d}", m, frac, mMax, fracMax);
    if (p->bPaused) { timeStr = "(paused) " + timeStr; }
    timeStr = "time: " + timeStr;

    if (p->pw.sampleRate != p->pw.origSampleRate)
        timeStr += FMT(" ({:.0f}% speed)", p->speedMul * 100);

    mvwaddnstr(status.pCon, 0, 0, timeStr.data(), getmaxx(status.pCon));
}

int 
Curses::drawVolume()
{
    auto volumeStr = FMT("volume: {:3.0f}%\n", 100.0 * p->volume);
    int maxx = getmaxx(status.pCon);

    long maxWidth = maxx - volumeStr.size() - 1;
    f64 maxLine = (p->volume * (f64)maxWidth) * (1.0 - (defaults::maxVolume - 1.0));

    auto getColor = [&](f64 i) -> int {
        f64 val = p->volume * (i / (maxLine));

        if (val > 1.01) return color::curses::red;
        else if (val > 0.51) return color::curses::yellow;
        else return color::curses::green;
    };

    int sCol = p->bMuted ? COLOR_PAIR(color::blue) : (A_BOLD | COLOR_PAIR(getColor(maxLine)));
    wattron(status.pCon, sCol);
    mvwaddnstr(status.pCon, 1, 0, volumeStr.data(), maxx);
    wattroff(status.pCon, sCol);

    for (long i = 0; i < maxLine; i++)
    {
        int color;
        const wchar_t* icon;
        if (p->bMuted)
        {
            color = defaults::mutedColor;
            icon = blockIcon2;
        }
        else
        {
            color = COLOR_PAIR(getColor(i));
            icon = blockIcon1;
        }

        wattron(status.pCon, color);
        mvwaddwstr(status.pCon, 1, 1 + i + volumeStr.size(), icon);
        wattroff(status.pCon, color);
    }

    return sCol;
}

void
Curses::drawPlayListCounter()
{
    auto songCounterStr = FMT("total: {} / {}", p->currSongIdx + 1, p->songs.size());
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
    attron(A_BOLD | A_ITALIC | COLOR_PAIR(color::curses::yellow));
    mvaddstr(5, 1, ls.data());
    attroff(A_BOLD | A_ITALIC | COLOR_PAIR(color::curses::yellow));
}

void
Curses::drawPlayList()
{
    long maxy = getmaxy(pl.pCon);
    long startFromY = 0; /* offset from border */

    fixCursorPos();

    werase(pl.pBor);
    for (long i = firstInList; i < (long)p->songs.size() && startFromY < maxy; i++, startFromY++)
    {
        auto lineStr = utils::removePath(p->songs[i]);
        lineStr.resize(getmaxx(pl.pCon) - 1);

        if (i == selected)
            wattron(pl.pCon, A_REVERSE);
        if (i == p->currSongIdx)
            wattron(pl.pCon, A_BOLD | COLOR_PAIR(color::curses::yellow));

        mvwaddstr(pl.pCon, startFromY, getbegx(pl.pCon), lineStr.data());

        wattroff(pl.pCon, A_REVERSE | A_BOLD | COLOR_PAIR(color::curses::yellow));
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
        auto ss = FMT(" [{}/{}]", p->currFoundIdx + 1, p->foundIndices.size());
        auto s = L"'" + p->searchingNow + L"'" + std::wstring(ss.begin(), ss.end());
        addwstr(s.data());
    }

    /* draw selected index */
    auto sel = FMT("{}\n", p->term.selected + 1);
    mvaddstr(maxy - 1, (getmaxx(stdscr) - 1) - sel.size(), sel.data());
}

void
Curses::drawInfo()
{
    int maxx = getmaxx(info.pCon);
    constexpr std::string_view sTitle = "title: ";
    constexpr std::string_view sAlbum = "album: ";
    constexpr std::string_view sArtist = "artist: ";

    werase(info.pBor);

    mvwaddnstr(info.pCon, 0, 0, sTitle.data(), maxx*2);
    wattron(info.pCon, A_BOLD | A_ITALIC | COLOR_PAIR(color::curses::yellow));
    mvwaddnstr(info.pCon, 0, sTitle.size(), p->info.title.data(), (maxx*2) - sTitle.size());
    wattroff(info.pCon, A_BOLD | A_ITALIC | COLOR_PAIR(color::curses::yellow));

    mvwaddnstr(info.pCon, 2, 0, sAlbum.data(), maxx);
    wattron(info.pCon, A_BOLD);
    mvwaddnstr(info.pCon, 2, sAlbum.size(), p->info.album.data(), maxx - sAlbum.size());
    wattroff(info.pCon, A_BOLD);

    mvwaddnstr(info.pCon, 3, 0, sArtist.data(), maxx);
    wattron(info.pCon, A_BOLD);
    mvwaddnstr(info.pCon, 3, sArtist.size(), p->info.artist.data(), maxx - sArtist.size());
    wattroff(info.pCon, A_BOLD);

    drawBorders(info.pBor);
}

void
Curses::drawStatus()
{
    werase(status.pBor);

    drawTime();
    auto color = drawVolume();
    drawPlayListCounter();

    drawBorders(status.pBor, (color::curses)PAIR_NUMBER(color));
}

void
Curses::drawVisualizer()
{
    werase(vis.pBor);

    int maxy = getmaxy(vis.pCon), maxx = getmaxx(vis.pCon);
    int lastChunkSize = p->pw.lastNFrames * p->pw.channels;

    std::vector<u32> bars(maxx);
    int accSize = lastChunkSize / bars.size();
    long chunkPos = 0;

    /* stupid simple time domain visualization. TODO: figure out dfft or something */
    for (long i = 0; i < (long)bars.size(); i++)
    {
        f32 acc = 0;
        for (long j = 0; j < accSize; j++)
        {
            auto newAcc = std::abs((p->chunk[chunkPos++]));
            if (newAcc > acc) acc = newAcc;
        }

        u32 h = std::round(acc * defaults::visualizerScalar); /* scale by some number to boost the bars heights */
        if (h > (u32)maxy) h = maxy;
        bars[i] = h;
    }

    constexpr short barHeightColors[4] {
        color::curses::red, color::curses::yellow, color::curses::green, color::curses::green
    };

    for (int r = 0; r < maxx; r++)
    {
        int maxHeight = bars[r];

        /* draw from bottom (maxy) upwards */
        for (int c = maxy - 1; c >= maxy - maxHeight; c--)
        {
            short color = COLOR_PAIR(barHeightColors[c]);

            wattron(vis.pCon, color);
            mvwaddwstr(vis.pCon, c, r, defaults::visualizerSymbol);
            wattroff(vis.pCon, color);
        }
    }
}

void
Curses::fixCursorPos()
{
    long maxy = getmaxy(pl.pCon);

    if (((long)(p->songs.size() - 1) - firstInList) < (maxy - 1))
        firstInList = (p->songs.size() - 1) - (maxy - 2);
    if ((long)p->songs.size() < maxy - 1)
        firstInList = 0;
    else if (firstInList < 0)
        firstInList = 0;

    long listSizeBound = firstInList + (maxy - 1);

    if (selected > listSizeBound - 1)
        firstInList = selected - (maxy - 2);
    else if (selected < firstInList)
        firstInList = selected;
}

PipeWirePlayer::PipeWirePlayer(int argc, char** argv)
{
    term.p = this;
    term.resizeWindows();

    pw_init(&argc, &argv);

    term.firstInList = 0;

    for (int i = 2; i < argc; i++)
    {
        std::string s = argv[i];

        for (auto& f : defaults::formatsSupported)
            if (s.ends_with(f))
            {
                songs.push_back(std::move(s));
                break;
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
        pw.origSampleRate = pw.sampleRate;

        pcmPos = 0;
        pcmSize = hSnd.frames() * pw.channels;

        info = song::Info(currSongName(), hSnd);

        /* restore speed multiplier */
        pw.sampleRate *= speedMul;
        setupPlayer(pw.format, pw.sampleRate, pw.channels);

        term.updateAll();
        term.drawUI();

        if (!ready)
            refresh(); /* refresh before first getch update */

        ready = true;

        /* TODO: there is probably a better way to update params than just to reset the whole thing */
updateParamsHack:
        if (bChangeParams)
        {
            bChangeParams = false;
            setupPlayer(pw.format, pw.sampleRate, pw.channels);
            term.resizeWindows();
            term.drawUI();
        }

        pw_main_loop_run(pw.loop);

        /* in this order */
        pw_stream_destroy(pw.stream);
        pw_main_loop_destroy(pw.loop);

        if (bChangeParams)
            goto updateParamsHack;
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

    timeout(defaults::timeOut);
    input::readWString(prefix, wb, std::size(wb));
    timeout(defaults::updateRate);

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

    timeout(defaults::timeOut);
    input::readWString(L"time: ", wb, std::size(wb));
    timeout(defaults::updateRate);

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

    timeout(defaults::timeOut);
    input::readWString(L"select: ", wb, std::size(wb));
    timeout(defaults::updateRate);

    if (wcsnlen((wchar_t*)wb, std::size(wb)) > 0)
    {
        wchar_t* end;
        long num = wcstol((wchar_t*)wb, &end, 10);
        num = std::clamp((long)num, 1L, (long)songs.size());
        term.selected = num - 1;
    }
}


} /* namespace app */
