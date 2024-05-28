/* NOTE: thanks cmus! https://github.com/cmus/cmus/blob/master/mpris.c */

#include "mpris.hh"
#include "app.hh"

#include <systemd/sd-bus.h>

#define MPRIS_PROP(name, type, read) \
	SD_BUS_PROPERTY(name, type, read, 0, \
			SD_BUS_VTABLE_PROPERTY_EMITS_CHANGE)

#define MPRIS_WPROP(name, type, read, write) \
	SD_BUS_WRITABLE_PROPERTY(name, type, read, write, 0, \
			SD_BUS_VTABLE_PROPERTY_EMITS_CHANGE)

namespace mpris
{

static sd_bus *pBus {};
static int mpris_fd = -1;

static int
msgIgnore([[maybe_unused]] sd_bus_message* m,
          [[maybe_unused]] void* _data,
          [[maybe_unused]] sd_bus_error* _retError)
{
    return sd_bus_reply_method_return(m, "");
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
    auto p = (app::PipeWirePlayer*)_data;
    const char* s = p->bPaused ? "Paused" : "Playing";
    return sd_bus_message_append_basic(reply, 's', s);
}

/* TODO: implement this! */
static int
metadata([[maybe_unused]] sd_bus* _bus,
         [[maybe_unused]] const char* _path,
         [[maybe_unused]] const char* _interface,
         [[maybe_unused]] const char* _property,
         [[maybe_unused]] sd_bus_message* reply,
         [[maybe_unused]] void* _data,
         [[maybe_unused]] sd_bus_error* _retError)
{
    // sd_bus_message_open_container(reply, 'a', "{sv}");

    // struct track_info* ti = player_info.ti;
    // if (ti)
    // {
        // char buf[] = "/1122334455667788";
        // sprintf(buf, "/%" PRIx64, ti->uid);
        // mpris_msg_append_so_dict(reply, "mpris:trackid", buf);

        // int64_t dur;
        // dur *= 1000 * 1000;
        // mpris_msg_append_sx_dict(reply, "mpris:length", dur);

        // The dbus connection closes if invalid data is sent.
        // As a *temporary* fix, ensure all strings are encoded in utf8.
        // if (ti->artist)
        // {
            // char corrected[u_str_print_size(ti->artist)];
            // u_to_utf8(corrected, ti->artist);
            // CK(mpris_msg_append_sas_dict(reply, "xesam:artist", corrected));
        // }
        // if (ti->title)
        // {
            // char corrected[u_str_print_size(ti->title)];
            // u_to_utf8(corrected, ti->title);
            // CK(mpris_msg_append_ss_dict(reply, "xesam:title", corrected));
        // }
        // else
        // {
            // char corrected[u_str_print_size(path_basename(ti->filename))];
            // u_to_utf8(corrected, path_basename(ti->filename));
            // CK(mpris_msg_append_ss_dict(reply, "xesam:title", corrected));
        // }
        // if (ti->album)
        // {
            // char corrected[u_str_print_size(ti->album)];
            // u_to_utf8(corrected, ti->album);
            // CK(mpris_msg_append_ss_dict(reply, "xesam:album", corrected));
        // }
        // if (ti->albumartist)
        // {
            // char corrected[u_str_print_size(ti->albumartist)];
            // u_to_utf8(corrected, ti->albumartist);
            // CK(mpris_msg_append_sas_dict(reply, "xesam:albumArtist", corrected));
        // }
        // if (ti->genre)
        // {
            // char corrected[u_str_print_size(ti->genre)];
            // u_to_utf8(corrected, ti->genre);
            // CK(mpris_msg_append_sas_dict(reply, "xesam:genre", corrected));
        // }
        // if (ti->comment)
        // {
            // char corrected[u_str_print_size(ti->comment)];
            // u_to_utf8(corrected, ti->comment);
            // CK(mpris_msg_append_sas_dict(reply, "xesam:comment", corrected));
        // }
        // if (ti->bpm != -1)
            // CK(mpris_msg_append_si_dict(reply, "xesam:audioBPM", ti->bpm));
        // if (ti->tracknumber != -1)
            // CK(mpris_msg_append_si_dict(reply, "xesam:trackNumber", ti->tracknumber));
        // if (ti->discnumber != -1)
            // CK(mpris_msg_append_si_dict(reply, "xesam:discNumber", ti->discnumber));
        // if (is_http_url(ti->filename))
            // CK(mpris_msg_append_ss_dict(reply, "cmus:stream_title", get_stream_title()));
    // }

    // CK(sd_bus_message_close_container(reply));
    return 0;
}

static const sd_bus_vtable vTmediaPlayer2[] {
	SD_BUS_VTABLE_START(0),
	// SD_BUS_METHOD("Raise", "", "", mpris_raise_vte, 0),
	SD_BUS_METHOD("Quit", "", "", msgIgnore, 0),
	MPRIS_PROP("CanQuit", "b", readFalse),
	// MPRIS_WPROP("Fullscreen", "b", readFalse, mpris_write_ignore),
	MPRIS_PROP("CanSetFullscreen", "b", readFalse),
	// MPRIS_PROP("CanRaise", "b", mpris_can_raise_vte),
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
	// MPRIS_WPROP("Rate", "d", mpris_rate, mpris_write_ignore),
	// MPRIS_WPROP("Shuffle", "b", mpris_shuffle, mpris_set_shuffle),
	// MPRIS_WPROP("Volume", "d", mpris_volume, mpris_set_volume),
	// SD_BUS_PROPERTY("Position", "x", mpris_position, 0, 0),
	// MPRIS_PROP("MinimumRate", "d", mpris_rate),
	// MPRIS_PROP("MaximumRate", "d", mpris_rate),
	MPRIS_PROP("CanGoNext", "b", readTrue),
	MPRIS_PROP("CanGoPrevious", "b", readTrue),
	MPRIS_PROP("CanPlay", "b", readTrue),
	MPRIS_PROP("CanPause", "b", readTrue),
	// MPRIS_PROP("CanSeek", "b", mpris_read_true),
	// SD_BUS_PROPERTY("CanControl", "b", mpris_read_true, 0, 0),
	// MPRIS_PROP("Metadata", "a{sv}", metadata),
	// SD_BUS_SIGNAL("Seeked", "x", 0),
	SD_BUS_VTABLE_END,
};

void
init(app::PipeWirePlayer* p)
{
	int res = 0;

	res = sd_bus_default_user(&pBus);
	if (res < 0) goto out;

	res = sd_bus_add_object_vtable(pBus,
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

	res = sd_bus_request_name(pBus,
                              "org.mpris.MediaPlayer2.kmp",
                              0);

	mpris_fd = sd_bus_get_fd(pBus);

out:
	if (res < 0)
    {
		sd_bus_unref(pBus);
		pBus = nullptr;
		mpris_fd = -1;

		LOG_FATAL("{}: {}\n", strerror(-res), "mpris::Init error");
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
    pBus = NULL;
    mpris_fd = -1;
}

} /* namespace mpris */
