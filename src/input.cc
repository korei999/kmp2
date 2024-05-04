#include "input.hh"

#include <ncurses.h>

void
input::read(app::PipeWirePlayer* p)
{
    int c;
    bool volumeChanged = false;
    bool refreshUI = false;

    auto goTop = [p]() -> void
    {
        p->term.selected = 0;
        p->term.firstInList = 0;
    };

    auto goBot = [p]() -> void
    {
        p->term.selected = p->songs.size() - 1;
        auto off = std::min(p->term.maxListSize(), (long)p->songs.size());
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
            case ')':
                p->volume += 0.01;
                if (p->volume > app::def::maxVolume)
                    p->volume = app::def::maxVolume;
                break;

            case '9':
                p->volume -= 0.04;
            case '(':
                p->volume -= 0.01;
                if (p->volume < app::def::minVolume)
                    p->volume = app::def::minVolume;
                break;

            case 'l':
                {
                    /* bad data race */
                    long newPos = p->pcmPos += app::def::step;
                    if (newPos > (long)p->pcmSize - 1)
                        newPos = p->pcmSize - 1;
                    p->pcmPos = newPos;
                }
                break;

            case 'h':
                {
                    /* bad data race */
                    long newPos = p->pcmPos -= app::def::step;
                    if (newPos < 0)
                        newPos = 0;
                    p->pcmPos = newPos;
                }
                break;

            case 'o':
                p->next = true;
                break;

            case 'i':
                p->prev = true;
                break;

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
                    p->term.listDown = true;
                }
                break;

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
                    p->term.listUp = true;
                }
                break;

            case 'g':
                goTop();
                break;

            case 'G':
                goBot();
                break;

            case 'r':
                p->repeatAll = !p->repeatAll;
                break;

            case ' ':
                p->paused = !p->paused;
                if (!p->paused)
                    p->pauseCnd.notify_one();
                break;

            case '\n':
                p->currSongIdx = p->term.selected;
                p->newSongSelected = true;
                break;

            case KEY_RESIZE:
            case 12: /* C-l */
                redrawwin(stdscr);
                break;

            default:
                break;
        }

        if (refreshUI)
        {
        }

        p->term.updateUI();
    }
}
