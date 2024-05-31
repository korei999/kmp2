/* https://docs.pipewire.org/page_tutorial4.html */

#include "play.hh"
#include "app.hh"
#include "utils.hh"

#include <cmath>

namespace play
{

void
onProcessCB(void* data)
{
    auto* p = (app::PipeWirePlayer*)data;

    std::lock_guard lock(p->m_pw.mtx);

    if (p->m_bChangeParams)
        pw_main_loop_quit(p->m_pw.pLoop);

    p->m_mtxPauseSwitch.lock();
    if (p->m_bPaused)
    {
        pw_main_loop_quit(p->m_pw.pLoop);
        /* NOTE: unlock in `PipeWirePlayer::playCurrent()` */
        return;
    }
    p->m_mtxPauseSwitch.unlock();

    pw_buffer* b;
    if ((b = pw_stream_dequeue_buffer(p->m_pw.pStream)) == nullptr)
    {
        pw_log_warn("out of buffers: %m");
        return;
    }

    spa_buffer* buf = b->buffer;
    f32* dst;

    if ((dst = (f32*)buf->datas[0].data) == nullptr)
    {
        CERR("dst == nullptr\n");
        return;
    }

    int stride = sizeof(f32) * p->m_pw.channels;
    int nFrames = buf->datas[0].maxsize / stride;
    if (b->requested) nFrames = SPA_MIN(b->requested, (u64)nFrames);

    if (nFrames > 1024*4) nFrames = 1024*4; /* limit to arbitrary number, `SPA_MAX` maybe? */

    p->m_pw.lastNFrames = nFrames;
    p->m_hSnd.readf(p->m_chunk, nFrames);
    int chunkPos = 0;

    /* non linear nicer ramping */
    f32 vol = p->m_bMuted ? 0.0 : std::pow(p->m_volume, defaults::volumePower);

    for (int i = 0; i < nFrames; i++)
    {
        for (int j = 0; j < (int)p->m_pw.channels; j++)
        {
            /* modify each sample here */
            f32 val = p->m_chunk[chunkPos] * vol;

            *dst++ = val;

            chunkPos++;
            p->m_pcmPos++;
        }
    }

    /* update position last time */
    p->m_pcmPos = p->m_hSnd.seek(0, SEEK_CUR) * p->m_pw.channels;

    buf->datas[0].chunk->offset = 0;
    buf->datas[0].chunk->stride = stride;
    buf->datas[0].chunk->size = nFrames * stride;

    pw_stream_queue_buffer(p->m_pw.pStream, b);

    if (p->m_bNext                          ||
        p->m_bPrev                          ||
        p->m_bNewSongSelected               ||
        p->m_bFinished                      ||
        p->m_pcmPos > (long)p->m_pcmSize - 1)
    {
        pw_main_loop_quit(p->m_pw.pLoop);
    }
}

void
stateChangedCB([[maybe_unused]] void* data,
               [[maybe_unused]] enum pw_stream_state old,
			   [[maybe_unused]] enum pw_stream_state state,
               [[maybe_unused]] const char* error)
{
    /*
    switch (state)
    {
        case PW_STREAM_STATE_ERROR:
            if (error) CERR("PW_STREAM_STATE_ERROR: {}\n", error);
            break;

        case PW_STREAM_STATE_UNCONNECTED:
            CERR("PW_STREAM_STATE_UNCONNECTED\n");
            break;

        case PW_STREAM_STATE_CONNECTING:
            CERR("PW_STREAM_STATE_CONNECTING\n");
            break;

        case PW_STREAM_STATE_PAUSED:
            CERR("PW_STREAM_STATE_PAUSED\n");
            break;

        case PW_STREAM_STATE_STREAMING:
            CERR("PW_STREAM_STATE_STREAMING\n");
            break;

        default:
            break;
    }
    */
}

void
ioChangedCB([[maybe_unused]] void* data,
            [[maybe_unused]] uint32_t id,
            [[maybe_unused]] void* area,
            [[maybe_unused]] uint32_t size)
{
    //
}

void
paramChangedCB([[maybe_unused]] void* data,
               [[maybe_unused]] uint32_t id,
               [[maybe_unused]] const spa_pod* param)
{
    //
}

} /* namespace play */
