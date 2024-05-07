#pragma once
#include "search.hh"
#include "ultratypes.h"

#include <condition_variable>
#include <pipewire/pipewire.h>
#include <sndfile.hh>
#include <ncurses.h>
#include <spa/param/audio/format-utils.h>


namespace app
{

#define M_PI_M2 (M_PI + M_PI)

namespace def
{

constexpr f64 maxVolume = 1.2;
constexpr f64 minVolume = 0.0;
constexpr int step = 100000;
constexpr int sampleRate = 48000;
constexpr int channels = 2;

};

struct PipeWireData
{
    pw_main_loop* loop {};
    pw_stream* stream {};
    enum spa_audio_format format = SPA_AUDIO_FORMAT_S16;
    u32 sampleRate = app::def::sampleRate;
    u32 channels = app::def::channels;
    static std::mutex mtx;
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
    WINDOW* pStd = stdscr;
    WINDOW* pPlayList {};
    long selected = 0;
    long firstInList = 0;
    const long listYPos = 6;
    bool goDown = false;
    bool goUp = false;
    std::mutex mtx;

    Curses();
    ~Curses() { endwin(); }

    void drawUI();
    void updateAll() { update.ui = update.time = update.volume = update.songName = update.playList = true; }
    size_t getMaxY() const { return getmaxy(pPlayList); }
    void resizePlayListWindow();

private:
    void drawTime();
    void drawVolume();
    void drawSongCounter();
    void drawSongName();
    void drawPlaylist();
};

struct PipeWirePlayer
{
    PipeWireData pw {};
    SndfileHandle hSnd {};
    std::vector<std::string> songs {};
    std::vector<int> foundIndices {};
    static f32 chunk[16384];
    long currSongIdx = 0;
    long currFoundIdx = 0;
    size_t pcmSize = 0;
    long pcmPos = 0;
    f64 volume = 0.20;
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
    void subStringSearch(enum search::dir direction);
    void jumpToFound(enum search::dir direction);
};

inline void
limitStringToMaxX(std::string* str)
{
    str->resize(getmaxx(stdscr));
}

} /* namespace app */
