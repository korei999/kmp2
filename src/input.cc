#include "input.hh"

#include <ncurses.h>

void
input::read(app::PipeWirePlayer* p)
{
    int c;
    bool volumeChanged = false;
    bool refreshUI = false;

    while ((c = getch()))
    {
        switch (c)
        {
            case 'q':
                pw_main_loop_quit(p->pw.loop);
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
                    /* trying to modify pcmPos directly causes bad data race */
                    long newPos = p->pcmPos += app::def::step;
                    if (newPos > (long)p->pcmSize - 1)
                        newPos = p->pcmSize - 1;
                    p->pcmPos = newPos;
                }
                break;

            case 'h':
                {
                    /* trying to modify pcmPos directly causes bad data race */
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

            case ' ':
                p->paused = !p->paused;
                if (!p->paused)
                    p->pauseCnd.notify_one();
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
