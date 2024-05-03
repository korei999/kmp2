#include "input.hh"
#include "utils.hh"

#include <ncurses.h>

void
readInput(app::App* app)
{
    int c;
    bool volumeChanged = false;

    while ((c = getch()))
    {
        switch (c)
        {
            case 'q':
                pw_main_loop_quit(app->pw.loop);
                return;
                break;

            case '+':
            case '=':
                app->volume += 0.05;
                volumeChanged = true;
                break;

            case '_':
            case '-':
                app->volume -= 0.05;
                volumeChanged = true;
                break;

            case 'l':
                app->pcmPos += app::defaults::step;
                if (app->pcmPos >= (long)app->pcmSize - 1)
                    app->pcmPos = app->pcmSize - 1;
                break;

            case 'h':
                app->pcmPos -= app::defaults::step;
                if (app->pcmPos < 0)
                    app->pcmPos = 0;
                break;

            case ' ':
                app->paused = !app->paused;
                LOG(WARN, "paused: {}\n", app->paused);

                if (!app->paused)
                    app->pauseCnd.notify_one();

                break;

            default:
                break;
        }

        if (volumeChanged)
        {
            volumeChanged = false;
            app->volume = std::clamp(app->volume, app::defaults::minVolume, app::defaults::maxVolume);
            LOG(OK, "volume: '{:.02f}'\n", app->volume);
        }
    }
}

