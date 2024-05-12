#include "song.hh"
#include "utils.hh"

namespace song
{

Info::Info(std::string_view _path, const SndfileHandle& h)
{
    const char* _title = h.getString(SF_STR_TITLE);
    const char* _artist = h.getString(SF_STR_ARTIST);
    const char* _album = h.getString(SF_STR_ALBUM);

    path = _path;

    if (_title) title = _title;
    else title = utils::removePath(_path);

    auto set = [](const char* s) -> std::string {
        if (s) return s;
        else return {};
    };

    artist = set(_artist);
    album = set(_album);
}

} /* namespace song */
