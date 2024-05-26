#pragma once
#include "search.hh"
#include "ultratypes.h"
#include "song.hh"
#include "defaults.hh"

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <pipewire/pipewire.h>
#include <sndfile.hh>
#include <ncurses.h>
#include <spa/param/audio/format-utils.h>

namespace app
{

constexpr wchar_t blockIcon0[3] = L"█";
constexpr wchar_t blockIcon1[3] = L"▮";
constexpr wchar_t blockIcon2[3] = L"▯";
constexpr wchar_t blockIconST0[3] = L"░";
constexpr wchar_t botBarIcon0[3] = L"▁";
constexpr size_t chunkSize = 0x4000; /* big enough */

struct PipeWireData
{
    pw_main_loop* loop {};
    pw_stream* stream {};
    enum spa_audio_format format = SPA_AUDIO_FORMAT_S16;
    u32 sampleRate = 48000;
    u32 origSampleRate = sampleRate;
    u32 channels = 2;
    int lastNFrames = 0;
    static std::mutex mtx;
};

struct PipeWirePlayer;

/* pBor for borders and pCon for content */
struct BWin
{
    WINDOW* pBor {};
    WINDOW* pCon {};
};

struct Curses
{
    /* parent pointer */
    PipeWirePlayer* p {};
    /* mark which to update on drawUI */
    struct {
        bool bPlayList = true;
        bool bBottomLine = true;
        bool bStatus = true;
        bool bInfo = true;
        bool bVisualizer = true;
    } update;
    BWin status {};
    BWin info {};
    BWin pl {};
    BWin vis {};
    long selected = 0;
    long firstInList = 0;
    const long listYPos = 6;
    const long visualizerYSize = 4;
    std::mutex mtx;
    std::mutex mtxRefresh;

    Curses();
    ~Curses();

    void drawUI();
    void updateAll() { update.bInfo = update.bStatus = update.bPlayList = update.bBottomLine = true; }
    size_t playListMaxY() const { return getmaxy(pl.pBor); }
    void resizeWindows();
    void drawVisualizer();

private:
    void drawTime();
    enum color::curses drawVolume();
    void drawPlayListCounter();
    void drawTitle();
    void drawPlayList();
    void drawBottomLine();
    void drawInfo();
    void drawStatus();
    void fixCursorPos();
};

struct PipeWirePlayer
{
    std::mutex mtxPause;
    std::atomic<bool> ready = false;
    std::condition_variable cndPause;
    PipeWireData pw {};
    SndfileHandle hSnd {};
    song::Info info {};
    Curses term {};
    std::vector<std::string> songs {};
    std::vector<int> foundIndices {};
    std::wstring searchingNow {};
    static f32 chunk[chunkSize];
    long currSongIdx = 0;
    long currFoundIdx = 0;
    size_t pcmSize = 0;
    long pcmPos = 0;
    f64 volume = defaults::volume;
    bool bMuted = false;
    std::atomic<bool> bPaused = false;
    bool bNext = false;
    bool bPrev = false;
    bool bNewSongSelected = false;
    bool bRepeatAfterLast = false;
    bool bWrapSelection = defaults::bWrapSelection;
    std::atomic<bool> bDrawVisualizer = defaults::bDrawVisualizer;
    std::atomic<bool> bFinished = false;
    bool bChangeParams = false;
    f64 speedMul = 1.0;

    PipeWirePlayer(int argc, char** argv);
    ~PipeWirePlayer();

    void setupPlayer(enum spa_audio_format format, u32 sampleRate, u32 channels);
    void playAll();
    void playCurrent();
    const std::string_view currSongName() const { return songs[currSongIdx]; }
    bool subStringSearch(enum search::dir direction);
    void jumpToFound(enum search::dir direction);
    void centerOn(size_t i);
    void setSeek();
    void jumpTo();
};

} /* namespace app */
