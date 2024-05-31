#pragma once
#include "search.hh"
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
    pw_core* pCore {};
    pw_context* pCtx {};
    pw_main_loop* pLoop {};
    pw_stream* pStream {};
    static const pw_stream_events streamEvents;
    enum spa_audio_format eformat = SPA_AUDIO_FORMAT_S16;
    u32 sampleRate = 48000;
    u32 origSampleRate = sampleRate;
    u32 channels = 2;
    int lastNFrames = 0;
    static std::mutex mtx;
};

class PipeWirePlayer;

/* pBor for borders and pCon for content */
struct BCWin
{
    WINDOW* pBor {};
    WINDOW* pCon {};
};

class Curses
{
public:
    /* parent pointer */
    PipeWirePlayer* m_p {};
    /* mark which to update on drawUI */
    struct {
        bool bPlayList = true;
        bool bBottomLine = true;
        bool bStatus = true;
        bool bInfo = true;
        bool bVisualizer = true;
    } m_update;
    BCWin m_status {};
    BCWin m_info {};
    BCWin m_pl {};
    BCWin m_vis {};
    long m_selected = 0;
    long m_firstInList = 0;
    const long m_listYPos = 6;
    const long m_visualizerYSize = 4;
    std::mutex m_mtx {};
    std::atomic<bool> m_bDrawVisualizer = defaults::bDrawVisualizer;

    Curses();
    ~Curses();

    size_t playListMaxY() const { return getmaxy(m_pl.pBor); }
    void updatePlayList() { m_update.bPlayList = true; }
    void updateBottomLine() { m_update.bBottomLine = true; }
    void updateStatus() { m_update.bStatus = true; }
    void updateInfo() { m_update.bInfo = true; }
    void updateVisualizer() { m_update.bVisualizer = true; }
    bool toggleVisualizer();
    void resizeWindows();
    void updateAll() { m_update.bPlayList = m_update.bBottomLine = m_update.bStatus = m_update.bInfo = m_update.bVisualizer = true; }
    void drawUI();

private:
    void drawVisualizer();
    void drawTime();
    enum color::curses drawVolume();
    void drawPlayListCounter();
    void drawTitle();
    void drawPlayList();
    void drawBottomLine();
    void drawInfo();
    void drawStatus();
    void adjustListToPosition();
};

class PipeWirePlayer
{
public:
    static constexpr std::string_view m_supportedFormats[] {".flac", ".opus", ".mp3", ".ogg", ".wav", ".caf", ".aif"};
    std::mutex m_mtxPause {};
    std::mutex m_mtxPauseSwitch {};
    std::atomic<bool> m_ready = false;
    std::condition_variable m_cndPause {};
    PipeWireData m_pw {};
    SndfileHandle m_hSnd {};
    song::Info m_info {};
    Curses m_term {};
    long m_selected = 0;
    std::vector<std::string> m_songs {};
    std::vector<int> m_foundIndices {};
    std::wstring m_searchingNow {};
    static f32 m_chunk[chunkSize];
    long m_currSongIdx = 0;
    long m_currFoundIdx = 0;
    size_t m_pcmSize = 0;
    long m_pcmPos = 0;
    f64 m_volume = defaults::volume;
    bool m_bMuted = false;
    bool m_bPaused = false;
    bool m_bNext = false;
    bool m_bPrev = false;
    bool m_bNewSongSelected = false;
    bool m_bRepeatAfterLast = false;
    bool m_bWrapSelection = defaults::bWrapSelection;
    std::atomic<bool> m_bFinished = false;
    bool m_bChangeParams = false;
    f64 m_speedMul = 1.0;

    PipeWirePlayer(int argc, char** argv);
    ~PipeWirePlayer();

    void setupPlayer(enum spa_audio_format format, u32 sampleRate, u32 channels);
    void playAll();
    void playCurrent();
    const std::string_view currSongName() const { return m_songs[m_currSongIdx]; }
    bool subStringSearch(enum search::dir direction);
    void jumpToFound(enum search::dir direction);
    void centerOn(size_t i);
    void setSeek();
    void jumpTo();
    void pause();
    void resume();
    bool togglePause();
    bool toggleMute() { return (m_bMuted = !m_bMuted); }
    bool toggleRepeatAfterLast() { return m_bRepeatAfterLast = !m_bRepeatAfterLast; }
    void setVolume(f64 vol);
    void addSampleRate(long val);
    void restoreOrigSampleRate();
    void finish();
    void next() { m_bNext = true; }
    void prev() { m_bPrev = true; }
    void select(long pos, bool bWrap = defaults::bWrapSelection);
    void selectNext() { select(m_selected + 1); }
    void selectPrev() { select(m_selected - 1); }
    void selectFirst() { select(0); }
    void selectLast() { select(m_songs.size() - 1); }
    void playSelected();
};

} /* namespace app */
