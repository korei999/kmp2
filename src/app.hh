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
constexpr f64 volume = 0.15;

};

constexpr wchar_t volumeLevels[17][2] {
    L" ", L"▁", L"▁", L"▂", L"▂", L"▃", L"▃", L"▄", L"▄", L"▅", L"▅", L"▆", L"▆", L"▇", L"▇", L"█", L"█"
};
constexpr wchar_t blockIcon[2] = L"█";
constexpr size_t chunkSize = 0x4000;

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
        termdef = -1, /* -1 should preserve terminal default color when use_default_colors() */
        white = 1,
        green,
        yellow,
        blue,
        cyan,
        red
    };

    /* mark which to update on drawUI */
    struct
    {
        bool bTime = true;
        bool bVolume = true;
        bool bSongName = true;
        bool bPlayList = true;
        bool bBottomLine = true;
    } update;
    PipeWirePlayer* p {};
    WINDOW* pPlayList {};
    long selected = 0;
    long firstInList = 0;
    const long listYPos = 6;
    std::mutex mtx;

    Curses();
    ~Curses() { endwin(); }

    void drawUI();
    void updateAll() { update.bTime = update.bVolume = update.bSongName = update.bPlayList = update.bBottomLine = true; }
    size_t playListMaxY() const { return getmaxy(pPlayList); }
    void resizePlayListWindow();

private:
    void drawTime();
    void drawVolume();
    void drawPlayListCounter();
    void drawTitle();
    void drawPlayList();
    void drawBorder();
    void drawBottomLine();
};

struct SongInfo
{
    std::string title;
};

struct PipeWirePlayer
{
    PipeWireData pw {};
    SndfileHandle hSnd {};
    SongInfo info {};
    std::vector<std::string> songs {};
    std::vector<int> foundIndices {};
    std::wstring searchingNow {};
    static f32 chunk[chunkSize];
    long currSongIdx = 0;
    long currFoundIdx = 0;
    size_t pcmSize = 0;
    long pcmPos = 0;
    f64 volume = def::volume;
    Curses term;
    bool bPaused = false;
    bool bNext = false;
    bool bPrev = false;
    bool bNewSongSelected = false;
    bool bRepeatAfterLast = false;
    bool bWrapSelection = true;
    bool bFinished = false;
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
    void centerOn(size_t i);
    void setSeek();
    void jumpTo();
};

} /* namespace app */
