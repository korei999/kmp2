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
                if (p->paused)
                {
                    p->paused = false;
                    p->pauseCnd.notify_all();
                }
                p->finished = true;
                return;
                break;

            case '0':
                p->volume += 0.04;
                [[fallthrough]];
            case ')':
                p->volume += 0.01;
                if (p->volume > app::def::maxVolume)
                    p->volume = app::def::maxVolume;
                p->term.update.volume = true;
                break;

            case '9':
                p->volume -= 0.04;
                [[fallthrough]];
            case '(':
                p->volume -= 0.01;
                if (p->volume < app::def::minVolume)
                    p->volume = app::def::minVolume;
                p->term.update.volume = true;
                break;

            case KEY_RIGHT:
            case 'l':
                {
                    std::lock_guard lock(p->pw.mtx);

                    timeout(50);
                    while (c == 'l' || c == KEY_RIGHT)
                    {
                        p->hSnd.seek(app::def::step, SEEK_CUR);
                        p->term.update.time = true;
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
                        p->term.update.time = true;
                        p->pcmPos = p->hSnd.seek(0, SEEK_CUR) * p->pw.channels;
                        p->term.drawUI();
                        c = getch();
                    }
                    timeout(1000);
                }
                break;

            case 'o':
                p->next = true;
                break;

            case 'i':
                p->prev = true;
                break;

            case KEY_DOWN:
            case 'j':
                {
                    auto newSel = p->term.selected + 1;
                    if (newSel > (long)p->songs.size() - 1)
                    {
                        if (p->wrapSelection)
                            newSel = 0;
                        else
                            newSel = p->songs.size() - 1;
                    }
                    p->term.selected = newSel;
                    p->term.update.playList = true;
                }
                break;

            case KEY_UP:
            case 'k':
                {
                    auto newSel = p->term.selected - 1;
                    if (newSel < 0)
                    {
                        if (p->wrapSelection) newSel = p->songs.size() - 1;
                        else newSel = 0;
                    }
                    p->term.selected = newSel;
                    p->term.update.playList = true;
                }
                break;

            case 'g':
                p->term.selected = 0;
                p->term.update.playList = true;
                break;

            case 'G':
                p->term.selected = p->songs.size() - 1;
                p->term.update.playList = true;
                break;

            case KEY_NPAGE:
            case 4: /* C-d */
            case 6: /* C-f */
                {
                    long newSel = p->term.selected + 22;
                    if (newSel >= (long)p->songs.size())
                        newSel = p->songs.size() - 1;

                    p->term.selected = newSel;
                    p->term.update.playList = true;
                }
                break;

            case KEY_PPAGE:
            case 2: /* C-b */
            case 21: /* C-u */
                {
                    long newSel = p->term.selected - 22;
                    if (newSel < 0) newSel = 0;

                    p->term.selected = newSel;
                    p->term.update.playList = true;
                }
                break;

            case 46:
            case '/':
                p->subStringSearch(search::dir::forward);
                break;

            case 44:
            case '?':
                p->subStringSearch(search::dir::backwards);
                break;

            case 'n':
                p->jumpToFound(search::dir::forward);
                p->term.update.playList = true;
                break;

            case 'N':
                p->jumpToFound(search::dir::backwards);
                p->term.update.playList = true;
                break;

            case 'r':
                p->repeatAfterLast = !p->repeatAfterLast;
                p->term.update.songName = true;
                break;

            case ' ':
                p->paused = !p->paused;
                if (!p->paused)
                    p->pauseCnd.notify_one();
                break;

            case '\n':
                p->currSongIdx = p->term.selected;
                p->newSongSelected = true;
                p->term.updateAll();
                break;
                
            case 'z':
                p->term.selected = p->currSongIdx;
                p->term.firstInList = (p->term.selected - (p->term.maxListSize() - 1) / 2);
                p->term.update.playList = true;
                break;

            case KEY_RESIZE:
            case 12: /* C-l */
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

        p->term.update.time = true;

        p->term.drawUI();
    }
}
