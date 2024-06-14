#pragma once
#include "ultratypes.h"

#include <cassert>
#include <string_view>
#include <vector>
#include <iostream>
#include <complex>
#include <valarray>

#ifdef FMT_LIB
    #include <fmt/format.h>
    #define COUT std::cout << fmt::format
    #define CERR std::cerr << fmt::format
    #define FMT fmt::format
#else
    #include <format>
    #define COUT std::cout << std::format
    #define CERR std::cerr << std::format
    #define FMT std::format
#endif

namespace utils
{

enum class sev : int
{
    ok,
    good,
    warn,
    bad,
    fatal
};

std::vector<char> loadFileToCharArray(std::string_view path, size_t addBytes = 1);
f64 timeNow();
int rngGet(int min, int max);
int rngGet();
f32 rngGet(f32 min, f32 max);

const std::string_view severityStr[(int)sev::fatal + 1] {
    "",
    "[GOOD]",
    "[WARNING]",
    "[BAD]",
    "[FATAL]"
};

#define NOP void(0)
#define LEN(A) (sizeof(A) / sizeof(A[0]))
#define ODD(A) (A & 1)
#define EVEN(A) (!ODD(A))
#define ROUND(A) ((int)((A < 0) ? (A - 0.5) : (A + 0.5)))
#define SQ(A) (A * A)

#ifdef LOGS
    #define LOG(severity, ...)                                                                                         \
        do                                                                                                             \
        {                                                                                                              \
            CERR("{}({}): {} ", __FILE__, __LINE__, utils::severityStr[(int)severity]);                                \
            CERR(__VA_ARGS__);                                                                                         \
            switch ((int)severity)                                                                                     \
            {                                                                                                          \
                case (int)utils::sev::fatal:                                                                           \
                    abort();                                                                                           \
                    break;                                                                                             \
                                                                                                                       \
                case (int)utils::sev::bad:                                                                             \
                    exit(1);                                                                                           \
                    break;                                                                                             \
                                                                                                                       \
                default:                                                                                               \
                    break;                                                                                             \
            }                                                                                                          \
        } while (0)
    #define LOG_OK(...) LOG(utils::sev::ok, __VA_ARGS__)
    #define LOG_GOOD(...) LOG(utils::sev::good, __VA_ARGS__)
    #define LOG_WARN(...) LOG(utils::sev::warn, __VA_ARGS__)
    #define LOG_BAD(...) LOG(utils::sev::bad, __VA_ARGS__)
    #define LOG_FATAL(...) LOG(utils::sev::fatal, __VA_ARGS__)
#else
    #define LOG(severity, ...) NOP
#endif

constexpr inline u64
hashFNV(std::string_view str)
{
    u64 hash = 0xCBF29CE484222325;
    for (u64 i = 0; i < (u64)str.size(); i++)
        hash = (hash ^ (u64)str[i]) * 0x100000001B3;
    return hash;
}

constexpr std::string
removePath(std::string_view str)
{
    return std::string(str.substr(str.find_last_of("/") + 1, str.size()));
}

constexpr std::wstring
removePath(std::wstring_view str)
{
    return std::wstring(str.substr(str.find_last_of(L"/") + 1, str.size()));
}

constexpr int
round(double x)
{
    return x < 0 ? x - 0.5 : x + 0.5;
}

/* https://rosettacode.org/wiki/Fast_Fourier_transform#C++ */
static inline void
fft(std::valarray<std::complex<f32>>& a)
{
    const size_t n = a.size();
    if (n <= 1) return;

    /* divide */
    std::valarray<std::complex<f32>> even = a[std::slice(0, n/2, 2)];
    std::valarray<std::complex<f32>> odd  = a[std::slice(1, n/2, 2)];

    /* conquer */
    fft(even);
    fft(odd);

    /* combine */
    for (size_t k = 0; k < n/2; ++k)
    {
        f32 tt = (f32)k / n;

        std::complex<f32> t  = std::polar(1.0f, -2.0f * (f32)M_PI * tt) * odd[k];
        a[k      ] = even[k] + t;
        a[k + n/2] = even[k] - t;
    }
}

} /* namespace utils */
