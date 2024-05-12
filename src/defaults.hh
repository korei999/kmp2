#pragma once
#include "ultratypes.h"

namespace defaults
{

constexpr f64 maxVolume = 1.2; /* 1.0 == 100% */
constexpr f64 minVolume = 0.0;
constexpr f64 volume = 0.15; /* volume at startup */
constexpr int step = 100000; /* pcm relative seek step */
constexpr u32 updateRate = 1000; /* time (in ms) between input polls */
constexpr u32 timeOut = 5000; /* time (in ms) to cancel input */

} /* namespace defaults */
