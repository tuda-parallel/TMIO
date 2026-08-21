// Verifies TMIO stays sane (no crash, no hang, no corruption) when a single
// process uses BOTH MPI-IO (MPI_File_iwrite) and direct POSIX aio_write in
// the same run. EMPI_DESIRED_BW_IWRITE is a single process-wide global that
// both the MPI-IO layer (via mpi_iotrace's Bw_limit) and this POSIX layer
// (via get_libc_iotrace()'s Bw_limit, see pace_on_completion in
// libc_interface.cxx) read/write -- this is the one thing neither
// test_posix_aio.cxx nor test_bw_limit.cxx (POSIX-only) nor the HACC-IO
// example (MPI-IO-only) actually exercises together.
#include <aio.h>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <mpi.h>
#include <unistd.h>
#include <vector>

extern "C" long double EMPI_DESIRED_BW_IWRITE;

int main(int argc, char **argv)
{
    MPI_Init(&argc, &argv);

    // 1. MPI-IO write: drives mpi_iotrace's Bw_limit phase tracking, which
    // calls apply_bw_limit() on MPI_Wait and updates the shared
    // EMPI_DESIRED_BW_IWRITE global from MPI-IO's own measured throughput.
    const char *mpi_path = "tmio_mixed_mpi.dat";
    const char mpi_msg[] = "MPI-IO half of the mixed workload";
    MPI_File fh;
    assert(MPI_File_open(MPI_COMM_SELF, mpi_path, MPI_MODE_CREATE | MPI_MODE_WRONLY, MPI_INFO_NULL, &fh) == MPI_SUCCESS);
    MPI_Request req;
    assert(MPI_File_iwrite(fh, mpi_msg, sizeof(mpi_msg), MPI_BYTE, &req) == MPI_SUCCESS);
    MPI_Status status;
    assert(MPI_Wait(&req, &status) == MPI_SUCCESS);
    assert(MPI_File_close(&fh) == MPI_SUCCESS);
    printf("MPI-IO write: OK\n");

    // The MPI-IO write above is too small/fast to reliably make mpi_iotrace's
    // own Bw_limit phase math land on a specific nonzero EMPI_DESIRED_BW_IWRITE
    // (timing-dependent). Force one explicitly here to deterministically
    // simulate "the MPI-IO layer just set a real target", and confirm the
    // POSIX layer actually inherits and paces off it, per ioflags.h's
    // BW_LIMIT_LAYER doc: standalone POSIX AIO with no MPI-IO in its own call
    // chain always paces here, regardless of which layer last wrote the
    // shared global.
    EMPI_DESIRED_BW_IWRITE = 128.0L * 1024; // 128 KiB/s, inherited from "MPI-IO"

    // 2. Direct POSIX aio_write, in the same process right after.
    const char *posix_path = "tmio_mixed_posix.dat";
    std::vector<char> posix_buf(256 * 1024, 'p'); // -> ~2s at the 128 KiB/s cap above
    int fd = open(posix_path, O_CREAT | O_WRONLY | O_TRUNC, 0644);
    assert(fd >= 0);

    struct aiocb cb = {};
    cb.aio_fildes = fd;
    cb.aio_buf = posix_buf.data();
    cb.aio_nbytes = posix_buf.size();
    cb.aio_offset = 0;

    double t0 = MPI_Wtime();
    assert(aio_write(&cb) == 0);
    const struct aiocb *list[1] = {&cb};
    assert(aio_suspend(list, 1, nullptr) == 0);
    assert(aio_error(&cb) == 0);
    assert(aio_return(&cb) == (ssize_t)posix_buf.size());
    double elapsed = MPI_Wtime() - t0;
    close(fd);
    printf("POSIX AIO write: OK (took %.3f s, expected ~2s inherited from the MPI-IO-set target)\n", elapsed);
    assert(elapsed >= 1.5 && elapsed < 30.0); // genuinely paced by the inherited value, and finite

    // Correctness: both files must actually contain what was written,
    // regardless of any cross-layer pacing interaction above.
    char buf[128] = {};
    int rfd = open(mpi_path, O_RDONLY);
    assert(read(rfd, buf, sizeof(buf)) == (ssize_t)sizeof(mpi_msg));
    assert(memcmp(buf, mpi_msg, sizeof(mpi_msg)) == 0);
    close(rfd);

    memset(buf, 'p', sizeof(buf));
    char check[sizeof(buf)];
    rfd = open(posix_path, O_RDONLY);
    assert(read(rfd, check, sizeof(check)) == (ssize_t)sizeof(check));
    assert(memcmp(check, buf, sizeof(check)) == 0); // spot-check: first bytes are all 'p'
    close(rfd);

    unlink(mpi_path);
    unlink(posix_path);
    printf("test_mixed_mpi_posix: OK\n");

    MPI_Finalize();
    return 0;
}
