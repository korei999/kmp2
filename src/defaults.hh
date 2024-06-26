#pragma once
#include "color.hh"
#include "ultratypes.h"

namespace defaults
{

constexpr f64 maxVolume   = 1.2; /* 1.0 == 100% */
constexpr f64 minVolume   = 0.0;
constexpr f32 volume      = 0.15; /* volume at startup */
constexpr f64 volumePower = 3.0; /* affects volume curve aka 'std::pow(volume, volumePower)' */

constexpr long maxSampleRate = 666666; /* can be stupid big */
constexpr long minSampleRate = 1000; /* should be > 0 */

constexpr f64 step            = 5.0; /* seek step (in seconds) */
constexpr u32 updateRate      = 200; /* time (ms) between input polls (affects visualizer updates for now) */
constexpr u32 timeOut         = 5000; /* time (ms) to cancel input */
constexpr bool bWrapSelection = true; /* jump to first after scrolling past the last element in the list */

constexpr bool bDrawVisualizer        = false;
constexpr f32 visualizerScalar        = 9.0; /* scale the height of each bar */
constexpr wchar_t visualizerSymbol[2] = L":";
constexpr long visualizerHeight = 4;
constexpr short barHeightColors[visualizerHeight] {
    color::curses::red,
    color::curses::yellow,
    color::curses::green,
    color::curses::green
};

constexpr bool bTransparentBg      = true;
constexpr bool bCustomColorPallete = false; /* use custom colors below, otherwise use terminal defaults */
constexpr color::rgb black   = 0x000000;
constexpr color::rgb red     = 0xFF0000;
constexpr color::rgb green   = 0x00FF00;
constexpr color::rgb yellow  = 0xFFFF00;
constexpr color::rgb blue    = 0x0000FF;
constexpr color::rgb magenta = 0xFF00FF;
constexpr color::rgb cyan    = 0x0000FF;
constexpr color::rgb white   = 0xFFFFFF;

constexpr color::curses borderColor = color::blue;
constexpr color::curses mutedColor  = color::blue;

} /* namespace defaults */
