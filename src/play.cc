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

    std::lock_guard lock(p->pw.mtx);

    if (p->bPaused)
        return;

    pw_buffer* b;
    if ((b = pw_stream_dequeue_buffer(p->pw.stream)) == nullptr)
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

    int stride = sizeof(f32) * p->pw.channels;
    int nFrames = buf->datas[0].maxsize / stride;
    if (b->requested)
        nFrames = SPA_MIN(b->requested, (u64)nFrames);

    p->hSnd.readf(p->chunk, nFrames);
    int chunkPos = 0;

    /* non linear nicer ramping */
    f32 vol = p->bMuted ? 0.0 : std::pow(p->volume, 1.5);

    for (int i = 0; i < nFrames; i++)
    {
        for (int j = 0; j < (int)p->pw.channels; j++)
        {
            /* modify each sample here */
            f32 val = p->chunk[chunkPos] * vol;

            *dst++ = val;

            chunkPos++;
            p->pcmPos++;
        }
    }

    /* update position last time */
    p->pcmPos = p->hSnd.seek(0, SEEK_CUR) * p->pw.channels;

    buf->datas[0].chunk->offset = 0;
    buf->datas[0].chunk->stride = stride;
    buf->datas[0].chunk->size = nFrames * stride;

    pw_stream_queue_buffer(p->pw.stream, b);

    if (p->bNext || p->bPrev || p->bNewSongSelected || p->bFinished || p->pcmPos > (long)p->pcmSize - 1)
        pw_main_loop_quit(p->pw.loop);
}

} /* namespace play */
