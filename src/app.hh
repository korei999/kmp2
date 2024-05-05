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

constexpr f64 maxVolume = 1.3;
constexpr f64 minVolume = 0.0;
constexpr int step = 150000;
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
    enum color : short
    {
        termdef = -1, /* -1 should preserve terminal default color due to use_default_colors() */
        green = 1,
        yellow,
        blue,
        cyan,
        red
    };

    struct
    {
        bool ui = true;
        bool time = true;
        bool volume = true;
        bool songName = true;
        bool playList = true;
    } update;
    PipeWirePlayer* p {};
    WINDOW* plWin {};
    long selected = 0;
    long firstInList = 0;
    const long listYPos = 7;
    bool goDown = false;
    bool goUp = false;

    void updateUI();
    void drawTime();
    void drawVolume();
    void drawSongCounter();
    void drawSongName();
    void drawPlaylist();
    void updateAll();
    long maxListSize() const { return getmaxy(stdscr) - listYPos; }
};

struct PipeWirePlayer
{
    PipeWireData pw {};
    std::vector<std::string> songs {};
    long currSongIdx = 0;
    s16* pcmData {};
    size_t pcmSize = 0;
    long pcmPos = 0;
    f64 volume = 0.05;
    Curses term;
    bool paused = false;
    bool next = false;
    bool prev = false;
    bool newSongSelected = false;
    bool repeatAfterLast = false;
    bool wrapSelection = true;
    bool finished = false;
    std::mutex pauseMtx;
    std::condition_variable pauseCnd;

    PipeWirePlayer(int argc, char** argv);
    ~PipeWirePlayer();

    void setupPlayer(enum spa_audio_format format, u32 sampleRate, u32 channels);
    void playAll();
    void playCurrent();
    const std::string_view currSongName() const { return songs[currSongIdx]; }
};

} /* namespace app */
