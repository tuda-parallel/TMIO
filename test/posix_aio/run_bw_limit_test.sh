#!/bin/bash
# Verifies TMIO's POSIX-AIO bandwidth pacing (-DBW_LIMIT, custom bw-limit
# MPICH -- see docs/bandwidth_limit.md). Unlike run_test.sh, correctness here
# IS the timing: an unpaced write must be fast, and a write capped at
# 1 MiB/s must take roughly as long as its size implies (asserted inside
# test_bw_limit.cxx).
set -e

DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
ROOT=$(readlink -f "$DIR/../..")
BUILD="$ROOT/build"
MPICXX="$ROOT/dep/bw_limit_mpich/mpich-bin/bin/mpicxx"
MPIRUN="$ROOT/dep/bw_limit_mpich/mpich-bin/bin/mpirun"

if [ ! -x "$MPICXX" ]; then
    echo "FAIL: custom bw-limit MPICH not built at $MPICXX (make bw_limit_mpich_git_build first)."
    exit 1
fi

echo "Building libtmio.so with BW_LIMIT + io_uring-backed POSIX AIO (make bw_limit_iouring_library)..."
(cd "$BUILD" && make bw_limit_iouring_library) >/dev/null

echo "Compiling test_bw_limit and test_mixed_mpi_posix..."
"$MPICXX" -std=c++17 -o "$DIR/test_bw_limit" "$DIR/test_bw_limit.cxx"
"$MPICXX" -std=c++17 -o "$DIR/test_mixed_mpi_posix" "$DIR/test_mixed_mpi_posix.cxx"

# -launcher fork: this MPICH's mpirun defaults to an ssh-based launcher even
# for a local single-rank job, which needs a working local sshd we can't
# assume exists.
# HWLOC_COMPONENTS=-gl: MPI_Init's hwloc-based topology detection loads
# hwloc's GL plugin, which calls XOpenDisplay() to enumerate GPU topology
# over X11. On a machine with a live X/Wayland session that connect() can
# hang indefinitely (confirmed via gdb backtrace: MPI_Init -> ... ->
# MPII_hwtopo_init -> hwloc_topology_load -> hwloc_gl.so -> XOpenDisplay ->
# blocks in connect()). This isn't about bandwidth limiting or TMIO at all --
# it reproduces with a bare MPI_Init()/MPI_Finalize() program -- so just tell
# hwloc to skip that component.
echo "Running test_bw_limit..."
HWLOC_COMPONENTS=-gl LD_PRELOAD="$BUILD/libtmio.so" "$MPIRUN" -launcher fork -n 1 "$DIR/test_bw_limit"

echo "Running test_mixed_mpi_posix (MPI-IO + POSIX AIO in the same process)..."
HWLOC_COMPONENTS=-gl LD_PRELOAD="$BUILD/libtmio.so" "$MPIRUN" -launcher fork -n 1 "$DIR/test_mixed_mpi_posix"
