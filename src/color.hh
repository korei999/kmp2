#pragma once
#include "utils.hh"

#include <ncurses.h>

namespace color
{

enum curses : short
{
    termdef = -1, /* -1 should preserve terminal default color when use_default_colors() */
    black = 0,
    white = 1,
    green,
    yellow,
    blue,
    cyan,
    red
};

struct rgb
{
    int r, g, b;

    constexpr rgb(int hex)
        : r{utils::cRound((((hex >> 16) & 0xff) / 255.0) * 1000.0)},
          g{utils::cRound((((hex >> 8) & 0xff) / 255.0) * 1000.0)},
          b{utils::cRound(((hex & 0xff) / 255.0) * 1000.0)}
    {}
};

} /* namespace color */
