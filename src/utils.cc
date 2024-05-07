#include "utils.hh"

#include <fstream>
#include <chrono>
#include <random>
#include <mutex>
#include <climits>

namespace utils
{

static std::mt19937 rngCreate();

static std::mt19937 mt {rngCreate()};
std::mutex logMtx;

static std::mt19937
rngCreate()
{
    std::random_device rd {};
    std::seed_seq ss {
        (std::seed_seq::result_type)(timeNow()), rd(), rd(), rd(), rd(), rd(), rd(), rd()
    };

    return std::mt19937(ss);
}

int
rngGet(int min, int max)
{
    return std::uniform_int_distribution {min, max}(mt);
}

int
rngGet()
{
    return std::uniform_int_distribution {INT_MIN, INT_MAX}(mt);
}

f32
rngGet(f32 min, f32 max)
{
    return std::uniform_real_distribution {min, max}(mt);
}

static std::mutex fileMtx;

std::vector<char>
loadFileToCharArray(std::string_view path, size_t addBytes)
{
    std::lock_guard lock(fileMtx);

    std::ifstream file(path.data(), std::ios::in | std::ios::ate | std::ios::binary);
    if (!file.is_open())
        LOG(sev::fatal, "failed to open file: '{}'\n", path);

    size_t fileSize = (size_t)file.tellg();
    std::vector<char> buffer(fileSize + addBytes, '\0');

    file.seekg(0);
    file.read(buffer.data(), fileSize);
    file.close();

    return buffer;
}

f64
timeNow()
{
    typedef std::chrono::high_resolution_clock Time;
    typedef std::chrono::duration<f64> Fsec;

    Fsec fs = Time::now().time_since_epoch();
    return fs.count();
}

std::string
replaceFileSuffixInPath(std::string_view path, std::string* suffix)
{
#ifdef _WIN32
    suffix->back() = '\0';
#endif

    auto lastSlash = path.find_last_of("/");
    std::string pathToMtl {path.begin(), path.begin() + lastSlash};

    return {pathToMtl + "/" + (*suffix)};
}

} /* namespace utils */
