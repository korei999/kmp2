cmake_minimum_required(VERSION 3.8)

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
    target_link_options(kmp BEFORE PRIVATE -fsanitize=undefined PRIVATE -fsanitize=address)
endif()

message(STATUS "CMAKE_BUILD_TYPE: '${CMAKE_BUILD_TYPE}'")

set_property(TARGET kmp PROPERTY CXX_STANDARD 20)
target_compile_definitions(kmp PRIVATE "-DLOGS") # used for LOG_* macros

find_package(PkgConfig REQUIRED)
pkg_check_modules(PKGS REQUIRED sndfile libpipewire-0.3 ncursesw)

target_include_directories(kmp PRIVATE ${PKGS_INCLUDE_DIRS})
target_link_libraries(kmp PRIVATE ${PKGS_LIBRARIES})

pkg_check_modules(SYSTEMD libsystemd)
if (${SYSTEMD_FOUND})
    target_include_directories(kmp PRIVATE ${SYSTEMD_INCLUDE_DIRS})
    target_link_libraries(kmp PRIVATE ${SYSTEMD_LIBRARIES})
    target_sources(kmp PRIVATE "src/mpris.cc")
    target_compile_definitions(kmp PRIVATE "-DMPRIS_LIB")
else()
    pkg_check_modules(BASU basu)
    if (${BASU_FOUND})
        target_include_directories(kmp PRIVATE ${BASU_INCLUDE_DIRS})
        target_link_libraries(kmp PRIVATE ${BASU_LIBRARIES})
        target_sources(kmp PRIVATE "src/mpris.cc")
        target_compile_definitions(kmp PRIVATE "-DMPRIS_LIB" "-DBASU_LIB")
    endif()
endif()

pkg_check_modules(FMT fmt)
if (${FMT_FOUND})
    add_definitions("-DFMT_LIB")
    target_include_directories(kmp PRIVATE ${FMT_INCLUDE_DIRS})
    target_link_libraries(kmp PRIVATE ${FMT_LIBRARIES})
endif()

install(TARGETS kmp DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
