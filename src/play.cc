#include "play.hh"
#include "app.hh"
#include "utils.hh"

/* https://docs.pipewire.org/page_tutorial4.html */

void
play::onProcess(void* data)
{
    app::PipeWirePlayer* p = (app::PipeWirePlayer*)data;
    pw_buffer* b;

    if ((b = pw_stream_dequeue_buffer(p->pw.stream)) == nullptr)
    {
        pw_log_warn("out of buffers: %m");
        return;
    }

    spa_buffer* buf = b->buffer;
    s16* dst;

    if ((dst = (s16*)buf->datas[0].data) == nullptr)
    {
        CERR("dst == nullptr\n");
        return;
    }

    int stride = sizeof(*dst) * app::def::channels;
    int nFrames = buf->datas[0].maxsize / stride;
    if (b->requested)
        nFrames = SPA_MIN(b->requested, (u64)nFrames);

    for (int i = 0; i < nFrames; i++)
    {
        for (int j = 0; j < app::def::channels; j++)
        {
            if (p->pcmPos > (long)p->pcmSize - 1)
            {
                pw_main_loop_quit(p->pw.loop);
                return;
            }

            /* modify each sample here */

            s16 val = p->pcmData[p->pcmPos] * p->volume;
            *dst++ = val;
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
        p->pauseCnd.wait(lock);
    }

    if (p->next || p->prev)
    {
        pw_main_loop_quit(p->pw.loop);
        return;
    }
}
