#pragma once

#include <sndfile.hh>
#include <string>

namespace song
{

struct Info
{
    std::string path {};
    std::string title {};
    std::string artist {};
    std::string album {};

    Info() = default;
    Info(std::string_view _path, const SndfileHandle& h);
};

} /* namespace song */
