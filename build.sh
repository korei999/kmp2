#!/bin/sh
set -e
set -x

debug()
{
    rm -rf build
    if meson setup build -Db_sanitize=address,undefined --buildtype=debug
    then
        ninja -C build/
    fi
}

release()
{
    rm -rf build
    if meson setup build
    then
        ninja -C build/
    fi
}

run()
{
    BIN=kmp

    if ninja -C build/
    then
        echo ""
        # ASAN_OPTIONS=detect_leaks=1 LSAN_OPTIONS=suppressions=leaks.txt ./build/$BIN "$@" # 2> /tmp/$BIN-dbg.txt
        # ASAN_OPTIONS=halt_on_error=0 ./build/$BIN "$@" # 2> /tmp/$BIN-dbg.txt
        # ./build/$BIN "$@" # 2> /tmp/$BIN-dbg.txt
        ./build/$BIN "$@" 2> /tmp/$BIN-dbg.txt
    fi
}

cd $(dirname $0)

case "$1" in
    debug) debug ;;
    run) run "$@" ;;
    *) release ;;
esac
