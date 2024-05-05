/* https://docs.pipewire.org/page_tutorial4.html */

#include "play.hh"
#include "app.hh"
#include "utils.hh"

void
play::onProcess(void* data)
{
    auto* p = (app::PipeWirePlayer*)data;
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

    // static f32 chunk[30000];
    // p->hSnd.readf(chunk, nFrames);
    // int pos = 0;
    // auto vol = p->volume;

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
            // f32 val = chunk[pos] * vol;
            f32 val = p->pcmData[p->pcmPos] * p->volume;
            // val = std::clamp(val, SHRT_MIN, SHRT_MAX);

            *dst++ = val;

            // pos++;
            p->pcmPos++;
        }
    }

    buf->datas[0].chunk->offset = 0;
    buf->datas[0].chunk->stride = stride;
    buf->datas[0].chunk->size = nFrames * stride;

    pw_stream_queue_buffer(p->pw.stream, b);

    if (p->paused)
    {
        std::unique_lock lock(p->pauseMtx);
        pw_stream_set_active(p->pw.stream, false);
        p->pauseCnd.wait(lock);
        pw_stream_set_active(p->pw.stream, true);
    }

    if (p->next || p->prev || p->newSongSelected || p->finished)
    {
        pw_main_loop_quit(p->pw.loop);
        return;
    }
}
