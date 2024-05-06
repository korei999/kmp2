#include "input.hh"
#include "utils.hh"

#include <ncurses.h>

void
input::read(app::PipeWirePlayer* p)
{
    int c;

    auto goTop = [p]() -> void
    {
        p->term.selected = 0;
        p->term.firstInList = 0;
    };

    auto goBot = [p]() -> void
    {
        p->term.selected = p->songs.size() - 1;
        auto off = std::min(p->term.maxListSize(), p->songs.size());
        p->term.firstInList = p->term.selected - (off - 1);
    };

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
                    p->hSnd.seek(app::def::step, SEEK_CUR);
                }
                break;

            case KEY_LEFT:
            case 'h':
                {
                    std::lock_guard lock(p->pw.mtx);
                    p->hSnd.seek(-app::def::step, SEEK_CUR);
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
                        {
                            newSel = 0;
                            goTop();
                        }
                        else
                        {
                            newSel = p->songs.size() - 1;
                        }
                    }
                    p->term.selected = newSel;
                    p->term.goDown = true;
                    p->term.update.playList = true;
                }
                break;

            case KEY_UP:
            case 'k':
                {
                    auto newSel = p->term.selected - 1;
                    if (newSel < 0)
                    {
                        if (p->wrapSelection)
                        {
                            newSel = p->songs.size() - 1;
                            goBot();
                        }
                        else
                        {
                            newSel = 0;
                        }
                    }
                    p->term.selected = newSel;
                    p->term.goUp = true;
                    p->term.update.playList = true;
                }
                break;

            case 'g':
                goTop();
                p->term.update.playList = true;
                break;

            case 'G':
                goBot();
                p->term.update.playList = true;
                break;

            case 6:
            case KEY_NPAGE:
            case 4: /* C-d */
                {
                    long newSel = p->term.selected + 22;
                    long newFirst = p->term.firstInList + 22;

                    if (newFirst > ((long)p->songs.size() - 1) - ((long)p->term.maxListSize() - 1))
                        newFirst = (p->songs.size() - 1) - (p->term.maxListSize() - 1);
                    if (newFirst < 0)
                        newFirst = 0;
                    if (newSel > (long)(p->songs.size() - 1))
                        newSel = (p->songs.size() - 1);

                    p->term.selected = newSel;
                    p->term.firstInList = newFirst;
                    p->term.update.playList = true;
                }
                break;

            case 2:
            case KEY_PPAGE:
            case 21: /* C-u */
                {
                    long newSel = p->term.selected - 22;
                    long newFirst = p->term.firstInList - 22;

                    if (newSel < 0)
                    {
                        goTop();
                    }
                    else
                    {
                        if (newFirst < 0) newFirst = 0;

                        p->term.selected = newSel;
                        p->term.firstInList = newFirst;
                    }

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

            case KEY_RESIZE:
            case 12: /* C-l */
                p->term.updateAll();
                redrawwin(stdscr);
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

        p->term.updateUI();
    }
}
