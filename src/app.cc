#include "app.hh"
#include "color.hh"
#include "input.hh"
#include "play.hh"
#include "utils.hh"
#ifdef MPRIS_LIB
#    include "mpris.hh"
#endif

#include <algorithm>
#include <cassert>
#include <cmath>
#include <thread>

namespace app
{

f32 PipeWirePlayer::m_chunk[chunkSize] {};
std::mutex PipeWireData::mtx;
const pw_stream_events PipeWireData::streamEvents {
    .version = PW_VERSION_STREAM_EVENTS,
    .destroy {},
    .state_changed = play::stateChangedCB,
    .control_info {},
    .io_changed = play::ioChangedCB,
    .param_changed = play::paramChangedCB,
    .add_buffer {},
    .remove_buffer {},
    .process = play::onProcessCB,
    .drained {},
    .command {},
    .trigger_done {},
};

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


CursesUI::CursesUI()
{
    /* reopen stdin to fix getch if pipe was used */
    if (!freopen("/dev/tty", "r", stdin))
        LOG_BAD("freopen(\"/dev/tty\", \"r\", stdin)\n");

    initscr();
    start_color();
    if (defaults::bTransparentBg) use_default_colors();
    curs_set(0);
    set_escdelay(0);
    noecho();
    cbreak();
    timeout(defaults::updateRate);
    keypad(stdscr, true);
    refresh();

    int td = defaults::bTransparentBg ? -1 : COLOR_BLACK;

    if (defaults::bCustomColorPallete)
    {
#ifndef NDEBUG
        constexpr auto printColor = [](std::string_view s, color::rgb c) -> void {
            CERR("{}: ({}, {}, {})\n", s, c.r, c.g, c.b);
        };

        printColor("COLOR_BLACK", defaults::black);
        printColor("COLOR_GREEN", defaults::green);
        printColor("COLOR_YELLOW", defaults::yellow);
        printColor("COLOR_BLUE", defaults::blue);
        printColor("COLOR_MAGENTA", defaults::magenta);
        printColor("COLOR_CYAN", defaults::cyan);
        printColor("COLOR_WHITE", defaults::white);
#endif
        auto initCustom = [](int cursesColor, color::rgb c) -> void {
            init_color(cursesColor, c.r, c.g, c.b);
        };

        initCustom(COLOR_BLACK,   defaults::black);
        initCustom(COLOR_RED,     defaults::red);
        initCustom(COLOR_GREEN,   defaults::green);
        initCustom(COLOR_YELLOW,  defaults::yellow);
        initCustom(COLOR_BLUE,    defaults::blue);
        initCustom(COLOR_MAGENTA, defaults::magenta);
        initCustom(COLOR_CYAN,    defaults::cyan);
        initCustom(COLOR_WHITE,   defaults::white);
    }

    init_pair(color::green, COLOR_GREEN, td);
    init_pair(color::yellow, COLOR_YELLOW, td);
    init_pair(color::blue, COLOR_BLUE, td);
    init_pair(color::cyan, COLOR_CYAN, td);
    init_pair(color::red, COLOR_RED, td);
    init_pair(color::white, COLOR_WHITE, td);
    init_pair(color::black, COLOR_BLACK, td);
}

CursesUI::~CursesUI()
{
    endwin();

    if (m_p->m_songs.empty())
        COUT("kmp: no input provided\n");
}

void
CursesUI::drawUI()
{
    /* disallow drawing to ncurses screen from multiple threads */
    std::lock_guard lock(m_mtx);

    int maxy = getmaxy(stdscr), maxx = getmaxx(stdscr);

    if (maxy > 11 && maxx > 11)
    {
        touchwin(stdscr);

        if (m_update.bStatus)     { m_update.bStatus     = false; drawStatus();     }
        if (m_update.bInfo)       { m_update.bInfo       = false; drawInfo();       }
        if (m_update.bBottomLine) { m_update.bBottomLine = false; drawBottomLine(); }
        if (m_update.bPlayList)   { m_update.bPlayList   = false; drawPlayList();   }

        if (m_bDrawVisualizer && m_update.bVisualizer)
        {
            m_update.bVisualizer = false;
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
CursesUI::resizeWindows()
{
    int maxy = getmaxy(stdscr), maxx = getmaxx(stdscr);

    if (m_visualizerYSize >= maxy)
    {
        endwin();
        COUT("defaults::visualizerHeight: '{}', is abusurdly big\n", m_visualizerYSize);
        exit(1);
    }

    int visOff = 0;
    if (m_bDrawVisualizer) visOff = m_visualizerYSize;

    m_vis.pBor = subwin(stdscr, m_visualizerYSize, maxx, m_listYPos, 0);
    m_vis.pCon = derwin(m_vis.pBor, getmaxy(m_vis.pBor), getmaxx(m_vis.pBor) - 2, 0, 1);

    m_pl.pBor = subwin(stdscr, maxy - (m_listYPos + visOff + 1), maxx, m_listYPos + visOff, 0);
    m_pl.pCon = derwin(m_pl.pBor, getmaxy(m_pl.pBor) - 1, getmaxx(m_pl.pBor) - 2, 1, 1);

    int iXW = std::round(maxx*0.4);
    int iYP = std::round(maxx*0.6);
    int sYP = iXW;

    m_info.pBor = subwin(stdscr, m_listYPos, iYP, 0, iXW);
    m_info.pCon = derwin(m_info.pBor, getmaxy(m_info.pBor) - 1, getmaxx(m_info.pBor) - 2, 1, 1);

    m_status.pBor = subwin(stdscr, m_listYPos, sYP, 0, 0);
    m_status.pCon = derwin(m_status.pBor, getmaxy(m_status.pBor) - 1, getmaxx(m_status.pBor) - 2, 1, 1);

    redrawwin(stdscr);
    werase(stdscr);
    updateAll();
}

void
CursesUI::toggleVisualizer()
{
    m_bDrawVisualizer = !m_bDrawVisualizer;
    resizeWindows();
    m_update.bPlayList = true;
}

void
CursesUI::drawTime()
{
    u64 t = (m_p->m_pcmPos/m_p->m_pw.channels) / m_p->m_pw.sampleRate;
    u64 maxT = (m_p->m_pcmSize/m_p->m_pw.channels) / m_p->m_pw.sampleRate;

    f64 mF = t / 60.0;
    u64 m = u64(mF);
    f64 frac = 60 * (mF - m);

    f64 mFMax = maxT / 60.0;
    u64 mMax = u64(mFMax);
    u64 fracMax = 60 * (mFMax - mMax);

    auto timeStr = FMT("{}:{:02.0f} / {}:{:02d}", m, frac, mMax, fracMax);
    if (m_p->m_bPaused) { timeStr = "(paused) " + timeStr; }
    timeStr = "time: " + timeStr;

    if (m_p->m_pw.sampleRate != m_p->m_pw.origSampleRate)
        timeStr += FMT(" ({:.0f}% speed)", m_p->m_speedMul * 100);

    auto col = COLOR_PAIR(color::white);
    wattron(m_status.pCon, col);
    mvwaddnstr(m_status.pCon, 0, 0, timeStr.data(), getmaxx(m_status.pCon));
    wattroff(m_status.pCon, col);
}

enum color::curses 
CursesUI::drawVolume()
{
    auto volumeStr = FMT("volume: {:3.0f}%\n", 100.0 * m_p->m_volume);
    int maxx = getmaxx(m_status.pCon);

    long maxWidth = maxx - volumeStr.size() - 1;
    f64 maxLine = (m_p->m_volume * (f64)maxWidth) * (1.0 - (defaults::maxVolume - 1.0));

    auto getColor = [&](f64 i) -> int {
        f64 val = m_p->m_volume * (i / (maxLine));

        if (val > 1.01) return color::curses::red;
        else if (val > 0.51) return color::curses::yellow;
        else return color::curses::green;
    };

    auto mutedColor = COLOR_PAIR(defaults::mutedColor);
    int sCol = m_p->m_bMuted ? mutedColor : (A_BOLD | COLOR_PAIR(getColor(maxLine)));
    wattron(m_status.pCon, sCol);
    mvwaddnstr(m_status.pCon, 1, 0, volumeStr.data(), maxx);
    wattroff(m_status.pCon, sCol);

    for (long i = 0; i < maxLine; i++)
    {
        int color;
        const wchar_t* icon;
        if (m_p->m_bMuted)
        {
            color = mutedColor;
            icon = blockIcon2;
        }
        else
        {
            color = COLOR_PAIR(getColor(i));
            icon = blockIcon1;
        }

        wattron(m_status.pCon, color);
        mvwaddwstr(m_status.pCon, 1, 1 + i + volumeStr.size(), icon);
        wattroff(m_status.pCon, color);
    }

    return (enum color::curses)sCol;
}

void
CursesUI::drawPlayListCounter()
{
    auto songCounterStr = FMT("total: {} / {}", m_p->m_currSongIdx + 1, m_p->m_songs.size());

    if (m_p->m_eRepeat != repeatMethod::none)
        songCounterStr += FMT(" (repeat {})", repeatMethodStrings[(int)m_p->m_eRepeat]);

    auto col = COLOR_PAIR(color::white);
    wattron(m_status.pCon, col);
    mvwaddnstr(m_status.pCon, 3, 0, songCounterStr.data(), getmaxx(m_status.pCon));
    wattroff(m_status.pCon, col);
}

void
CursesUI::drawTitle()
{
    auto ls = "playing: " + m_p->m_info.title;
    ls.resize(getmaxx(m_pl.pBor) - 1);

    move(5, 0);
    clrtoeol();
    attron(A_BOLD | A_ITALIC | COLOR_PAIR(color::curses::yellow));
    mvaddstr(5, 1, ls.data());
    attroff(A_BOLD | A_ITALIC | COLOR_PAIR(color::curses::yellow));
}

void
CursesUI::drawPlayList()
{
    long maxy = getmaxy(m_pl.pCon);
    long startFromY = 0; /* offset from border */

    adjustListToPosition();

    werase(m_pl.pBor);
    for (long i = m_firstInList; i < (long)m_p->m_songs.size() && startFromY < maxy; i++, startFromY++)
    {
        auto lineStr = utils::removePath(m_p->m_songs[i]);
        auto col = COLOR_PAIR(color::white);
        wattron(m_pl.pCon, col);

        if (i == m_selected)
            wattron(m_pl.pCon, col | A_REVERSE);
        if (i == m_p->m_currSongIdx)
            wattron(m_pl.pCon, A_BOLD | COLOR_PAIR(color::curses::yellow));

        mvwaddnstr(m_pl.pCon, startFromY, getbegx(m_pl.pCon), lineStr.data(), getmaxx(m_pl.pCon) - 1);
        wattroff(m_pl.pCon, col | A_REVERSE | A_BOLD | COLOR_PAIR(color::curses::yellow));
    }

    drawBorders(m_pl.pBor);
}

void
CursesUI::drawBottomLine()
{
    int maxy = getmaxy(stdscr);
    move(maxy - 1, 0);
    clrtoeol();
    move(maxy - 1, 1);

    auto col = COLOR_PAIR(color::white);
    attron(col);

    if (!m_p->m_searchingNow.empty() && !m_p->m_foundIndices.empty())
    {
        auto ss = FMT(" [{}/{}]", m_p->m_currFoundIdx + 1, m_p->m_foundIndices.size());
        auto s = L"'" + m_p->m_searchingNow + L"'" + std::wstring(ss.begin(), ss.end());
        addwstr(s.data());
    }

    /* draw selected index */
    auto sel = FMT("{}\n", m_p->m_term.m_selected + 1);

    mvaddstr(maxy - 1, (getmaxx(stdscr) - 1) - sel.size(), sel.data());
    wattroff(m_status.pCon, col);
}

void
CursesUI::drawInfo()
{
    int maxx = getmaxx(m_info.pCon);
    constexpr std::string_view sTitle = "title: ";
    constexpr std::string_view sAlbum = "album: ";
    constexpr std::string_view sArtist = "artist: ";

    werase(m_info.pBor);

    auto col = COLOR_PAIR(color::white);

    wattron(m_info.pCon, col);
    mvwaddnstr(m_info.pCon, 0, 0, sTitle.data(), maxx*2);
    wattron(m_info.pCon, A_BOLD | A_ITALIC | COLOR_PAIR(color::curses::yellow));
    mvwaddnstr(m_info.pCon, 0, sTitle.size(), m_p->m_info.title.data(), (maxx*2) - sTitle.size());
    wattroff(m_info.pCon, A_BOLD | A_ITALIC | COLOR_PAIR(color::curses::yellow));

    wattron(m_info.pCon, col);
    mvwaddnstr(m_info.pCon, 2, 0, sAlbum.data(), maxx);
    wattron(m_info.pCon, A_BOLD | col);
    mvwaddnstr(m_info.pCon, 2, sAlbum.size(), m_p->m_info.album.data(), maxx - sAlbum.size());
    wattroff(m_info.pCon, A_BOLD | col);

    wattron(m_info.pCon, col);
    mvwaddnstr(m_info.pCon, 3, 0, sArtist.data(), maxx);
    wattron(m_info.pCon, A_BOLD | col);
    mvwaddnstr(m_info.pCon, 3, sArtist.size(), m_p->m_info.artist.data(), maxx - sArtist.size());
    wattroff(m_info.pCon, A_BOLD | col);

    drawBorders(m_info.pBor);
}

void
CursesUI::drawStatus()
{
    werase(m_status.pBor);

    drawTime();
    auto color = drawVolume();
    drawPlayListCounter();

    drawBorders(m_status.pBor, (color::curses)PAIR_NUMBER(color));
}

void
CursesUI::drawVisualizer()
{
    werase(m_vis.pBor);

    int maxy = getmaxy(m_vis.pCon), maxx = getmaxx(m_vis.pCon);
    int nChannels = m_p->m_pw.channels;
    int lastNFrames = m_p->m_pw.lastNFrames; /* lastChunkSize == lastNFrames * nChannels */

    std::vector<u32> bars(maxx);
    f32* pChunk = m_p->m_chunk;
    f32 accSize = (f32)lastNFrames / (f32)bars.size();
    long chunkPos = 0;

    std::valarray<std::complex<f32>> aFreqDomain(lastNFrames);
    for (size_t i = 0, j = 0; i < aFreqDomain.size(); i++, j += nChannels)
        aFreqDomain[i] = std::complex(pChunk[j], 0.0f);

    utils::fft(aFreqDomain);
    if (aFreqDomain.size() > 0)
        aFreqDomain[0] = std::complex(0.0f, 0.0f);

    f32 maxAmp = 0.0f;
    for (auto& e : aFreqDomain)
    {
        auto max = std::max(e.imag(), e.real());
        if (max > maxAmp) maxAmp = max;
    }

    for (auto& e : aFreqDomain)
    {
        e.real(e.real() / maxAmp);
        e.imag(e.imag() / maxAmp);
    }

    long reducedAcc = std::ceil((f32)accSize / 6.0f); /* TODO: make some sort of non-linear sections, this simply cuts off most of frequencies */

    for (long i = 0; i < (long)bars.size(); i++)
    {
        f32 acc = 0;
        for (long j = 0; j < reducedAcc; j++)
        {
            auto real = aFreqDomain[chunkPos].real();
            auto imag = aFreqDomain[chunkPos].imag();
            auto newAcc = std::max(real, imag);
            if (newAcc > acc) acc = newAcc;

            chunkPos++;
        }

        u32 h = std::round(acc * defaults::visualizerScalar); /* scale by some number to boost the bars heights */
        if (h > (u32)maxy) h = maxy;
        bars[i] = h;
    }

    for (int r = 0; r < maxx; r++)
    {
        int maxHeight = bars[r];

        /* draw from bottom (maxy) upwards */
        for (int c = maxy - 1; c >= maxy - maxHeight; c--)
        {
            short color = COLOR_PAIR(defaults::barHeightColors[c]);

            wattron(m_vis.pCon, color);
            mvwaddwstr(m_vis.pCon, c, r, defaults::visualizerSymbol);
            wattroff(m_vis.pCon, color);
        }
    }
}

void
CursesUI::adjustListToPosition()
{
    long maxy = getmaxy(m_pl.pCon);

    if (((long)(m_p->m_songs.size() - 1) - m_firstInList) < (maxy - 1))
        m_firstInList = (m_p->m_songs.size() - 1) - (maxy - 2);
    if ((long)m_p->m_songs.size() < maxy - 1)
        m_firstInList = 0;
    else if (m_firstInList < 0)
        m_firstInList = 0;

    long listSizeBound = m_firstInList + (maxy - 1);

    if (m_selected > listSizeBound - 1)
        m_firstInList = m_selected - (maxy - 2);
    else if (m_selected < m_firstInList)
        m_firstInList = m_selected;
}

PipeWirePlayer::PipeWirePlayer(int argc, char** argv)
{
    m_term.m_p = this;
    m_term.resizeWindows();

    pw_init(&argc, &argv);

    m_term.m_firstInList = 0;

    for (int i = 1; i < argc; i++)
    {
        std::string s = argv[i];

        for (auto& f : m_supportedFormats)
            if (s.ends_with(f))
            {
                m_songs.push_back(std::move(s));
                break;
            }
    }

    if (m_songs.empty())
    {
        m_bFinished = true;
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
    u8 setupBuffer[1024] {};
    const spa_pod* params[1] {};
    spa_pod_builder b = SPA_POD_BUILDER_INIT(setupBuffer, sizeof(setupBuffer));

    m_pw.pLoop = pw_main_loop_new(nullptr);

    m_pw.pStream = pw_stream_new_simple(pw_main_loop_get_loop(m_pw.pLoop),
                                      "kmpStream",
                                      pw_properties_new(PW_KEY_MEDIA_TYPE, "Audio",
                                                        PW_KEY_MEDIA_CATEGORY, "Playback",
                                                        PW_KEY_MEDIA_ROLE, "Music",
                                                        nullptr),
                                      &m_pw.streamEvents,
                                      this);


    spa_audio_info_raw rawInfo {
        .format = format,
        .flags {},
        .rate = sampleRate,
        .channels = channels,
        .position {}
    };

    params[0] = spa_format_audio_raw_build(&b, SPA_PARAM_EnumFormat, &rawInfo);

    pw_stream_connect(m_pw.pStream,
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
    while (!m_bFinished)
    {
#ifdef MPRIS_LIB
        /* doing it here effectively raises kmp for playerctl without actually implementing raise method
         * https://specifications.freedesktop.org/mpris-spec/latest/Media_Player.html#Method:Raise */
        mpris::init(this);
#endif

        playCurrent();

#ifdef MPRIS_LIB
        mpris::clean();
#endif
    }
}

void
PipeWirePlayer::playCurrent()
{
    m_hSnd = SndfileHandle(currSongName().data(), SFM_READ);

    /* skip song on error */
    if (m_hSnd.error() == 0)
    {
        m_pw.eformat = SPA_AUDIO_FORMAT_F32;
        m_pw.sampleRate = m_hSnd.samplerate();
        m_pw.channels = m_hSnd.channels();
        m_pw.origSampleRate = m_pw.sampleRate;

        m_pcmPos = 0;
        m_pcmSize = m_hSnd.frames() * m_pw.channels;

        m_info = song::Info(currSongName(), m_hSnd);

        /* restore speed multiplier */
        m_pw.sampleRate *= m_speedMul;
        setupPlayer(m_pw.eformat, m_pw.sampleRate, m_pw.channels);

        m_term.updateAll();
        m_term.drawUI();

        if (!m_ready)
            refresh(); /* refresh before first getch update */
        m_ready = true;

        /* TODO: there is probably a better way to update params than just to reset the whole thing */
updateParams:
        if (m_bChangeParams)
        {
            m_bChangeParams = false;
            setupPlayer(m_pw.eformat, m_pw.sampleRate, m_pw.channels);
            m_term.resizeWindows();
            m_term.drawUI();
        }

        pw_main_loop_run(m_pw.pLoop);

        /* in this order */
        pw_stream_destroy(m_pw.pStream);
        pw_main_loop_destroy(m_pw.pLoop);

        if (m_bPaused)
        {
            /* let input thread wake up cndPause */
            m_mtxPauseSwitch.unlock();

            std::unique_lock lock(m_mtxPause);
            m_cndPause.wait(lock);
            m_bChangeParams = true;
        }

        if (m_bChangeParams)
            goto updateParams;
    }
    else
    {
        m_bNext = true;
    }

    if (m_bNewSongSelected)
    {
        m_bNewSongSelected = false;
        m_currSongIdx = m_term.m_selected;
    }
    else if (m_bNext)
    {
        m_bNext = false;
        m_currSongIdx++;
        if (m_currSongIdx > (long)m_songs.size() - 1)
            m_currSongIdx = 0;
    }
    else if (m_bPrev)
    {
        m_bPrev = false;
        m_currSongIdx--;
        if (m_currSongIdx < 0)
            m_currSongIdx = m_songs.size() - 1;
    }
    else
    {
        if (m_eRepeat != repeatMethod::track) m_currSongIdx++;
    }

    if (m_currSongIdx > (long)m_songs.size() - 1)
    {
        if (m_eRepeat == repeatMethod::playlist)
            m_currSongIdx = 0;
        else
            m_bFinished = true;
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
        m_searchingNow = (wchar_t*)wb;
    else
        return false;

    if (!m_searchingNow.empty())
    {
        m_foundIndices = search::getIndexList(m_songs, m_searchingNow, direction);
        m_currFoundIdx = 0;

        return true;
    }

    return false;
}

void
PipeWirePlayer::jumpToFound(enum search::dir direction)
{
    if (!m_foundIndices.empty())
    {
        int next = direction == search::dir::forward ? 1 : -1;
        m_currFoundIdx += next;

        if (m_currFoundIdx > (long)m_foundIndices.size() - 1)
            m_currFoundIdx = 0;
        else if (m_currFoundIdx < 0)
            m_currFoundIdx = m_foundIndices.size() - 1;

        m_term.m_selected = m_foundIndices[m_currFoundIdx];
        centerOn(m_term.m_selected);
    }
}

void
PipeWirePlayer::centerOn(size_t i)
{
    select(i);
    m_term.m_firstInList = (m_term.m_selected - (m_term.playListMaxY() - 3) / 2);
}

void
PipeWirePlayer::setSeek(f64 value)
{
    std::lock_guard lock(m_pw.mtx);

    value = std::clamp(value, 0.0, getMaxTimeInSec());

    value *= m_pw.sampleRate;
    m_hSnd.seek(value, SEEK_SET);
    m_pcmPos = m_hSnd.seek(0, SEEK_CUR) * m_pw.channels;
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
        long num = wcstol((wchar_t*)wb, &end, std::size(wb));
        num = std::clamp((long)num, 1L, (long)m_songs.size());
        m_term.m_selected = num - 1;
    }
}

void
PipeWirePlayer::pause()
{
    std::lock_guard lock(m_mtxPauseSwitch);
    m_bPaused = true;
}

void
PipeWirePlayer::resume()
{
    std::lock_guard lock(m_mtxPauseSwitch);
    m_bPaused = false;
    m_cndPause.notify_one();
}

void
PipeWirePlayer::togglePause()
{
    std::lock_guard lock(m_mtxPauseSwitch);
    m_bPaused = !m_bPaused;
    if (!m_bPaused) m_cndPause.notify_one();
}

void
PipeWirePlayer::cycleRepeatMethods(int i)
{
    assert((i == -1 || i == 1) && "wrong i");

    auto m = (int)m_eRepeat + i;
    if (m >= (int)repeatMethod::size)
        m = 0;
    else if (m < 0)
        m = (int)repeatMethod::size - 1;

    setRepeatMethod((enum repeatMethod)m);
}

void
PipeWirePlayer::setVolume(f64 vol)
{
    m_volume = std::clamp(vol, defaults::minVolume, defaults::maxVolume);
}

void
PipeWirePlayer::addSampleRate(long val)
{
    long nSr = (f64)m_pw.sampleRate + val;
    nSr = std::clamp(nSr, defaults::minSampleRate, defaults::maxSampleRate);

    m_pw.sampleRate = nSr;
    m_speedMul = (f64)m_pw.sampleRate / (f64)m_pw.origSampleRate;
    m_bChangeParams = true;
}

void
PipeWirePlayer::restoreOrigSampleRate()
{
    m_pw.sampleRate = m_pw.origSampleRate;
    m_speedMul = 1.0;
    m_bChangeParams = true;
}

void
PipeWirePlayer::finish()
{
    m_bFinished = true;
    if (m_bPaused)
    {
        m_bPaused = false;
        m_cndPause.notify_all();
    }
}

void
PipeWirePlayer::select(long pos, bool bWrap)
{
    bool bWrapOverride = (bWrap == defaults::bWrapSelection ? m_bWrapSelection : bWrap);

    if (pos > (long)m_songs.size() - 1)
    {
        if (bWrapOverride) pos = 0;
        else pos = m_songs.size() - 1;
    }
    else if (pos < 0)
    {
        if (bWrapOverride) pos = m_songs.size() - 1;
        else pos = 0;
    }
    m_term.m_selected = m_selected = pos;
}

void
PipeWirePlayer::playSelected()
{
    m_currSongIdx = m_term.m_selected;
    m_bNewSongSelected = true;
}

} /* namespace app */
