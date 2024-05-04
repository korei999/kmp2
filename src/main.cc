#include "input.hh"
#include "app.hh"

#include <ncurses.h>
#include <thread>
#include <spa/param/audio/format-utils.h>

int
main(int argc, char* argv[])
{
    setlocale(LC_ALL, "");

    initscr();
    use_default_colors();
    curs_set(0);
    noecho();
    cbreak();
    timeout(1000);
    keypad(stdscr, true);
    refresh();

    app::PipeWirePlayer p(argc, argv);

    std::thread inputThread(input::read, &p);
    inputThread.detach();

    p.playAll();

    endwin();
    fflush(stderr);
}
