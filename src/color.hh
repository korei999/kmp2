#pragma once

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

constexpr int
cRound(double x)
{
    return x < 0 ? x - 0.5 : x + 0.5;
}

struct rgb
{
    int r, g, b;

    constexpr rgb(int hex)
        : r{cRound((((hex >> 16) & 0xff) / 255.0) * 1000.0)},
          g{cRound((((hex >> 8) & 0xff) / 255.0) * 1000.0)},
          b{cRound(((hex & 0xff) / 255.0) * 1000.0)}
    {}
};

} /* namespace color */
