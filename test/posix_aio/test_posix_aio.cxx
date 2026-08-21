// Correctness tests for TMIO's io_uring-backed POSIX AIO emulation
// (BW_LIMIT_POSIX_AIO=1). See test/posix_aio/run_test.sh for how this is
// built/run and how it is used as the RED/GREEN oracle: it also verifies (via
// strace) that aio_write/aio_read actually go through io_uring, not glibc's
// AIO passthrough, since correctness alone can't tell the two backends apart.
#include <aio.h>
#include <cassert>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <mpi.h>
#include <unistd.h>

static const char *PATH = "tmio_posix_aio_test.dat";

static void test_write_roundtrip()
{
    int fd = open(PATH, O_CREAT | O_WRONLY | O_TRUNC, 0644);
    assert(fd >= 0);

    const char msg[] = "TMIO io_uring backed POSIX AIO write payload";
    struct aiocb cb = {};
    cb.aio_fildes = fd;
    cb.aio_buf = (void *)msg;
    cb.aio_nbytes = sizeof(msg);
    cb.aio_offset = 0;

    assert(aio_write(&cb) == 0);

    const struct aiocb *list[1] = {&cb};
    assert(aio_suspend(list, 1, nullptr) == 0);
    assert(aio_error(&cb) == 0);
    assert(aio_return(&cb) == (ssize_t)sizeof(msg));
    close(fd);

    int rfd = open(PATH, O_RDONLY);
    char buf[sizeof(msg)] = {};
    assert(read(rfd, buf, sizeof(buf)) == (ssize_t)sizeof(msg));
    close(rfd);
    assert(memcmp(buf, msg, sizeof(msg)) == 0);

    printf("test_write_roundtrip: OK\n");
}

static void test_read_roundtrip()
{
    const char msg[] = "TMIO io_uring backed POSIX AIO read payload";
    int wfd = open(PATH, O_CREAT | O_WRONLY | O_TRUNC, 0644);
    assert(wfd >= 0);
    assert(write(wfd, msg, sizeof(msg)) == (ssize_t)sizeof(msg));
    close(wfd);

    int fd = open(PATH, O_RDONLY);
    assert(fd >= 0);

    char buf[sizeof(msg)] = {};
    struct aiocb cb = {};
    cb.aio_fildes = fd;
    cb.aio_buf = buf;
    cb.aio_nbytes = sizeof(buf);
    cb.aio_offset = 0;

    assert(aio_read(&cb) == 0);

    const struct aiocb *list[1] = {&cb};
    assert(aio_suspend(list, 1, nullptr) == 0);
    assert(aio_error(&cb) == 0);
    assert(aio_return(&cb) == (ssize_t)sizeof(msg));
    close(fd);

    assert(memcmp(buf, msg, sizeof(msg)) == 0);

    printf("test_read_roundtrip: OK\n");
}

static void test_lio_listio_wait()
{
    int fd = open(PATH, O_CREAT | O_WRONLY | O_TRUNC, 0644);
    assert(fd >= 0);

    const char part_a[] = "AAAAAAAAAA"; // 10 bytes
    const char part_b[] = "BBBBBBBBBB"; // 10 bytes

    struct aiocb cb_a = {}, cb_b = {};
    cb_a.aio_fildes = fd;
    cb_a.aio_buf = (void *)part_a;
    cb_a.aio_nbytes = 10;
    cb_a.aio_offset = 0;
    cb_a.aio_lio_opcode = LIO_WRITE;

    cb_b.aio_fildes = fd;
    cb_b.aio_buf = (void *)part_b;
    cb_b.aio_nbytes = 10;
    cb_b.aio_offset = 10;
    cb_b.aio_lio_opcode = LIO_WRITE;

    struct aiocb *list[2] = {&cb_a, &cb_b};
    assert(lio_listio(LIO_WAIT, list, 2, nullptr) == 0);
    close(fd);

    char buf[20] = {};
    int rfd = open(PATH, O_RDONLY);
    assert(read(rfd, buf, sizeof(buf)) == 20);
    close(rfd);
    assert(memcmp(buf, "AAAAAAAAAABBBBBBBBBB", 20) == 0);

    printf("test_lio_listio_wait: OK\n");
}

static void test_lio_listio_nowait()
{
    int fd = open(PATH, O_CREAT | O_WRONLY | O_TRUNC, 0644);
    assert(fd >= 0);

    const char part_c[] = "CCCCCCCCCC"; // 10 bytes
    const char part_d[] = "DDDDDDDDDD"; // 10 bytes

    struct aiocb cb_c = {}, cb_d = {};
    cb_c.aio_fildes = fd;
    cb_c.aio_buf = (void *)part_c;
    cb_c.aio_nbytes = 10;
    cb_c.aio_offset = 0;
    cb_c.aio_lio_opcode = LIO_WRITE;

    cb_d.aio_fildes = fd;
    cb_d.aio_buf = (void *)part_d;
    cb_d.aio_nbytes = 10;
    cb_d.aio_offset = 10;
    cb_d.aio_lio_opcode = LIO_WRITE;

    struct aiocb *list[2] = {&cb_c, &cb_d};
    assert(lio_listio(LIO_NOWAIT, list, 2, nullptr) == 0);

    const struct aiocb *wait_list[2] = {&cb_c, &cb_d};
    assert(aio_suspend(wait_list, 2, nullptr) == 0);
    while (aio_error(&cb_c) == EINPROGRESS || aio_error(&cb_d) == EINPROGRESS)
        aio_suspend(wait_list, 2, nullptr);

    assert(aio_return(&cb_c) == 10);
    assert(aio_return(&cb_d) == 10);
    close(fd);

    char buf[20] = {};
    int rfd = open(PATH, O_RDONLY);
    assert(read(rfd, buf, sizeof(buf)) == 20);
    close(rfd);
    assert(memcmp(buf, "CCCCCCCCCCDDDDDDDDDD", 20) == 0);

    printf("test_lio_listio_nowait: OK\n");
}

// Deterministic half of aio_cancel's contract: canceling a request that has
// already completed must report AIO_ALLDONE, not AIO_CANCELED. (The other
// half -- canceling one still in flight -- needs a slow enough transfer to
// reliably race against, which only test_bw_limit.cxx's EMPI_DESIRED_BW_*
// control gives us; see its test_cancel_in_flight.)
static void test_cancel_already_done()
{
    int fd = open(PATH, O_CREAT | O_WRONLY | O_TRUNC, 0644);
    assert(fd >= 0);

    const char msg[] = "cancel-after-completion payload";
    struct aiocb cb = {};
    cb.aio_fildes = fd;
    cb.aio_buf = (void *)msg;
    cb.aio_nbytes = sizeof(msg);
    cb.aio_offset = 0;

    assert(aio_write(&cb) == 0);
    const struct aiocb *list[1] = {&cb};
    assert(aio_suspend(list, 1, nullptr) == 0);
    assert(aio_error(&cb) == 0); // genuinely completed before we try to cancel it

    assert(aio_cancel(fd, &cb) == AIO_ALLDONE);
    assert(aio_return(&cb) == (ssize_t)sizeof(msg));
    close(fd);

    printf("test_cancel_already_done: OK\n");
}

// Other half of aio_cancel's contract: canceling a request that's genuinely
// still in flight. A regular file completes too fast locally to reliably
// race against, so this writes to a pipe filled to capacity with no reader --
// the write physically cannot complete (0 bytes of buffer space exist) until
// something reads or the request is canceled, giving a deterministic window.
// The actual outcome (AIO_CANCELED vs AIO_NOTCANCELED) is kernel/timing
// dependent -- POSIX allows either for a best-effort cancel -- so this only
// asserts it's not wrongly reported AIO_ALLDONE and that the call itself
// doesn't hang.
static void test_cancel_in_flight()
{
    int fds[2];
    assert(pipe(fds) == 0);
    int read_fd = fds[0], write_fd = fds[1];

    // Fill the pipe completely so the aio_write below has zero space to
    // partially succeed into -- it must genuinely stay pending.
    int flags = fcntl(write_fd, F_GETFL);
    fcntl(write_fd, F_SETFL, flags | O_NONBLOCK);
    char filler[4096];
    memset(filler, 'f', sizeof(filler));
    while (write(write_fd, filler, sizeof(filler)) > 0) {}
    assert(errno == EAGAIN);
    fcntl(write_fd, F_SETFL, flags); // restore blocking mode for aio_write

    char payload[4096];
    memset(payload, 'p', sizeof(payload));
    struct aiocb cb = {};
    cb.aio_fildes = write_fd;
    cb.aio_buf = payload;
    cb.aio_nbytes = sizeof(payload);

    assert(aio_write(&cb) == 0);
    usleep(50000); // let the kernel actually start (and block on) the write

    int cancel_result = aio_cancel(write_fd, &cb);
    assert(cancel_result == AIO_CANCELED || cancel_result == AIO_NOTCANCELED);

    if (cancel_result == AIO_CANCELED)
    {
        assert(aio_error(&cb) == ECANCELED);
    }
    else
    {
        char drain[8192];
        while (read(read_fd, drain, sizeof(drain)) > 0) {}
        const struct aiocb *list[1] = {&cb};
        aio_suspend(list, 1, nullptr);
    }

    close(read_fd);
    close(write_fd);
    printf("test_cancel_in_flight: OK\n");
}

int main(int argc, char **argv)
{
    MPI_Init(&argc, &argv);

    test_write_roundtrip();
    test_read_roundtrip();
    test_lio_listio_wait();
    test_lio_listio_nowait();
    test_cancel_already_done();
    test_cancel_in_flight();

    unlink(PATH);
    printf("All POSIX AIO tests passed.\n");

    MPI_Finalize();
    return 0;
}
