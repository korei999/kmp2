#pragma once
#include "ultratypes.h"

#include <condition_variable>
#include <ncurses.h>
#include <pipewire/pipewire.h>
#include <spa/param/audio/format-utils.h>
#include <vector>

namespace app
{

#define M_PI_M2 (M_PI + M_PI)

namespace def
{

constexpr f64 maxVolume = 1.4;
constexpr f64 minVolume = 0.0;
constexpr int step = 100000;
constexpr int sampleRate = 44100;
constexpr int channels = 2;

};

struct PipeWireData
{
    pw_main_loop* loop {};
    pw_stream* stream {};
    enum spa_audio_format format = SPA_AUDIO_FORMAT_S16;
    u32 sampleRate = app::def::sampleRate;
    u32 channels = app::def::channels;
};

struct PipeWirePlayer;

struct Curses
{
    PipeWirePlayer* p;
    WINDOW* plWin;

    void updateUI();
    void drawTime();
    void drawPlaylist();
};

struct PipeWirePlayer
{
    std::vector<std::string> songs;
    long currSongIdx;
    s16* pcmData {};
    size_t pcmSize = 0;
    long pcmPos = 0;
    f64 volume = 0.3;
    PipeWireData pw;
    Curses term;
    bool paused = false;
    bool next = false;
    bool prev = false;
    bool repeatAll = false;
    bool finished = false;
    std::mutex pauseMtx;
    std::condition_variable pauseCnd;

    PipeWirePlayer(int argc, char** argv);
    ~PipeWirePlayer();

    void setupPlayer(enum spa_audio_format format, u32 sampleRate, u32 channels);
    void playAll();
    void playCurrent();
    std::string_view currSongName() { return songs[currSongIdx]; }
};

} /* namespace app */
