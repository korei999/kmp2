#pragma once
#include "ultratypes.h"

#include <condition_variable>
#include <pipewire/pipewire.h>
#include <vector>

namespace app
{

#define M_PI_M2 (M_PI + M_PI)

#define DEFAULT_RATE 48000
#define DEFAULT_CHANNELS 2

namespace defaults
{

constexpr f64 maxVolume = 1.4;
constexpr f64 minVolume = 0.0;
constexpr int step = 100000;

};

struct PwData
{
    pw_main_loop* loop {};
    pw_stream* stream {};
    pw_core* core {};
    u8* buffer[1024];
};

struct App
{
    std::vector<std::string> songs;
    s16* pcmData {};
    size_t pcmSize = 0;
    long pcmPos = 0;
    f64 volume = 0.5;
    PwData pw;

    bool paused = false;
    bool songFinished = false;
    std::mutex pauseMtx;
    std::condition_variable pauseCnd;

    App(int argc, char* argv[]);
    ~App();

    void setupPW(int argc, char* argv[]);
    void playAll();
};

} /* namespace app */
