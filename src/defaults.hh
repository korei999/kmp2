#pragma once
#include "ultratypes.h"
#include "color.hh"

#include <string_view>

namespace defaults
{

constexpr std::string_view formatsSupported[] {
    ".flac", ".opus", ".mp3", ".ogg", ".wav", ".caf", ".aif"
};

constexpr f32 maxVolume = 1.2; /* 1.0 == 100% */
constexpr f32 minVolume = 0.0;
constexpr f32 volume = 0.15; /* volume at startup */
constexpr f64 volumePower = 3.0; /* affects volume curve aka 'std::pow(volume, volumePower)' */
constexpr int step = 100000; /* pcm relative seek step (for sampleRate=48000 -> "100000 / sampleRate" == skip 2.08 sec each step) */
constexpr u32 updateRate = 50; /* time (ms) between input polls (affects visualizer updates for now) */
constexpr u32 timeOut = 5000; /* time (ms) to cancel input */
constexpr bool bWrapSelection = true; /* jump to first after scrolling past the last element in the list */
constexpr bool bDrawVisualizer = true;
constexpr u32 visualizerUpdateRate = 50; /* time (ms) between visualizer upates ((unused)) */
constexpr f32 visualizerScalar = 5.0; /* scale the height of each bar */
constexpr bool bNormalizeVisualizer = false;
constexpr wchar_t visualizerSymbol[2] = L":";
constexpr color::curses borderColor = color::blue;
constexpr color::curses mutedColor = color::blue;

} /* namespace defaults */
