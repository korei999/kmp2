#include "app.hh"
#include "play.hh"
#include "utils.hh"

#include <spa/param/audio/format-utils.h>

using namespace app;

static const pw_stream_events streamEvents {
    .version = PW_VERSION_STREAM_EVENTS,
    .process = play::onProcess,
};

App::App(int argc, char* argv[])
{
    setupPW(argc, argv);
}

App::~App()
{
    pw_core_disconnect(pw.core);
}

void
App::setupPW(int argc, char* argv[])
{
    for (int i = 1; i < argc; i++)
    {
        std::string s = argv[i];

        if (s.ends_with(".wav"))
            songs.push_back(std::move(s));
    }

    const spa_pod* params[1] {};
    spa_pod_builder b = SPA_POD_BUILDER_INIT(pw.buffer, sizeof(pw.buffer));

    pw_init(&argc, &argv);

    CERR("Compiled with libpipewire {}\n""Linked with libpipewire {}\n",
         pw_get_headers_version(), pw_get_library_version());

    pw.loop = pw_main_loop_new(nullptr);

    pw.stream = pw_stream_new_simple(pw_main_loop_get_loop(pw.loop),
                                         "audio-src",
                                         pw_properties_new(PW_KEY_MEDIA_TYPE,
                                                           "Audio",
                                                           PW_KEY_MEDIA_CATEGORY,
                                                           "Playback",
                                                           PW_KEY_MEDIA_ROLE,
                                                           "Music",
                                                           nullptr),
                                         &streamEvents,
                                         this);


    spa_audio_info_raw info {
        .format = SPA_AUDIO_FORMAT_S16,
        .rate = DEFAULT_RATE,
        .channels = DEFAULT_CHANNELS
    };

    params[0] = spa_format_audio_raw_build(&b, SPA_PARAM_EnumFormat, &info);

    pw_stream_connect(pw.stream,
                      PW_DIRECTION_OUTPUT,
                      PW_ID_ANY,
                      (enum pw_stream_flags)(PW_STREAM_FLAG_AUTOCONNECT |
                                             PW_STREAM_FLAG_MAP_BUFFERS |
                                             PW_STREAM_FLAG_ASYNC),
                      params,
                      1);

    pw.core = pw_stream_get_core(pw.stream);
}

void
App::playAll()
{
    auto wave = loadFileToCharArray(songs.back());
    pcmData = (s16*)wave.data();
    pcmSize = wave.size() / sizeof(s16);

    pw_main_loop_run(pw.loop);

    /* in this order */
    pw_stream_destroy(pw.stream);
    pw_main_loop_destroy(pw.loop);
}
