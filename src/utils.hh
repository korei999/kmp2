#pragma once
#include "ultratypes.h"

#include <mutex>
#include <string_view>
#include <vector>
#include <iostream>
#include <format>

enum LogSeverity : int
{
    OK,
    GOOD,
    WARN,
    BAD,
    FATAL
};

std::vector<char> loadFileToCharArray(std::string_view path, size_t addBytes = 1);
f64 timeNow();
int rngGet(int min, int max);
int rngGet();
f32 rngGet(f32 min, f32 max);
std::string replaceFileSuffixInPath(std::string_view str, std::string* suffix);

const std::string_view severityStr[FATAL + 1] {
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

#define COUT std::cout << std::format
#define CERR std::cerr << std::format

extern std::mutex logMtx;

#ifdef LOGS
#    define LOG(severity, ...)                                                                                         \
        do                                                                                                             \
        {                                                                                                              \
            std::lock_guard printLock(logMtx);                                                                         \
            CERR("{}({}): {} ", __FILE__, __LINE__, severityStr[severity]);                                            \
            CERR(__VA_ARGS__);                                                                                         \
            switch (severity)                                                                                          \
            {                                                                                                          \
                case FATAL:                                                                                            \
                    abort();                                                                                           \
                    break;                                                                                             \
                                                                                                                       \
                case BAD:                                                                                              \
                    exit(1);                                                                                           \
                    break;                                                                                             \
                                                                                                                       \
                default:                                                                                               \
                    break;                                                                                             \
            }                                                                                                          \
        } while (0)
#else
#    define LOG(severity, ...) NOP /* noop */
#endif

constexpr inline u64
hashFNV(std::string_view str)
{
    u64 hash = 0xCBF29CE484222325;
    for (u64 i = 0; i < (u64)str.size(); i++)
        hash = (hash ^ (u64)str[i]) * 0x100000001B3;
    return hash;
}
