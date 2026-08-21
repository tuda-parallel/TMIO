#!/bin/bash
# Verifies TMIO's io_uring-backed POSIX AIO emulation (BW_LIMIT_POSIX_AIO=1):
#   1. correctness of aio_write/aio_read/aio_suspend/aio_return/lio_listio
#      (asserted inside test_posix_aio.cxx itself)
#   2. that the calls actually go through io_uring rather than glibc's AIO
#      passthrough -- correctness alone can't tell the two backends apart,
#      since glibc's real AIO is already correct. This is checked here via
#      strace: if no io_uring_setup/io_uring_enter syscalls show up, TMIO
#      silently fell back to passthrough despite the flag.
set -e

DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
ROOT=$(readlink -f "$DIR/../..")
BUILD="$ROOT/build"
STRACE_LOG=$(mktemp)
trap 'rm -f "$STRACE_LOG"' EXIT

echo "Building libtmio.so with io_uring-backed POSIX AIO (make iouring_library)..."
(cd "$BUILD" && make iouring_library) >/dev/null

echo "Compiling test_posix_aio..."
mpicxx -std=c++17 -o "$DIR/test_posix_aio" "$DIR/test_posix_aio.cxx"

echo "Running under strace with TMIO preloaded..."
LD_PRELOAD="$BUILD/libtmio.so" strace -f -e trace=io_uring_setup,io_uring_enter \
    -o "$STRACE_LOG" mpirun -n 1 "$DIR/test_posix_aio"

if ! grep -q "io_uring_setup\|io_uring_enter" "$STRACE_LOG"; then
    echo "FAIL: aio_write/aio_read did not touch io_uring -- still glibc AIO passthrough."
    echo "--- strace log ---"
    cat "$STRACE_LOG"
    exit 1
fi

echo "PASS: POSIX AIO calls are routed through io_uring, and roundtrip correctness holds."
