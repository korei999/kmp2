#include "input.hh"
#include "utils.hh"

#include <ncurses.h>

namespace input
{

void
read(app::PipeWirePlayer* p)
{
    int c;

    while ((c = getch()))
    {
        switch (c)
        {
            case 'q':
                if (p->bPaused)
                {
                    p->bPaused = false;
                    p->pauseCnd.notify_all();
                }
                p->bFinished = true;
                return;
                break;

            case '0':
                p->volume += 0.04;
                [[fallthrough]];
            case ')':
                p->volume += 0.01;
                if (p->volume > app::def::maxVolume)
                    p->volume = app::def::maxVolume;
                p->term.update.bVolume = true;
                break;

            case '9':
                p->volume -= 0.04;
                [[fallthrough]];
            case '(':
                p->volume -= 0.01;
                if (p->volume < app::def::minVolume)
                    p->volume = app::def::minVolume;
                p->term.update.bVolume = true;
                break;

            case KEY_RIGHT:
            case 'l':
                {
                    std::lock_guard lock(p->pw.mtx);

                    timeout(50);
                    while (c == 'l' || c == KEY_RIGHT)
                    {
                        p->hSnd.seek(app::def::step, SEEK_CUR);
                        p->term.update.bTime = true;
                        p->pcmPos = p->hSnd.seek(0, SEEK_CUR) * p->pw.channels;
                        p->term.drawUI();
                        c = getch();
                    }
                    timeout(1000);
                }
                break;

            case KEY_LEFT:
            case 'h':
                {
                    std::lock_guard lock(p->pw.mtx);

                    timeout(50);
                    while (c == 'h' || c == KEY_LEFT)
                    {
                        p->hSnd.seek(-app::def::step, SEEK_CUR);
                        p->term.update.bTime = true;
                        p->pcmPos = p->hSnd.seek(0, SEEK_CUR) * p->pw.channels;
                        p->term.drawUI();
                        c = getch();
                    }
                    timeout(1000);
                }
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
                }
                break;

            case 'g':
                p->term.selected = 0;
                p->term.update.bPlayList = true;
                break;

            case 'G':
                p->term.selected = p->songs.size() - 1;
                p->term.update.bPlayList = true;
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
                }
                break;

            case 46:
            case '/':
                p->subStringSearch(search::dir::forward);
                if (!p->foundIndices.empty())
                    p->term.selected = p->foundIndices[p->currFoundIdx];
                p->term.update.bPlayList = true;
                p->term.update.bBottomLine = true;
                break;

            case 44:
            case '?':
                p->subStringSearch(search::dir::backwards);
                if (!p->foundIndices.empty())
                    p->term.selected = p->foundIndices[p->currFoundIdx];
                p->term.update.bPlayList = true;
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
                p->term.update.bSongName = true;
                break;

            case ' ':
                p->bPaused = !p->bPaused;
                if (!p->bPaused)
                    p->pauseCnd.notify_one();
                break;

            case '\n':
                p->currSongIdx = p->term.selected;
                p->bNewSongSelected = true;
                p->term.updateAll();
                break;
                
            case 'z':
                p->centerOn(p->currSongIdx);
                p->term.update.bPlayList = true;
                break;

            case '`':
            case 39:
            case 188:
                p->setSeek();
                break;

            case ':':
                p->jumpTo();
                p->term.update.bPlayList = true;
                break;

            case KEY_RESIZE:
            case 12: /* C-l */
                p->term.resizePlayListWindow();
                p->term.updateAll();
                redrawwin(stdscr);
                break;

            case ERR:
                break;

            default:
#ifndef NDEBUG
                CERR("pressed: '{}'\n", c);
#endif
                break;
        }

        /* TODO: each 1000ms time updates are not actually accurate */
        p->term.update.bTime = true;
        p->term.drawUI();
    }
}

void
readWString(std::wstring_view prefix, wint_t* pBuff, int buffSize)
{
    auto displayString = [&](bool curs) -> void
    {
        int maxy = getmaxy(stdscr);
        move(maxy - 1, 0);
        clrtoeol();
        addwstr(prefix.data());
        mvaddwstr(maxy - 1, prefix.size(), (wchar_t*)pBuff);
        if (curs)
            mvaddwstr(maxy - 1, prefix.size() + wcsnlen((wchar_t*)pBuff, buffSize), app::blockIcon);
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

    if (!std::isdigit(ts[0]))
        return std::nullopt;

    std::vector<std::wstring> numbers {};
    size_t ni = 0;

    [[maybe_unused]] bool percent = false;

    int i = 0;
    wchar_t c;

    auto getNumber = [&]() -> std::wstring {
        std::wstring ret;

        while (std::isdigit(ts[i]))
            ret.push_back(ts[i++]);

        return ret;
    };

    while ((c = ts[i]) && i < (int)ts.size() && numbers.size() < 2)
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
