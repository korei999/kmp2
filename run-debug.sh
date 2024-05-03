#!/bin/sh
set -e
set -x

BIN=kmp

cd $(dirname $0)
if make debug
then
    echo ""
    # ASAN_OPTIONS=detect_leaks=1 LSAN_OPTIONS=suppressions=leaks.txt ./build/$BIN "$@" # 2> /tmp/$BIN-dbg.txt
    # ASAN_OPTIONS=halt_on_error=0 ./build/$BIN "$@" # 2> /tmp/$BIN-dbg.txt
    # ./build/$BIN "$@" # 2> /tmp/$BIN-dbg.txt
    ./build/$BIN "$@" 2> /tmp/$BIN-dbg.txt
fi
