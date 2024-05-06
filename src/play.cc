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
    f32 vol = std::pow(p->volume, 1.5);

    for (int i = 0; i < nFrames; i++)
    {
        for (int j = 0; j < (int)p->pw.channels; j++)
        {
            if (p->pcmPos > (long)p->pcmSize - 1)
            {
                pw_main_loop_quit(p->pw.loop);
                return;
            }

            /* modify each sample here */
            f32 val = p->chunk[chunkPos] * vol;

            *dst++ = val;

            chunkPos++;
            p->pcmPos++;
        }
    }

    p->pcmPos = p->hSnd.seek(0, SEEK_CUR) * p->pw.channels;

    buf->datas[0].chunk->offset = 0;
    buf->datas[0].chunk->stride = stride;
    buf->datas[0].chunk->size = nFrames * stride;

    pw_stream_queue_buffer(p->pw.stream, b);

    while (p->paused)
    {
        /* FIXME: this blocks systems sound on device change */
        std::unique_lock lock(p->pauseMtx);
        p->pauseCnd.wait(lock);
    }

    if (p->next || p->prev || p->newSongSelected || p->finished)
    {
        p->pw.mtx.unlock();
        pw_main_loop_quit(p->pw.loop);
        return;
    }
}

} /* namespace play */
