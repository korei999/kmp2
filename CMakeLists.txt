cmake_minimum_required(VERSION 3.8)

include(FindPkgConfig)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_COLOR_DIAGNOSTICS ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
endif()

project(kmp2
        VERSION 0.2 
        LANGUAGES CXX)

add_executable(kmp
               src/main.cc
               src/app.cc
               src/input.cc
               src/play.cc
               src/search.cc
               src/song.cc
               src/utils.cc)

if (CMAKE_BUILD_TYPE MATCHES "Asan")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=undefined -fsanitize=address")
    target_link_options(kmp BEFORE PUBLIC -fsanitize=undefined PUBLIC -fsanitize=address)
endif()

message(STATUS "Build type: '${CMAKE_BUILD_TYPE}'")

set_property(TARGET kmp PROPERTY CXX_STANDARD 20)
add_definitions("-DLOGS") # used for LOG_* macros

pkg_check_modules(SNDFILE REQUIRED sndfile)
pkg_check_modules(PIPEWIRE REQUIRED libpipewire-0.3)
pkg_check_modules(NCURSES REQUIRED ncursesw)

target_include_directories(kmp PUBLIC ${SNDFILE_INCLUDE_DIRS} ${PIPEWIRE_INCLUDE_DIRS} ${NCURSES_INCLUDE_DIRS})
target_link_libraries(kmp PUBLIC ${SNDFILE_LIBRARIES} ${PIPEWIRE_LIBRARIES} ${NCURSES_LIBRARIES})

pkg_check_modules(SYSTEMD libsystemd)
if (${SYSTEMD_FOUND})
    target_include_directories(kmp PUBLIC ${SYSTEMD_INCLUDE_DIRS})
    target_link_libraries(kmp PUBLIC ${SYSTEMD_LIBRARIES})
    target_sources(kmp PRIVATE "src/mpris.cc")
    add_definitions("-DMPRIS_LIB")
else()
    pkg_check_modules(BASU basu)
    if (${BASU_FOUND})
        target_include_directories(kmp PUBLIC ${BASU_INCLUDE_DIRS})
        target_link_libraries(kmp PUBLIC ${BASU_LIBRARIES})
        target_sources(kmp PRIVATE "src/mpris.cc")
        add_definitions("-DMPRIS_LIB" "-DBASU_LIB")
    endif()
endif()

pkg_check_modules(FMT fmt)
if (${FMT_FOUND})
    add_definitions("-DFMT_LIB")
    target_include_directories(kmp PUBLIC ${FMT_INCLUDE_DIRS})
    target_link_libraries(kmp PUBLIC ${FMT_LIBRARIES})
endif()

install(TARGETS kmp DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
