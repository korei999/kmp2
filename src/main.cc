#include "input.hh"
#include "app.hh"

int
main(int argc, char* argv[])
{
    setlocale(LC_ALL, "");

    /* TODO: move to class */
    initscr();
    start_color();
    use_default_colors();
    curs_set(0);
    noecho();
    cbreak();
    timeout(1000);
    keypad(stdscr, true);
    refresh();

    int td = (app::Curses::termdef);
    init_pair(app::Curses::green, COLOR_GREEN, td);
    init_pair(app::Curses::yellow, COLOR_YELLOW, td);
    init_pair(app::Curses::blue, COLOR_BLUE, td);
    init_pair(app::Curses::cyan, COLOR_CYAN, td);
    init_pair(app::Curses::red, COLOR_RED, td);

    app::PipeWirePlayer p(argc, argv);

    std::thread inputThread(input::read, &p);
    inputThread.detach();

    p.playAll();

    endwin();
}
