#pragma once

namespace color
{

enum curses : short
{
    termdef = -1, /* -1 should preserve terminal default color when use_default_colors() */
    white = 1,
    green,
    yellow,
    blue,
    cyan,
    red
};

} /* namespace color */
