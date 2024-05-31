#ifdef MPRIS
#include "mpris.hh"
#endif
#include "input.hh"
#include "defaults.hh"
#ifndef NDEBUG
#include "utils.hh"
#endif

#include <ncurses.h>

namespace input
{

void
read(app::PipeWirePlayer* p)
{
    /* avoid possible multiple `refresh()` at start */
    while (!p->m_ready) {};

    int c;

    auto search = [p](enum search::dir d) -> void {
        bool succes = p->subStringSearch(d);
        if (!p->m_foundIndices.empty() && succes)
        {
            p->select(p->m_foundIndices[p->m_currFoundIdx]);
            p->m_term.updatePlayList();
            p->m_term.updateBottomLine();
            p->centerOn(p->m_selected);
        }
    };

    auto seek = [&]() -> void {
        std::lock_guard lock(p->m_pw.mtx);

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
            p->m_hSnd.seek(step, SEEK_CUR);
            p->m_term.updateStatus();
            p->m_term.updateBottomLine();
            p->m_pcmPos = p->m_hSnd.seek(0, SEEK_CUR) * p->m_pw.channels;
            p->m_term.drawUI();
            c = getch();
        }
        timeout(defaults::updateRate);
    };

    while ((c = getch()))
    {
        f64 newVol = p->m_volume;
        switch (c)
        {
            case 'q':
                p->finish();
                return;
                break;

            case '0':
                newVol += 0.04;
                [[fallthrough]];
            case ')':
                newVol += 0.01;
                p->setVolume(newVol);
                break;

            case '9':
                newVol -= 0.04;
                [[fallthrough]];
            case '(':
                newVol -= 0.01;
                p->setVolume(newVol);
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
                p->next();
                break;

            case 'i':
                p->prev();
                break;

            case KEY_DOWN:
            case 'j':
                p->selectNext();
                p->m_term.updatePlayList();
                p->m_term.updateBottomLine();
                break;

            case KEY_UP:
            case 'k':
                p->selectPrev();
                p->m_term.updatePlayList();
                p->m_term.updateBottomLine();
                break;

            case 'g':
                p->selectFirst();
                p->m_term.updatePlayList();
                p->m_term.updateBottomLine();
                break;

            case 'G':
                p->selectLast();
                p->m_term.updatePlayList();
                p->m_term.updateBottomLine();
                break;

            case KEY_NPAGE:
            case 4: /* C-d */
            case 6: /* C-f */
                p->select(p->m_selected + 22, false);
                p->m_term.updatePlayList();
                p->m_term.updateBottomLine();
                break;

            case KEY_PPAGE:
            case 2: /* C-b */
            case 21: /* C-u */
                p->select(p->m_selected - 22, false);
                p->m_term.updatePlayList();
                p->m_term.updateBottomLine();
                break;

            case 46:
            case '/':
                search(search::dir::forward);
                p->m_term.updateBottomLine();
                break;

            case 44:
            case '?':
                search(search::dir::backwards);
                p->m_term.updateBottomLine();
                break;

            case 'n':
                p->jumpToFound(search::dir::forward);
                p->m_term.updatePlayList();
                p->m_term.updateBottomLine();
                break;

            case 'N':
                p->jumpToFound(search::dir::backwards);
                p->m_term.updatePlayList();
                p->m_term.updateBottomLine();
                break;

            case 'r':
                p->toggleRepeatAfterLast();
                break;

            case ' ':
                p->togglePause();
                break;

            case '\n':
                p->playSelected();
                p->m_term.updateAll();
                break;
                
            case 'z':
            case 'Z':
                p->centerOn(p->m_currSongIdx);
                p->m_term.updatePlayList();
                p->m_term.updateBottomLine();
                break;

            case 't':
                p->setSeek();
                p->m_term.updateBottomLine();
                break;

            case ':':
                p->jumpTo();
                p->m_term.updatePlayList();
                break;

            case 'm':
                p->toggleMute();
                break;

            case KEY_RESIZE:
            case 12: /* C-l */
                p->m_term.resizeWindows();
                break;

            case '[':
                p->addSampleRate(-1000);
                break;

            case '{':
                p->addSampleRate(-100);
                break;

            case ']':
                p->addSampleRate(1000);
                break;

            case '}':
                p->addSampleRate(100);
                break;

            case 'v':
            case 'V':
                p->m_term.toggleVisualizer();
                break;

            case '\\':
                p->restoreOrigSampleRate();
                break;

            case ERR:
                break;

            default:
#ifndef NDEBUG
                CERR("pressed: '{}'\n", c);
#endif
                break;
        }

#ifdef MPRIS
        mpris::process(p);
#endif

        p->m_term.updateStatus();
        if (!p->m_bPaused) p->m_term.updateVisualizer();

        p->m_term.drawUI();
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
    if (ts.empty() || !std::isdigit(ts[0]))
        return std::nullopt;

    u64 ret = 0;
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
        {
            numbers.push_back(getNumber());
        }
        else if (c == L':')
        {
            i++;
        }
        else if (c == L'%')
        {
            percent = true;
            break;
        }
        else
        {
            i++;
        }
    }

    constexpr int mul[2] {60, 1};
    wchar_t* end;

    if (numbers.size() == 1)
    {
        ret = wcstoul(numbers[0].data(), &end, 10);

        if (percent)
            ret = (p->m_pcmSize * ((f64)ret/100.0)) / p->m_pw.channels / p->m_pw.sampleRate;
    }
    else
    {
        for (size_t i = 0; i < 2; i++)
            ret += wcstoul(numbers[i].data(), &end, 10) * (mul[i]);
    }

    return ret;
}

} /* namespace input */
