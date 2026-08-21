// Verifies that TMIO's io_uring-backed POSIX AIO actually paces to
// EMPI_DESIRED_BW_IWRITE (the same global Bw_limit writes to and the bw-limit
// MPICH's ROMIO patch reads -- see include/bw_limit.h and
// dep/bw_limit_mpich/.../async_io_funcs.c). Requires building with -DBW_LIMIT
// against that custom MPICH (see test/posix_aio/run_bw_limit_test.sh).
#include <aio.h>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <mpi.h>
#include <unistd.h>
#include <vector>

extern "C" long double EMPI_DESIRED_BW_IWRITE;

static const char *PATH = "tmio_bw_limit_test.dat";
static const size_t SIZE = 4 * 1024 * 1024; // 4 MiB

static double write_once(std::vector<char> &buf)
{
    int fd = open(PATH, O_CREAT | O_WRONLY | O_TRUNC, 0644);
    assert(fd >= 0);

    struct aiocb cb = {};
    cb.aio_fildes = fd;
    cb.aio_buf = buf.data();
    cb.aio_nbytes = buf.size();
    cb.aio_offset = 0;

    double t0 = MPI_Wtime();
    assert(aio_write(&cb) == 0);
    const struct aiocb *list[1] = {&cb};
    assert(aio_suspend(list, 1, nullptr) == 0);
    assert(aio_error(&cb) == 0);
    assert(aio_return(&cb) == (ssize_t)buf.size());
    double elapsed = MPI_Wtime() - t0;

    close(fd);
    return elapsed;
}

// Accuracy sweep: for each target bandwidth, pace a write sized so the
// target duration is ~2s, then check the *achieved* bandwidth (bytes /
// elapsed) is within TOLERANCE of the target -- not just "took at least N
// seconds". Mirrors what docs/bandwidth_limit.md's T/B_L/B plotting checks
// visually via an external example app, as a self-contained numeric check.
static const double TOLERANCE = 0.15; // +/-15%

static void test_accuracy_at(long double target_bw_bytes_per_sec)
{
    size_t size = static_cast<size_t>(target_bw_bytes_per_sec * 2.0L); // ~2s target
    std::vector<char> buf(size, 'x');

    EMPI_DESIRED_BW_IWRITE = target_bw_bytes_per_sec;
    double elapsed = write_once(buf);
    double achieved_bw = static_cast<double>(size) / elapsed;
    double target_bw = static_cast<double>(target_bw_bytes_per_sec);
    double rel_error = (achieved_bw - target_bw) / target_bw;

    printf("target %.0f B/s: wrote %zu bytes in %.3f s -> achieved %.0f B/s (%.1f%% error)\n",
           target_bw, size, elapsed, achieved_bw, rel_error * 100.0);
    assert(rel_error >= -TOLERANCE && rel_error <= TOLERANCE);
}

int main(int argc, char **argv)
{
    MPI_Init(&argc, &argv);
    std::vector<char> buf(SIZE, 'x');

    EMPI_DESIRED_BW_IWRITE = 0.0L;
    double unpaced = write_once(buf);
    printf("unpaced write of %zu bytes took %.3f s\n", SIZE, unpaced);
    assert(unpaced < 2.0); // sanity: without a cap this is fast (local disk/tmpfs)

    EMPI_DESIRED_BW_IWRITE = 1.0L * 1024 * 1024; // cap at 1 MiB/s -> expect ~4s
    double paced = write_once(buf);
    printf("paced write of %zu bytes at 1 MiB/s took %.3f s\n", SIZE, paced);
    assert(paced >= 3.0); // generous floor around the ~4s target to avoid flakiness
    printf("test_bw_limit_paces_posix_aio: OK\n");

    test_accuracy_at(256.0L * 1024);       // 256 KiB/s
    test_accuracy_at(1.0L * 1024 * 1024);  // 1 MiB/s
    test_accuracy_at(4.0L * 1024 * 1024);  // 4 MiB/s
    printf("test_bw_limit_accuracy: OK\n");

    unlink(PATH);
    MPI_Finalize();
    return 0;
}
