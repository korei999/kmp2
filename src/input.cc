#include "input.hh"
#include "utils.hh"

#include <ncurses.h>

void
input::read(app::PipeWirePlayer* p)
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
                p->term.update.bPlayList = true;
                break;

            case 44:
            case '?':
                p->subStringSearch(search::dir::backwards);
                p->term.update.bPlayList = true;
                break;

            case 'n':
                p->jumpToFound(search::dir::forward);
                p->term.update.bPlayList = true;
                break;

            case 'N':
                p->jumpToFound(search::dir::backwards);
                p->term.update.bPlayList = true;
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
                p->term.selected = p->currSongIdx;
                p->term.firstInList = (p->term.selected - (p->term.getMaxY() - 3) / 2);
                p->term.update.bPlayList = true;
                break;

            case KEY_RESIZE:
            case 12: /* C-l */
                p->term.resizePlayListWindow();
                p->term.updateAll();
                redrawwin(p->term.pStd);
                break;

            case -1:
                break;

            default:
#ifndef NDEBUG
                CERR("pressed: '{}'\n", c);
#endif
                break;
        }

        p->term.update.bTime = true;

        p->term.drawUI();
    }
}
