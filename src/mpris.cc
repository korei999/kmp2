/* NOTE: taken from cmus https://github.com/cmus/cmus/blob/master/mpris.c */

#include "mpris.hh"

#ifdef BASU
#include <basu/sd-bus.h>
#else
#include <systemd/sd-bus.h>
#endif

#define MPRIS_PROP(name, type, read) SD_BUS_PROPERTY(name, type, read, 0, SD_BUS_VTABLE_PROPERTY_EMITS_CHANGE)

#define MPRIS_WPROP(name, type, read, write)                                                                           \
    SD_BUS_WRITABLE_PROPERTY(name, type, read, write, 0, SD_BUS_VTABLE_PROPERTY_EMITS_CHANGE)

#define CK(v)                                                                                                          \
    do                                                                                                                 \
    {                                                                                                                  \
        int tmp = (v);                                                                                                 \
        if (tmp < 0)                                                                                                   \
            return tmp;                                                                                                \
    } while (0)

namespace mpris
{

static sd_bus *pBus {};
static int fdMpris = -1;

static int
msgAppendDictSAS(sd_bus_message* m, const char* a, const char* b)
{
    CK(sd_bus_message_open_container(m, 'e', "sv"));
    CK(sd_bus_message_append_basic(m, 's', a));
    CK(sd_bus_message_open_container(m, 'v', "as"));
    CK(sd_bus_message_open_container(m, 'a', "s"));
    CK(sd_bus_message_append_basic(m, 's', b));
    CK(sd_bus_message_close_container(m));
    CK(sd_bus_message_close_container(m));
    CK(sd_bus_message_close_container(m));
    return 0;
}

static int
msgAppendDictSimple(sd_bus_message* m, const char* tag, char type, const void* val)
{
    const char s[] = {type, 0};
    CK(sd_bus_message_open_container(m, 'e', "sv"));
    CK(sd_bus_message_append_basic(m, 's', tag));
    CK(sd_bus_message_open_container(m, 'v', s));
    CK(sd_bus_message_append_basic(m, type, val));
    CK(sd_bus_message_close_container(m));
    CK(sd_bus_message_close_container(m));
    return 0;
}

static int
msgAppendDictSS(sd_bus_message* m, const char* a, const char* b)
{
    return msgAppendDictSimple(m, a, 's', b);
}

static int
msgIgnore([[maybe_unused]] sd_bus_message* m,
          [[maybe_unused]] void* _data,
          [[maybe_unused]] sd_bus_error* _retError)
{
    return sd_bus_reply_method_return(m, "");
}

static int
writeIgnore([[maybe_unused]] sd_bus* _bus,
                   [[maybe_unused]] const char* _path,
                   [[maybe_unused]] const char* _interface,
                   [[maybe_unused]] const char* _property,
                   [[maybe_unused]] sd_bus_message* value,
                   [[maybe_unused]] void* _data,
                   [[maybe_unused]] sd_bus_error* _retError)
{
    return sd_bus_reply_method_return(value, "");
}

static int
readFalse([[maybe_unused]] sd_bus* _bus,
          [[maybe_unused]] const char* _path,
          [[maybe_unused]] const char* _interface,
          [[maybe_unused]] const char* _property,
          [[maybe_unused]] sd_bus_message* reply,
          [[maybe_unused]] void* _data,
          [[maybe_unused]] sd_bus_error* _retError)
{
    u32 b = 0;
    return sd_bus_message_append_basic(reply, 'b', &b);
}

static int
togglePause([[maybe_unused]] sd_bus_message* m,
            [[maybe_unused]] void* _data,
            [[maybe_unused]] sd_bus_error* _retError)
{
    auto p = (app::PipeWirePlayer*)_data;
    p->togglePause();
    return sd_bus_reply_method_return(m, "");
}

static int
next([[maybe_unused]] sd_bus_message* m,
     [[maybe_unused]] void* _data,
     [[maybe_unused]] sd_bus_error* _retError)
{
    auto p = (app::PipeWirePlayer*)_data;
    p->bNext = true;
    return sd_bus_reply_method_return(m, "");
}

static int
prev([[maybe_unused]] sd_bus_message* m,
     [[maybe_unused]] void* _data,
     [[maybe_unused]] sd_bus_error* _retError)
{
    auto p = (app::PipeWirePlayer*)_data;
    p->bPrev = true;
    return sd_bus_reply_method_return(m, "");
}

static int
pause([[maybe_unused]] sd_bus_message* m,
      [[maybe_unused]] void* _data,
      [[maybe_unused]] sd_bus_error* _retError)
{
    auto p = (app::PipeWirePlayer*)_data;
    p->pause();
    return sd_bus_reply_method_return(m, "");
}

/* NOTE: same as pause */
static int
stop([[maybe_unused]] sd_bus_message* m,
     [[maybe_unused]] void* _data,
     [[maybe_unused]] sd_bus_error* _retError)
{
    return pause(m, _data, _retError);
}

static int
resume([[maybe_unused]] sd_bus_message* m,
       [[maybe_unused]] void* _data,
       [[maybe_unused]] sd_bus_error* _retError)
{
    auto p = (app::PipeWirePlayer*)_data;
    p->resume();
    return sd_bus_reply_method_return(m, "");
}

static int
readTrue([[maybe_unused]] sd_bus* _bus,
         [[maybe_unused]] const char* _path,
         [[maybe_unused]] const char* _interface,
         [[maybe_unused]] const char* _property,
         [[maybe_unused]] sd_bus_message* reply,
         [[maybe_unused]] void* _data,
         [[maybe_unused]] sd_bus_error* _retError)
{
    u32 b = 1;
    return sd_bus_message_append_basic(reply, 'b', &b);
}

static int
identity([[maybe_unused]] sd_bus* _bus,
         [[maybe_unused]] const char* _path,
         [[maybe_unused]] const char* _interface,
         [[maybe_unused]] const char* _property,
         [[maybe_unused]] sd_bus_message* reply,
         [[maybe_unused]] void* _data,
         [[maybe_unused]] sd_bus_error* _retError)
{
    const char* id = "kmp";
    return sd_bus_message_append_basic(reply, 's', id);
}

static int
playbackStatus([[maybe_unused]] sd_bus* _bus,
               [[maybe_unused]] const char* _path,
               [[maybe_unused]] const char* _interface,
               [[maybe_unused]] const char* _property,
               [[maybe_unused]] sd_bus_message* reply,
               [[maybe_unused]] void* _data,
               [[maybe_unused]] sd_bus_error* _retError)
{
    const auto p = (app::PipeWirePlayer*)_data;
    const char* s = p->bPaused ? "Paused" : "Playing"; /* NOTE: "Stopped" ignored */
    return sd_bus_message_append_basic(reply, 's', s);
}

static int
volume([[maybe_unused]] sd_bus* _bus,
       [[maybe_unused]] const char* _path,
       [[maybe_unused]] const char* _interface,
       [[maybe_unused]] const char* _property,
       [[maybe_unused]] sd_bus_message* reply,
       [[maybe_unused]] void* _data,
       [[maybe_unused]] sd_bus_error* _retError)
{
    const auto p = (app::PipeWirePlayer*)_data;
    f64 vol = p->volume;
    return sd_bus_message_append_basic(reply, 'd', &vol);
}

static int
setVolume([[maybe_unused]] sd_bus* _bus,
          [[maybe_unused]] const char* _path,
          [[maybe_unused]] const char* _interface,
          [[maybe_unused]] const char* _property,
          [[maybe_unused]] sd_bus_message* value,
          [[maybe_unused]] void* _data,
          [[maybe_unused]] sd_bus_error* _retError)
{
    auto p = (app::PipeWirePlayer*)_data;
    f64 vol;
    CK(sd_bus_message_read_basic(value, 'd', &vol));
    p->setVolume(vol);

    return sd_bus_reply_method_return(value, "");
}

static int
position([[maybe_unused]] sd_bus* _bus,
         [[maybe_unused]] const char* _path,
         [[maybe_unused]] const char* _interface,
         [[maybe_unused]] const char* _property,
         [[maybe_unused]] sd_bus_message* reply,
         [[maybe_unused]] void* _data,
         [[maybe_unused]] sd_bus_error* _retError)
{
    auto p = (app::PipeWirePlayer*)_data;
    u64 t = (p->pcmPos/p->pw.channels) / p->pw.sampleRate;
    t *= 1000 * 1000;

    return sd_bus_message_append_basic(reply, 'x', &t);
}

static int
rate([[maybe_unused]] sd_bus* _bus,
     [[maybe_unused]] const char* _path,
     [[maybe_unused]] const char* _interface,
     [[maybe_unused]] const char* _property,
     [[maybe_unused]] sd_bus_message* reply,
     [[maybe_unused]] void* _data,
     [[maybe_unused]] sd_bus_error* _retError)
{
    const auto p = (app::PipeWirePlayer*)_data;
    f64 mul = (f64)p->pw.sampleRate / (f64)p->pw.origSampleRate;
    return sd_bus_message_append_basic(reply, 'd', &mul);
}

static int
minRate([[maybe_unused]] sd_bus* _bus,
        [[maybe_unused]] const char* _path,
        [[maybe_unused]] const char* _interface,
        [[maybe_unused]] const char* _property,
        [[maybe_unused]] sd_bus_message* reply,
        [[maybe_unused]] void* _data,
        [[maybe_unused]] sd_bus_error* _retError)
{
    const auto p = (app::PipeWirePlayer*)_data;
    f64 mul = (f64)defaults::minSampleRate / (f64)p->pw.origSampleRate;
    return sd_bus_message_append_basic(reply, 'd', &mul);
}

static int
maxRate([[maybe_unused]] sd_bus* _bus,
        [[maybe_unused]] const char* _path,
        [[maybe_unused]] const char* _interface,
        [[maybe_unused]] const char* _property,
        [[maybe_unused]] sd_bus_message* reply,
        [[maybe_unused]] void* _data,
        [[maybe_unused]] sd_bus_error* _retError)
{
    const auto p = (app::PipeWirePlayer*)_data;
    f64 mul = (f64)defaults::maxSampleRate / (f64)p->pw.origSampleRate;
    return sd_bus_message_append_basic(reply, 'd', &mul);
}

static int
shuffle([[maybe_unused]] sd_bus* _bus,
        [[maybe_unused]] const char* _path,
        [[maybe_unused]] const char* _interface,
        [[maybe_unused]] const char* _property,
        [[maybe_unused]] sd_bus_message* reply,
        [[maybe_unused]] void* _data,
        [[maybe_unused]] sd_bus_error* _retError)
{
    const uint32_t s = 0;
    return sd_bus_message_append_basic(reply, 'b', &s);
}

static int
metadata([[maybe_unused]] sd_bus* _bus,
         [[maybe_unused]] const char* _path,
         [[maybe_unused]] const char* _interface,
         [[maybe_unused]] const char* _property,
         [[maybe_unused]] sd_bus_message* reply,
         [[maybe_unused]] void* _data,
         [[maybe_unused]] sd_bus_error* _retError)
{
    const auto p = (app::PipeWirePlayer*)_data;

    CK(sd_bus_message_open_container(reply, 'a', "{sv}"));

    if (!p->info.title.empty())
        CK(msgAppendDictSS(reply, "xesam:title", p->info.title.data()));
    if (!p->info.album.empty())
        CK(msgAppendDictSS(reply, "xesam:album", p->info.album.data()));
    if (!p->info.artist.empty())
        CK(msgAppendDictSAS(reply, "xesam:artist", p->info.artist.data()));

    CK(sd_bus_message_close_container(reply));

    return 0;
}

static const sd_bus_vtable vTmediaPlayer2[] {
    SD_BUS_VTABLE_START(0),
    // SD_BUS_METHOD("Raise", "", "", raiseVte, 0),
    SD_BUS_METHOD("Quit", "", "", msgIgnore, 0),
    MPRIS_PROP("CanQuit", "b", readFalse),
    MPRIS_WPROP("Fullscreen", "b", readFalse, writeIgnore),
    MPRIS_PROP("CanSetFullscreen", "b", readFalse),
    MPRIS_PROP("CanRaise", "b", readFalse),
    MPRIS_PROP("HasTrackList", "b", readFalse),
    MPRIS_PROP("Identity", "s", identity),
    // MPRIS_PROP("SupportedUriSchemes", "as", mpris_uri_schemes),
    // MPRIS_PROP("SupportedMimeTypes", "as", mpris_mime_types),
    SD_BUS_VTABLE_END,
};

static const sd_bus_vtable vTmediaPlayer2Player[] = {
    SD_BUS_VTABLE_START(0),
    SD_BUS_METHOD("Next", "", "", next, 0),
    SD_BUS_METHOD("Previous", "", "", prev, 0),
    SD_BUS_METHOD("Pause", "", "", pause, 0),
    SD_BUS_METHOD("PlayPause", "", "", togglePause, 0),
    SD_BUS_METHOD("Stop", "", "", stop, 0),
    SD_BUS_METHOD("Play", "", "", resume, 0),
    // SD_BUS_METHOD("Seek", "x", "", mpris_seek, 0),
    // SD_BUS_METHOD("SetPosition", "ox", "", mpris_seek_abs, 0),
    // SD_BUS_METHOD("OpenUri", "s", "", mpris_play_file, 0),
    MPRIS_PROP("PlaybackStatus", "s", playbackStatus),
    // MPRIS_WPROP("LoopStatus", "s", mpris_loop_status, mpris_set_loop_status),
    MPRIS_WPROP("Rate", "d", rate, writeIgnore), /* this one is not used by playerctl afaik */
    MPRIS_WPROP("Shuffle", "b", shuffle, writeIgnore),
    MPRIS_WPROP("Volume", "d", volume, setVolume),
    SD_BUS_PROPERTY("Position", "x", position, 0, 0),
    MPRIS_PROP("MinimumRate", "d", minRate),
    MPRIS_PROP("MaximumRate", "d", maxRate),
    MPRIS_PROP("CanGoNext", "b", readTrue),
    MPRIS_PROP("CanGoPrevious", "b", readTrue),
    MPRIS_PROP("CanPlay", "b", readTrue),
    MPRIS_PROP("CanPause", "b", readTrue),
    MPRIS_PROP("CanSeek", "b", readFalse),
    SD_BUS_PROPERTY("CanControl", "b", readTrue, 0, 0),
    MPRIS_PROP("Metadata", "a{sv}", metadata),
    SD_BUS_SIGNAL("Seeked", "x", 0),
    SD_BUS_VTABLE_END,
};

void
init(app::PipeWirePlayer* p)
{
    int res = 0;

    res = sd_bus_default_user(&pBus);
    if (res < 0)
        goto out;

    res =
        sd_bus_add_object_vtable(pBus,
                                 nullptr,
                                 "/org/mpris/MediaPlayer2",
                                 "org.mpris.MediaPlayer2",
                                 vTmediaPlayer2,
                                 p);
    if (res < 0) goto out;

    res = sd_bus_add_object_vtable(pBus,
                                   nullptr,
                                   "/org/mpris/MediaPlayer2",
                                   "org.mpris.MediaPlayer2.Player",
                                   vTmediaPlayer2Player,
                                   p);
    if (res < 0) goto out;

    res = sd_bus_request_name(pBus, "org.mpris.MediaPlayer2.kmp", 0);
    fdMpris = sd_bus_get_fd(pBus);

out:
    if (res < 0)
    {
        sd_bus_unref(pBus);
        pBus = nullptr;
        fdMpris = -1;

        LOG_WARN("{}: {}\n", strerror(-res), "mpris::init error");
    }
}

void
process(app::PipeWirePlayer* p)
{
    if (pBus)
    {
        while (sd_bus_process(pBus, nullptr) > 0 && !p->bFinished)
            ;
    }
}

void
clean()
{
    sd_bus_unref(pBus);
    pBus = nullptr;
    fdMpris = -1;
}

} /* namespace mpris */
