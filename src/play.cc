#include "play.hh"
#include "app.hh"

void
play::onProcess(void* userData)
{
    app::App* app = (app::App*)userData;
    pw_buffer* b;

    if ((b = pw_stream_dequeue_buffer(app->pw.stream)) == nullptr)
    {
        pw_log_warn("out of buffers: %m");
        return;
    }

    spa_buffer* buf = b->buffer;
    s16* dst;

    if ((dst = (s16*)buf->datas[0].data) == nullptr)
        return;

    int stride = sizeof(*dst) * DEFAULT_CHANNELS;
    int nFrames = buf->datas[0].maxsize / stride;
    if (b->requested)
        nFrames = SPA_MIN(b->requested, (u64)nFrames);

    for (int i = 0; i < nFrames; i++)
    {
        for (int j = 0; j < DEFAULT_CHANNELS; j++)
        {
            if (app->pcmPos >= app->pcmSize - 1)
            {
                pw_main_loop_quit(app->pw.loop);
                return;
            }

            s16 val = app->pcmData[app->pcmPos] * app->volume;
            *dst++ = val;
            app->pcmPos++;
        }
    }

    buf->datas[0].chunk->offset = 0;
    buf->datas[0].chunk->stride = stride;
    buf->datas[0].chunk->size = nFrames * stride;

    if (app->paused)
    {
        std::unique_lock lock(app->pauseMtx);
        app->pauseCnd.wait(lock);
    }

    pw_stream_queue_buffer(app->pw.stream, b);
}
