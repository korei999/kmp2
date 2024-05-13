#include "input.hh"
#include "defaults.hh"
#ifndef NDEBUG
#include "utils.hh"
#endif

#include <ncurses.h>
#include <thread>

namespace input
{

[[maybe_unused]] static void
drawVisualizer(app::PipeWirePlayer* p)
{
    while (!p->bFinished)
    {
        if (p->bPaused || !p->bDrawVisualizer)
        {
            std::unique_lock lock(p->mtxPause);
            p->cndPause.wait(lock);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(defaults::visualizerUpdateRate));

        if (!p->bPaused && p->bDrawVisualizer)
        {
            std::lock_guard lock(p->term.mtx);

            p->term.drawVisualizer();
            wrefresh(p->term.vis.pCon);
        }
    }
}

void
read(app::PipeWirePlayer* p)
{
    int c;

    auto search = [p](enum search::dir d) -> void {
        bool succes = p->subStringSearch(d);
        if (!p->foundIndices.empty() && succes)
        {
            p->term.selected = p->foundIndices[p->currFoundIdx];
            p->term.update.bPlayList = true;
            p->term.update.bBottomLine = true;
            p->centerOn(p->term.selected);
        }
    };

    auto seek = [&]() -> void {
        std::lock_guard lock(p->pw.mtx);

        int step;
        int key0;
        int key1;

        if (c == 'l' || c == KEY_RIGHT)
        {
            step = defaults::step;
            key0 = 'l';
            key1 = KEY_RIGHT;
        }
        else
        {
            step = -defaults::step;
            key0 = 'h';
            key1 = KEY_LEFT;
        }

        timeout(50);
        while (c == key0 || c == key1)
        {
            p->hSnd.seek(step, SEEK_CUR);
            p->term.update.bStatus = true;
            p->term.update.bBottomLine = true;
            p->pcmPos = p->hSnd.seek(0, SEEK_CUR) * p->pw.channels;
            p->term.drawUI();
            c = getch();
        }
        timeout(defaults::updateRate);
    };

    auto addSampleRate = [&](long val) -> void {
        long nSr = (f64)p->pw.sampleRate + val;
        if (nSr < 1000) nSr = 1000;

        p->pw.sampleRate = nSr;
        p->speedMul = (f64)p->pw.sampleRate / (f64)p->pw.origSampleRate;
        p->bChangeParams = true;
    };

    while ((c = getch()))
    {
        switch (c)
        {
            case 'q':
                p->bFinished = true;
                if (p->bPaused)
                {
                    p->bPaused = false;
                    p->cndPause.notify_one();
                }
                return;
                break;

            case '0':
                p->volume += 0.04;
                [[fallthrough]];
            case ')':
                p->volume += 0.01;
                if (p->volume > defaults::maxVolume)
                    p->volume = defaults::maxVolume;
                break;

            case '9':
                p->volume -= 0.04;
                [[fallthrough]];
            case '(':
                p->volume -= 0.01;
                if (p->volume < defaults::minVolume)
                    p->volume = defaults::minVolume;
                break;

            case KEY_RIGHT:
            case 'l':
                seek();
                break;

            case KEY_LEFT:
            case 'h':
                seek();
                break;

            case 'o':
                p->bNext = true;
                break;

            case 'i':
                p->bPrev = true;
                break;

            case KEY_DOWN:
            case 'j':
                {
                    auto newSel = p->term.selected + 1;
                    if (newSel > (long)p->songs.size() - 1)
                    {
                        if (p->bWrapSelection)
                            newSel = 0;
                        else
                            newSel = p->songs.size() - 1;
                    }
                    p->term.selected = newSel;
                    p->term.update.bPlayList = true;
                    p->term.update.bBottomLine = true;
                }
                break;

            case KEY_UP:
            case 'k':
                {
                    auto newSel = p->term.selected - 1;
                    if (newSel < 0)
                    {
                        if (p->bWrapSelection) newSel = p->songs.size() - 1;
                        else newSel = 0;
                    }
                    p->term.selected = newSel;
                    p->term.update.bPlayList = true;
                    p->term.update.bBottomLine = true;
                }
                break;

            case 'g':
                p->term.selected = 0;
                p->term.update.bPlayList = true;
                p->term.update.bBottomLine = true;
                break;

            case 'G':
                p->term.selected = p->songs.size() - 1;
                p->term.update.bPlayList = true;
                p->term.update.bBottomLine = true;
                break;

            case KEY_NPAGE:
            case 4: /* C-d */
            case 6: /* C-f */
                {
                    long newSel = p->term.selected + 22;
                    if (newSel >= (long)p->songs.size())
                        newSel = p->songs.size() - 1;

                    p->term.selected = newSel;
                    p->term.update.bPlayList = true;
                    p->term.update.bBottomLine = true;
                }
                break;

            case KEY_PPAGE:
            case 2: /* C-b */
            case 21: /* C-u */
                {
                    long newSel = p->term.selected - 22;
                    if (newSel < 0) newSel = 0;

                    p->term.selected = newSel;
                    p->term.update.bPlayList = true;
                    p->term.update.bBottomLine = true;
                }
                break;

            case 46:
            case '/':
                search(search::dir::forward);
                p->term.update.bBottomLine = true;
                break;

            case 44:
            case '?':
                search(search::dir::backwards);
                p->term.update.bBottomLine = true;
                break;

            case 'n':
                p->jumpToFound(search::dir::forward);
                p->term.update.bPlayList = true;
                p->term.update.bBottomLine = true;
                break;

            case 'N':
                p->jumpToFound(search::dir::backwards);
                p->term.update.bPlayList = true;
                p->term.update.bBottomLine = true;
                break;

            case 'r':
                p->bRepeatAfterLast = !p->bRepeatAfterLast;
                break;

            case ' ':
                p->bPaused = !p->bPaused;
                if (!p->bPaused)
                    p->cndPause.notify_all();
                break;

            case '\n':
                p->currSongIdx = p->term.selected;
                p->bNewSongSelected = true;
                p->term.updateAll();
                break;
                
            case 'z':
                p->centerOn(p->currSongIdx);
                p->term.update.bPlayList = true;
                p->term.update.bBottomLine = true;
                break;

            case 't':
                p->setSeek();
                p->term.update.bBottomLine = true;
                break;

            case ':':
                p->jumpTo();
                p->term.update.bPlayList = true;
                break;

            case 'm':
                p->bMuted = !p->bMuted;
                break;

            case KEY_RESIZE:
            case 12: /* C-l */
                p->term.resizeWindows();
                break;

            case '[':
                addSampleRate(-1000);
                break;

            case '{':
                addSampleRate(-100);
                break;

            case ']':
                addSampleRate(1000);
                break;

            case '}':
                addSampleRate(100);
                break;

            case 'v':
            case 'V':
                p->bDrawVisualizer = !p->bDrawVisualizer;
                p->term.resizeWindows();
                p->term.update.bPlayList = true;
                break;

            case '\\':
                {
                    std::lock_guard lock(p->pw.mtx);
                    p->pw.sampleRate = p->pw.origSampleRate;
                    p->speedMul = 1.0;
                    p->bChangeParams = true;
                }
                break;

            case ERR:
                break;

            default:
#ifndef NDEBUG
                CERR("pressed: '{}'\n", c);
#endif
                break;
        }

        p->term.update.bStatus = true;
        if (!p->bPaused) p->term.update.bVisualizer = true;

        p->term.drawUI();
    }
}

void
readWString(std::wstring_view prefix, wint_t* pBuff, int buffSize)
{
    auto displayString = [&](bool curs) -> void {
        int maxy = getmaxy(stdscr);
        move(maxy - 1, 0);
        clrtoeol();
        addwstr(prefix.data());
        mvaddwstr(maxy - 1, prefix.size(), (wchar_t*)pBuff);
        if (curs)
            mvaddwstr(maxy - 1, prefix.size() + wcsnlen((wchar_t*)pBuff, buffSize), app::blockIcon0);
    };

    displayString(true);

    wint_t wc = 0;
    int i = 0;
    while (get_wch(&wc) != ERR)
    {
        switch (wc)
        {
            case '\n':
                goto done;
                break;

            case 27: /* escape */
                memset(pBuff, '\0', buffSize * sizeof(*pBuff));
                goto done;
                break;

            case 23: /* C-w */
                memset(pBuff, '\0', buffSize * sizeof(*pBuff));
                i = 0;
                break;

            case '\t':
                /* ignore tabs */
                pBuff[i] = '\0';
                break;

            case 263:
            case '\b':
                if (--i < 0) i = 0;

                pBuff[i] = '\0';
                break;

            default:
                pBuff[i++] = wc;
                if (i >= buffSize) i = buffSize - 1;
                break;
        }

        pBuff[buffSize - 1] = '\0';
        displayString(true);
    }

done:
    displayString(false);
}

std::optional<u64>
parseTimeString(std::wstring_view ts, app::PipeWirePlayer* p)
{
    u64 ret = 0;

    if (ts.empty() || !std::isdigit(ts[0]))
        return std::nullopt;

    std::vector<std::wstring> numbers {};

    bool percent = false;

    int i = 0;
    wchar_t c;

    auto getNumber = [&]() -> std::wstring {
        std::wstring ret;

        while (i < (int)ts.size() && std::isdigit(ts[i]))
            ret.push_back(ts[i++]);

        return ret;
    };

    while (i < (int)ts.size() && (c = ts[i]) && numbers.size() < 2)
    {
        if (std::isdigit(c))
            numbers.push_back(getNumber());
        else if (c == L':')
            i++;
        else if (c == L'%')
        {
            percent = true;
            break;
        }
        else
            i++;
    }

    constexpr int mul[2] {60, 1};
    wchar_t* end;

    if (numbers.size() == 1)
    {
        ret = wcstoul(numbers[0].data(), &end, 10);

        if (percent)
            ret = (p->pcmSize * ((f64)ret/100.0)) / p->pw.channels / p->pw.sampleRate;
    }
    else
    {
        for (size_t i = 0; i < 2; i++)
            ret += wcstoul(numbers[i].data(), &end, 10) * (mul[i]);
    }

    return ret;
}

} /* namespace input */
