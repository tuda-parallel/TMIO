#include "tmio.h"

#if ENABLE_LIBC_TRACE == 1
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <stdio.h>
#include <time.h>
#include <aio.h>

#ifdef __linux__
#ifndef _GNU_SOURCE
#define _GNU_SOURCE // For preadv2, must be defined before including any headers
#endif
#include <sys/uio.h>
#endif

// Check for the prototype
#if defined(__GLIBC__) && (__GLIBC__ > 2 || (__GLIBC__ == 2 && __GLIBC_MINOR__ >= 10))
// preadv is available in glibc since 2.10
#define HAVE_PREADV
#endif
#if defined(__GLIBC__) && (__GLIBC__ > 2 || (__GLIBC__ == 2 && __GLIBC_MINOR__ >= 26))
// preadv2 is available in glibc since 2.26
#define HAVE_PREADV2
#endif


IOtraceLibc& get_libc_iotrace() {
	// Must use static IOtraceLibc instance to ensure a single instance is initialized when first used.
	// TODO: Might need to use a mutex to protect the instance creation if multiple threads might access it simultaneously.
	// `thread_local` is for some MPI implementations that has a separate MPI progress thread, 
	// which differs from the main thread, and both the main thread and the MPI progress thread
	// might access the IOtraceLibc instance simultaneously.  So we use thread_local to ensure
	// that each thread has its own instance of IOtraceLibc.
    static thread_local IOtraceLibc instance;
    return instance;
}

TMIO_FORWARD_DECL(open, int, (const char *path, int flags, ...));
// See：https://github.com/darshan-hpc/darshan/issues/253 for `__open_2`
TMIO_FORWARD_DECL(__open_2, int, (const char *path, int oflag));
TMIO_FORWARD_DECL(open64, int, (const char *path, int flags, ...));
TMIO_FORWARD_DECL(openat, int, (int dirfd, const char *pathname, int flags, ...));
TMIO_FORWARD_DECL(openat64, int, (int dirfd, const char *pathname, int flags, ...));
// TMIO_FORWARD_DECL(creat, int, (const char* path, mode_t mode));
// TMIO_FORWARD_DECL(creat64, int, (const char* path, mode_t mode));
TMIO_FORWARD_DECL(close, int, (int fd));

TMIO_FORWARD_DECL(aio_read, int, (struct aiocb * aiocbp));
TMIO_FORWARD_DECL(aio_read64, int, (struct aiocb64 * aiocbp));
TMIO_FORWARD_DECL(aio_write, int, (struct aiocb * aiocbp));
TMIO_FORWARD_DECL(aio_write64, int, (struct aiocb64 * aiocbp));
TMIO_FORWARD_DECL(aio_error, int, (const struct aiocb *aiocbp));
TMIO_FORWARD_DECL(aio_cancel, int, (int fd, struct aiocb *aiocbp));
TMIO_FORWARD_DECL(aio_cancel64, int, (int fd, struct aiocb64 *aiocbp));
// `restrict` said cannot be multiple pointer to the same object, but C++ does not support `restrict` keyword, so we use `__restrict__`
TMIO_FORWARD_DECL(aio_suspend, int, (const struct aiocb *const aiocb_list[], int n, const struct timespec *__restrict__ timeout));
TMIO_FORWARD_DECL(aio_return, ssize_t, (struct aiocb * aiocbp));
TMIO_FORWARD_DECL(aio_return64, ssize_t, (struct aiocb64 * aiocbp));
TMIO_FORWARD_DECL(lio_listio, int, (int mode, struct aiocb *const aiocb_list[], int nitems, struct sigevent *sevp));
TMIO_FORWARD_DECL(lio_listio64, int, (int mode, struct aiocb64 *const aiocb_list[], int nitems, struct sigevent *sevp));

#if BW_LIMIT_POSIX_AIO == 1
#include "interfaces/iouring_interface.h"
#include <liburing.h>
#include <unordered_map>
#include <unordered_set>
#include <vector>

// Reimplements POSIX AIO (aio_write/aio_read/aio_error/aio_return/aio_suspend/
// lio_listio) on top of TMIO's own io_uring instance instead of glibc's AIO,
// so submissions/completions become observable points to bandwidth-limit
// against (see ioflags.h's BW_LIMIT_POSIX_AIO comment). Tracing keeps using
// get_libc_iotrace() exactly as before (unchanged call sites below); this
// namespace only replaces *how* the bytes actually move.
//
// The ring here is deliberately never registered with the io_uring tracer
// (get_iouring_iotrace()/Register_Ring, see tmio_real_io_uring_queue_init):
// it's an implementation detail of POSIX AIO emulation, not an
// application-level io_uring the app is directly responsible for. Registering
// it would make get_iouring_iotrace() double-count transactions already
// reported here via get_libc_iotrace().
#ifdef BW_LIMIT
// EMPI_DESIRED_BW_I{WRITE,READ}: the same globals Bw_limit writes to and the
// bw-limit MPICH's ROMIO patch reads (see include/bw_limit.h and
// dep/bw_limit_mpich/.../async_io_funcs.c). Only declared/used when linked
// against that MPICH build (docs/bandwidth_limit.md). Declared at file scope,
// matching bw_limit.cxx's own convention for these externs -- inside a
// namespace, C++ name-mangles the declaration and it no longer matches the
// plain C symbol these globals actually have.
extern long double EMPI_DESIRED_BW_IWRITE;
extern long double EMPI_DESIRED_BW_IREAD;
#endif

namespace tmio_posix_aio
{
    struct CompletionState
    {
        bool done = false;
        int res = 0; // >=0: bytes transferred; <0: -errno
    };

#ifdef BW_LIMIT
    struct SubmitMeta
    {
        double submit_time;
        bool is_write;
    };
#endif

    // NOTE: thread_local, same limitation as tmio::in_mpi_io_call (see
    // tmio_helper_functions.h): an aiocb submitted on one thread cannot be
    // waited on / queried for completion from another thread.
    struct RingContext
    {
        struct io_uring ring{};
        bool initialized = false;
        std::unordered_map<const struct aiocb *, CompletionState> completions;
        std::unordered_set<struct aiocb *> pending; // submitted, not yet completed -- for aio_cancel
#ifdef BW_LIMIT
        std::unordered_map<const struct aiocb *, SubmitMeta> submit_meta;
#endif

        ~RingContext()
        {
            if (initialized)
                io_uring_queue_exit(&ring);
        }
    };

    inline thread_local RingContext ring_ctx;

    inline void ensure_ring_initialized()
    {
        if (ring_ctx.initialized)
            return;
        int rc = tmio_real_io_uring_queue_init(64, &ring_ctx.ring, 0);
        if (rc < 0)
        {
            fprintf(stderr, "TMIO > failed to initialize io_uring for POSIX AIO emulation: %s\n", strerror(-rc));
            exit(1);
        }
        ring_ctx.initialized = true;
    }

#ifdef BW_LIMIT
    // Mirrors the reference bw-limit MPICH's ROMIO patch (async_io_funcs.c's
    // WriteContig/ReadContig): let the transfer run at full speed, then sleep
    // out the remainder of the target duration before the completion becomes
    // observable to aio_error/aio_return/aio_suspend.
    // ponytail: unlike the reference, there's no time_excess carry-over
    // between requests to compensate a slow request with a shorter sleep on
    // the next one -- each request is paced independently. Upgrade if bursty
    // completions show a systematic overshoot above the target bandwidth.
    inline void pace_on_completion(const struct aiocb *aiocbp, int res)
    {
        if (res < 0)
            return; // failed transfer; nothing meaningful to pace

        auto it = ring_ctx.submit_meta.find(aiocbp);
        if (it == ring_ctx.submit_meta.end())
            return;
        double submit_time = it->second.submit_time;
        bool is_write = it->second.is_write;
        ring_ctx.submit_meta.erase(it);

        // Transactions with no MPI-IO in their call chain always pace here
        // regardless of BW_LIMIT_LAYER; only skip when MPI-IO's own ROMIO
        // patch already paced this one (see ioflags.h's BW_LIMIT_LAYER doc).
        if (tmio::in_mpi_io_call && BW_LIMIT_LAYER == 0)
            return;

        long double desired_bw = is_write ? EMPI_DESIRED_BW_IWRITE : EMPI_DESIRED_BW_IREAD;
        if (desired_bw <= 0.0L)
            return;

        double run_time = MPI_Wtime() - submit_time;
        double target_time = static_cast<double>(res) / static_cast<double>(desired_bw);
        double remaining = target_time - run_time;
        if (remaining > 0.0)
            usleep(static_cast<useconds_t>(remaining * 1'000'000.0));
    }
#endif

    // Records one io_uring completion: pace it (BW_LIMIT builds only), refresh
    // Bw_limit's next-round bandwidth target, then make it observable via
    // ring_ctx.completions.
    inline void record_completion(struct io_uring_cqe *cqe)
    {
        auto *aiocbp = reinterpret_cast<struct aiocb *>(io_uring_cqe_get_data(cqe));
        int res = cqe->res;
        io_uring_cqe_seen(&ring_ctx.ring, cqe);

        ring_ctx.pending.erase(aiocbp);

#ifdef BW_LIMIT
        pace_on_completion(aiocbp, res);
#endif

        ring_ctx.completions[aiocbp] = {true, res};

#if BW_LIMIT_GRANULARITY == 1
        get_libc_iotrace().apply_bw_limit();
#elif defined CUSTOM_MPI
        get_libc_iotrace().set_custom_throughput();
#endif
    }

    // Records every ready completion (if any) without blocking.
    inline void drain_ready_completions()
    {
        struct io_uring_cqe *cqe;
        while (io_uring_peek_cqe(&ring_ctx.ring, &cqe) == 0)
            record_completion(cqe);
    }

    // Submits one aiocb as a single io_uring read or write SQE. Mirrors
    // aio_write/aio_read's return contract: 0 on successful enqueue, -1/errno
    // on failure.
    inline int submit_one(struct aiocb *aiocbp, bool is_write)
    {
        ensure_ring_initialized();

        struct io_uring_sqe *sqe = io_uring_get_sqe(&ring_ctx.ring);
        if (!sqe)
        {
            drain_ready_completions();
            sqe = io_uring_get_sqe(&ring_ctx.ring);
            if (!sqe)
            {
                errno = EAGAIN;
                return -1;
            }
        }

        void *buf = const_cast<void *>(aiocbp->aio_buf);
        if (is_write)
            io_uring_prep_write(sqe, aiocbp->aio_fildes, buf, static_cast<unsigned>(aiocbp->aio_nbytes), static_cast<__u64>(aiocbp->aio_offset));
        else
            io_uring_prep_read(sqe, aiocbp->aio_fildes, buf, static_cast<unsigned>(aiocbp->aio_nbytes), static_cast<__u64>(aiocbp->aio_offset));

        io_uring_sqe_set_data(sqe, aiocbp);
        ring_ctx.completions.erase(aiocbp); // Fresh request; drop any stale prior completion.

        int rc = io_uring_submit(&ring_ctx.ring);
        if (rc < 0)
        {
            errno = -rc;
            return -1;
        }
        ring_ctx.pending.insert(aiocbp);
#ifdef BW_LIMIT
        ring_ctx.submit_meta[aiocbp] = {MPI_Wtime(), is_write};
#endif
        return 0;
    }

    // Returns EINPROGRESS, 0 (success), or a positive errno, matching aio_error's contract.
    inline int error_of(const struct aiocb *aiocbp)
    {
        drain_ready_completions();
        auto it = ring_ctx.completions.find(aiocbp);
        if (it == ring_ctx.completions.end() || !it->second.done)
            return EINPROGRESS;
        return it->second.res >= 0 ? 0 : -it->second.res;
    }

    // Consumes the recorded completion for aiocbp, matching aio_return's
    // contract (bytes transferred, or -1/errno). Per POSIX, calling this
    // again for the same aiocb before a new submission is undefined; we free
    // the entry to bound memory instead of tracking that explicitly.
    inline ssize_t return_of(struct aiocb *aiocbp)
    {
        auto it = ring_ctx.completions.find(aiocbp);
        if (it == ring_ctx.completions.end() || !it->second.done)
        {
            errno = EINVAL;
            return -1;
        }
        ssize_t result;
        if (it->second.res >= 0)
            result = it->second.res;
        else
        {
            errno = -it->second.res;
            result = -1;
        }
        ring_ctx.completions.erase(it);
        return result;
    }

    // Blocks until aiocbp's completion is recorded (or a wait error occurs).
    inline void wait_for(const struct aiocb *aiocbp)
    {
        ensure_ring_initialized();
        while (!(ring_ctx.completions.count(aiocbp) && ring_ctx.completions[aiocbp].done))
        {
            struct io_uring_cqe *cqe = nullptr;
            int rc = io_uring_wait_cqe(&ring_ctx.ring, &cqe);
            if (rc < 0)
            {
                errno = -rc;
                return;
            }
            record_completion(cqe);
        }
    }

    // aio_suspend: blocks until at least one of `list`'s entries has completed.
    // ponytail: a fresh io_uring_wait_cqe_timeout is re-armed on each spurious
    // (irrelevant-completion) wakeup, so the *total* wait can overrun the
    // caller's timeout when other unrelated requests complete first on this
    // thread's ring. Upgrade if callers start passing multi-item lists with a
    // tight timeout on a busy ring; today's callers wait on 1-2 items.
    inline int suspend(const struct aiocb *const list[], int nitems, const struct timespec *timeout)
    {
        ensure_ring_initialized();

        auto is_target = [&](const struct aiocb *p)
        {
            for (int i = 0; i < nitems; ++i)
                if (list[i] == p)
                    return true;
            return false;
        };

        drain_ready_completions();
        for (int i = 0; i < nitems; ++i)
        {
            if (list[i] && ring_ctx.completions.count(list[i]) && ring_ctx.completions[list[i]].done)
                return 0;
        }

        struct __kernel_timespec kts;
        if (timeout)
        {
            kts.tv_sec = timeout->tv_sec;
            kts.tv_nsec = timeout->tv_nsec;
        }

        for (;;)
        {
            struct io_uring_cqe *cqe = nullptr;
            int rc = timeout ? io_uring_wait_cqe_timeout(&ring_ctx.ring, &cqe, &kts)
                              : io_uring_wait_cqe(&ring_ctx.ring, &cqe);

            if (rc == -ETIME)
            {
                errno = EAGAIN;
                return -1;
            }
            if (rc < 0)
            {
                errno = -rc;
                return -1;
            }

            auto *cbp = reinterpret_cast<const struct aiocb *>(io_uring_cqe_get_data(cqe));
            record_completion(cqe);

            if (is_target(cbp))
                return 0;
        }
    }

    // Sentinel user_data for a cancel operation's own SQE/CQE, so
    // record_completion() (which assumes any other user_data is an aiocb*)
    // never sees it: cancel_one() intercepts and consumes it directly.
    inline char cancel_op_marker;

    // aio_cancel(fd, aiocbp): best-effort cancel of one specific request.
    // Returns AIO_CANCELED, AIO_NOTCANCELED, or AIO_ALLDONE (see <aio.h>).
    inline int cancel_one(struct aiocb *aiocbp)
    {
        ensure_ring_initialized();
        drain_ready_completions();

        if (ring_ctx.completions.count(aiocbp) && ring_ctx.completions[aiocbp].done)
            return AIO_ALLDONE;
        if (!ring_ctx.pending.count(aiocbp))
            return AIO_ALLDONE; // never submitted on this thread's ring, or already reaped

        struct io_uring_sqe *sqe = io_uring_get_sqe(&ring_ctx.ring);
        if (!sqe)
        {
            errno = EAGAIN;
            return -1;
        }
        io_uring_prep_cancel(sqe, aiocbp, 0);
        io_uring_sqe_set_data(sqe, &cancel_op_marker);

        int rc = io_uring_submit(&ring_ctx.ring);
        if (rc < 0)
        {
            errno = -rc;
            return -1;
        }

        // Block for the cancel op's own completion, recording any other
        // (unrelated) completions normally along the way.
        int cancel_res = 0;
        for (;;)
        {
            struct io_uring_cqe *cqe = nullptr;
            int wrc = io_uring_wait_cqe(&ring_ctx.ring, &cqe);
            if (wrc < 0)
            {
                errno = -wrc;
                return -1;
            }
            if (io_uring_cqe_get_data(cqe) == &cancel_op_marker)
            {
                cancel_res = cqe->res;
                io_uring_cqe_seen(&ring_ctx.ring, cqe);
                break;
            }
            record_completion(cqe);
        }

        // On success the target's own completion (res == -ECANCELED) should
        // now be ready too; drain it so aio_error()/aio_return() see it.
        drain_ready_completions();

        if (cancel_res == 0)
            return AIO_CANCELED;
        if (cancel_res == -EALREADY)
            return AIO_NOTCANCELED;
        // -ENOENT or anything else: target wasn't found on this ring (raced
        // with its own completion, or already reaped).
        return ring_ctx.pending.count(aiocbp) ? AIO_NOTCANCELED : AIO_ALLDONE;
    }

    // aio_cancel(fd, NULL): best-effort cancel of every outstanding request
    // for fd that was submitted on this thread's ring.
    inline int cancel_all(int fd)
    {
        ensure_ring_initialized();
        drain_ready_completions();

        std::vector<struct aiocb *> targets;
        for (auto *p : ring_ctx.pending)
            if (p->aio_fildes == fd)
                targets.push_back(p);

        if (targets.empty())
            return AIO_ALLDONE;

        bool all_canceled = true;
        for (auto *p : targets)
            if (cancel_one(p) != AIO_CANCELED)
                all_canceled = false;
        return all_canceled ? AIO_CANCELED : AIO_NOTCANCELED;
    }

    // lio_listio helpers: submit every non-NOP item, and/or block until every
    // submitted item has completed. Returns 0 if all items succeeded, else -1
    // with errno set to the first failure's error.
    inline int submit_batch(struct aiocb *const list[], int nitems)
    {
        int first_errno = 0;
        for (int i = 0; i < nitems; ++i)
        {
            if (!list[i] || list[i]->aio_lio_opcode == LIO_NOP)
                continue;
            bool is_write = (list[i]->aio_lio_opcode == LIO_WRITE);
            if (submit_one(list[i], is_write) != 0 && !first_errno)
                first_errno = errno;
        }
        if (first_errno)
        {
            errno = first_errno;
            return -1;
        }
        return 0;
    }

    inline int wait_batch(struct aiocb *const list[], int nitems)
    {
        int first_errno = 0;
        for (int i = 0; i < nitems; ++i)
        {
            if (!list[i] || list[i]->aio_lio_opcode == LIO_NOP)
                continue;
            wait_for(list[i]);
            auto it = ring_ctx.completions.find(list[i]);
            if (it != ring_ctx.completions.end() && it->second.res < 0 && !first_errno)
                first_errno = -it->second.res;
        }
        if (first_errno)
        {
            errno = first_errno;
            return -1;
        }
        return 0;
    }
} // namespace tmio_posix_aio
#endif // BW_LIMIT_POSIX_AIO

TMIO_FORWARD_DECL(read, ssize_t, (int fd, void *buf, size_t count));
TMIO_FORWARD_DECL(write, ssize_t, (int fd, const void *buf, size_t count));
TMIO_FORWARD_DECL(pread, ssize_t, (int fd, void *buf, size_t count, off_t offset));
TMIO_FORWARD_DECL(pread64, ssize_t, (int fd, void *buf, size_t count, off64_t offset));
TMIO_FORWARD_DECL(pwrite, ssize_t, (int fd, const void *buf, size_t count, off_t offset));
TMIO_FORWARD_DECL(pwrite64, ssize_t, (int fd, const void *buf, size_t count, off64_t offset));
TMIO_FORWARD_DECL(readv, ssize_t, (int fd, const struct iovec *iov, int iovcnt, off_t offset));
#ifdef HAVE_PREADV
TMIO_FORWARD_DECL(preadv, ssize_t, (int fd, const struct iovec *iov, int iovcnt, off_t offset));
TMIO_FORWARD_DECL(preadv64, ssize_t, (int fd, const struct iovec *iov, int iovcnt, off64_t offset));
#endif // HAVE_PREADV
#ifdef HAVE_PREADV2
TMIO_FORWARD_DECL(preadv2, ssize_t, (int fd, const struct iovec *iov, int iovcnt, off_t offset, int flags));
TMIO_FORWARD_DECL(preadv64v2, ssize_t, (int fd, const struct iovec *iov, int iovcnt, off64_t offset, int flags));
#endif // HAVE_PREADV2
TMIO_FORWARD_DECL(writev, ssize_t, (int fd, const struct iovec *iov, int iovcnt));
#ifdef HAVE_PREADV
TMIO_FORWARD_DECL(pwritev, ssize_t, (int fd, const struct iovec *iov, int iovcnt, off_t offset));
TMIO_FORWARD_DECL(pwritev64, ssize_t, (int fd, const struct iovec *iov, int iovcnt, off64_t offset));
#endif // HAVE_PREADV
#ifdef HAVE_PREADV2
TMIO_FORWARD_DECL(pwritev2, ssize_t, (int fd, const struct iovec *iov, int iovcnt, off_t offset, int flags));
TMIO_FORWARD_DECL(pwritev64v2, ssize_t, (int fd, const struct iovec *iov, int iovcnt, off64_t offset, int flags));
#endif // HAVE_PREADV2

// //! ----------------------- Open, Close, and Create ------------------------------

int TMIO_DECL(open)(const char *path, int flags, ...)
{
	Function_Debug(__PRETTY_FUNCTION__);
	int mode = 0;
	int ret;

	MAP_OR_FAIL(open);

	if (flags & O_CREAT)
	{
		va_list arg;
		va_start(arg, flags);
		mode = va_arg(arg, int);
		va_end(arg);

		ret = __real_open(path, flags, mode);
	}
	else
	{
		ret = __real_open(path, flags);
	}

	get_libc_iotrace().Open(path, ret);
	return (ret);
}

int TMIO_DECL(__open_2)(const char *path, int oflag)
{
	Function_Debug(__PRETTY_FUNCTION__);
	int ret;

	MAP_OR_FAIL(__open_2);

	ret = __real_open(path, oflag);

	get_libc_iotrace().Open(path, ret);

	return (ret);
}

int TMIO_DECL(open64)(const char *path, int flags, ...)
{
	Function_Debug(__PRETTY_FUNCTION__);
	int mode = 0;
	int ret;

	MAP_OR_FAIL(open64);

	if (flags & O_CREAT)
	{
		va_list arg;
		va_start(arg, flags);
		mode = va_arg(arg, int);
		va_end(arg);

		ret = __real_open64(path, flags, mode);
	}
	else
	{
		ret = __real_open64(path, flags);
	}

	get_libc_iotrace().Open(path, ret);
	return (ret);
}

int TMIO_DECL(openat)(int dirfd, const char *pathname, int flags, ...)
{
	Function_Debug(__PRETTY_FUNCTION__);
	int mode = 0;
	int ret;

	MAP_OR_FAIL(openat);

	if (flags & O_CREAT)
	{
		va_list arg;
		va_start(arg, flags);
		mode = va_arg(arg, int);
		va_end(arg);

		ret = __real_openat(dirfd, pathname, flags, mode);
	}
	else
	{
		ret = __real_openat(dirfd, pathname, flags);
	}

	get_libc_iotrace().Open(pathname, ret);
	return (ret);
}

int TMIO_DECL(openat64)(int dirfd, const char *pathname, int flags, ...)
{
	Function_Debug(__PRETTY_FUNCTION__);
	int mode = 0;
	int ret;

	MAP_OR_FAIL(openat64);

	if (flags & O_CREAT)
	{
		va_list arg;
		va_start(arg, flags);
		mode = va_arg(arg, int);
		va_end(arg);

		ret = __real_openat64(dirfd, pathname, flags, mode);
	}
	else
	{
		ret = __real_openat64(dirfd, pathname, flags);
	}

	get_libc_iotrace().Open(pathname, ret);
	return (ret);
}

int TMIO_DECL(close)(int fd)
{
	Function_Debug(__PRETTY_FUNCTION__);
	int ret;

	MAP_OR_FAIL(close);

	ret = __real_close(fd);

	get_libc_iotrace().Close(fd);

	// std::cout << "TMIO > close(" << fd << ")" << std::endl;

	return (ret);
}

//! ----------------------- Async Write ------------------------------
int TMIO_DECL(aio_write)(struct aiocb *aiocbp)
{
	std::string function_name = __PRETTY_FUNCTION__;
	// Also append the fd to the function name for better debugging
	function_name += " (fd: " + std::to_string(aiocbp->aio_fildes) + ")";
	Function_Debug(function_name);
	int ret;

	get_libc_iotrace().Write_Async_Start(aiocbp);
#if BW_LIMIT_POSIX_AIO == 1
	ret = tmio_posix_aio::submit_one(aiocbp, true);
#else
	MAP_OR_FAIL(aio_write);
	ret = __real_aio_write(aiocbp);
#endif
	return (ret);
}

int TMIO_DECL(aio_write64)(struct aiocb64 *aiocbp)
{
	Function_Debug(__PRETTY_FUNCTION__);
	int ret;

	get_libc_iotrace().Write_Async_Start(aiocbp);
#if BW_LIMIT_POSIX_AIO == 1
	ret = tmio_posix_aio::submit_one(reinterpret_cast<struct aiocb *>(aiocbp), true);
#else
	MAP_OR_FAIL(aio_write64);
	ret = __real_aio_write64(aiocbp);
#endif
	return (ret);
}

//! ----------------------- Async Read ------------------------------
int TMIO_DECL(aio_read)(struct aiocb *aiocbp)
{
	Function_Debug(__PRETTY_FUNCTION__);
	int ret;

	get_libc_iotrace().Read_Async_Start(aiocbp);
#if BW_LIMIT_POSIX_AIO == 1
	ret = tmio_posix_aio::submit_one(aiocbp, false);
#else
	MAP_OR_FAIL(aio_read);
	ret = __real_aio_read(aiocbp);
#endif
	return (ret);
}

int TMIO_DECL(aio_read64)(struct aiocb64 *aiocbp)
{
	Function_Debug(__PRETTY_FUNCTION__);
	int ret;

	get_libc_iotrace().Read_Async_Start(aiocbp);
#if BW_LIMIT_POSIX_AIO == 1
	ret = tmio_posix_aio::submit_one(reinterpret_cast<struct aiocb *>(aiocbp), false);
#else
	MAP_OR_FAIL(aio_read64);
	ret = __real_aio_read64(aiocbp);
#endif
	return (ret);
}

//! ----------------------- Wait and Test ------------------------------
/**
 * @brief Trace the only actual completion of an asynchronous I/O operation.
 * @param aiocbp Pointer to the aiocb structure representing the I/O operation.
 * @return The number of bytes read or written, or -1 on error.
 */
int TMIO_DECL(aio_error)(const struct aiocb *aiocbp)
{
	std::string function_name = __PRETTY_FUNCTION__;
	// Also append the fd to the function name for better debugging
	function_name += " (fd: " + std::to_string(aiocbp->aio_fildes) + ")";
	Function_Debug(function_name);

	int ret;

#if BW_LIMIT_POSIX_AIO == 1
	ret = tmio_posix_aio::error_of(aiocbp);
#else
	MAP_OR_FAIL(aio_error);
	ret = __real_aio_error(aiocbp);
#endif

    switch (ret)
    {
        case EINPROGRESS:
            // Request has not been completed yet; do not trace
            break;
        case ECANCELED:
            // Request was canceled; trace and print error
            get_libc_iotrace().Read_Async_End(aiocbp);
            get_libc_iotrace().Write_Async_End(aiocbp);
            fprintf(stderr, "TMIO > aio_error with aio_fildes: %d was canceled (ECANCELED: %s)\n", aiocbp->aio_fildes, strerror(ECANCELED));
            break;
        case 0:
            // Request completed successfully; trace
            get_libc_iotrace().Read_Async_End(aiocbp);
            get_libc_iotrace().Write_Async_End(aiocbp);
            break;
        default:
            if (ret > 0) {
                // A positive error number: operation failed; trace and print error
                get_libc_iotrace().Read_Async_End(aiocbp);
                get_libc_iotrace().Write_Async_End(aiocbp);
                fprintf(stderr, "TMIO > aio_error with aio_fildes: %d failed with error code: %s\n", aiocbp->aio_fildes, strerror(ret));
            }
            break;
    }

	return (ret);
}

/**
 * @brief Cancel one (or, with aiocbp == NULL, all) outstanding AIO request(s) for a file descriptor.
 * @return AIO_CANCELED, AIO_NOTCANCELED, or AIO_ALLDONE (see aio.h), or -1/errno on failure.
 */
int TMIO_DECL(aio_cancel)(int fd, struct aiocb *aiocbp)
{
	Function_Debug(__PRETTY_FUNCTION__);
	int ret;

#if BW_LIMIT_POSIX_AIO == 1
	ret = aiocbp ? tmio_posix_aio::cancel_one(aiocbp) : tmio_posix_aio::cancel_all(fd);
#else
	MAP_OR_FAIL(aio_cancel);
	ret = __real_aio_cancel(fd, aiocbp);
#endif
	return (ret);
}

int TMIO_DECL(aio_cancel64)(int fd, struct aiocb64 *aiocbp)
{
	Function_Debug(__PRETTY_FUNCTION__);
#if BW_LIMIT_POSIX_AIO == 1
	return aio_cancel(fd, reinterpret_cast<struct aiocb *>(aiocbp));
#else
	MAP_OR_FAIL(aio_cancel64);
	return __real_aio_cancel64(fd, aiocbp);
#endif
}

/**
 * @brief Record the required finished time for those AIO requests.
 * @note TMIO should treat a call to aio_suspend as an explicit indication from the user that
 * 		 they are waiting for the specified AIO requests to complete.
 *
 * 		 Therefore, the point at which aio_suspend are called (the relevant request(s) should be
 * 		 later confirmed complete via aio_error) would be used to record the required finished time for those AIO requests.
 * @param aiocbp Pointer to the aiocb structure representing the I/O operation.
 * @return The number of bytes read or written, or -1 on error.
 */
int TMIO_DECL(aio_suspend)(const struct aiocb *const aiocb_list[], int nitems, const struct timespec *__restrict__ timeout)
{
	Function_Debug(__PRETTY_FUNCTION__);
	int ret;

	for (int i = 0; i < nitems; ++i)
	{
		if (aiocb_list[i] != nullptr)
		{
			get_libc_iotrace().Read_Async_Required(aiocb_list[i]);
			get_libc_iotrace().Write_Async_Required(aiocb_list[i]);
		}
	}
#if BW_LIMIT_POSIX_AIO == 1
	ret = tmio_posix_aio::suspend(aiocb_list, nitems, timeout);
#else
	MAP_OR_FAIL(aio_suspend);
	ret = __real_aio_suspend(aiocb_list, nitems, timeout);
#endif
	return (ret);
}

int TMIO_DECL(aio_suspend64)(const struct aiocb64 *const aiocb_list[], int nitems, const struct timespec *__restrict__ timeout)
{
	Function_Debug(__PRETTY_FUNCTION__);

	return aio_suspend(reinterpret_cast<const struct aiocb *const *>(aiocb_list), nitems, timeout); // Cast aiocb64 to aiocb
}

/**
 * @brief Fallback for `aio_suspend`
 *
 * @note For actual time: aio_return further confirms the completion and
 * 		could be a point to finalize the recording of the "actual" finished time
 * 		if not already definitively captured.
 *
 * 		For required time: If TMIO hasn't already established a "required" finish time
 * 		for an AIO request (e.g., through an encompassing aio_suspend),
 * 		could the call to aio_return for that request serve as a fallback
 * 		to record its "required" finished time. This bhv is configurable by
 * 		preprocessing flags to disable this option if needed.
 */
ssize_t TMIO_DECL(aio_return)(struct aiocb *aiocbp)
{
	Function_Debug(__PRETTY_FUNCTION__);
	ssize_t ret;

#if BW_LIMIT_POSIX_AIO == 1
	ret = tmio_posix_aio::return_of(aiocbp);
#else
	MAP_OR_FAIL(aio_return);
	ret = __real_aio_return(aiocbp);
#endif

	if (ret != -1) // If success, trace
	{
		// Further confirm the completion of actual time
		get_libc_iotrace().Read_Async_End(aiocbp);
		get_libc_iotrace().Write_Async_End(aiocbp);

		// Fallback for required time
		get_libc_iotrace().Read_Async_Required(aiocbp);
		get_libc_iotrace().Write_Async_Required(aiocbp);
	}

	return (ret);
}

ssize_t TMIO_DECL(aio_return64)(struct aiocb64 *aiocbp)
{
	Function_Debug(__PRETTY_FUNCTION__);

	return aio_return(reinterpret_cast<struct aiocb *>(aiocbp)); // Cast aiocb64 to aiocb
}
// //! ----------------------- List I/O ------------------------------
int TMIO_DECL(lio_listio)(int mode, struct aiocb *const aiocb_list[], int nitems, struct sigevent *sevp)
{
	Function_Debug(__PRETTY_FUNCTION__);

	ssize_t ret;
	MAP_OR_FAIL(lio_listio);

	if (mode == LIO_WAIT) // Sync
	{
#if BATCH_LIO == 1
		// Merge all aiocb into one overarching request, but only for tracing
		int batch_read_iovcnt = 0;
		int batch_write_iovcnt = 0;
		const static int OFFSET = INT32_MAX; // Use a large offset to avoid conflicts

		for (int i = 0; i < nitems; ++i)
		{
			if (aiocb_list[i] != nullptr)
			{
				if (aiocb_list[i]->aio_lio_opcode == LIO_READ)
					batch_read_iovcnt += aiocb_list[i]->aio_nbytes;
				else if (aiocb_list[i]->aio_lio_opcode == LIO_WRITE)
					batch_write_iovcnt += aiocb_list[i]->aio_nbytes;
				else
					continue; // LIO_NOP
			}
		}

		if (batch_read_iovcnt > 0)
			get_libc_iotrace().Read_Sync_Start(batch_read_iovcnt, OFFSET);

		if (batch_write_iovcnt > 0)
			get_libc_iotrace().Write_Sync_Start(batch_write_iovcnt, OFFSET);
#else
		for (int i = 0; i < nitems; ++i)
		{
			if (aiocb_list[i] != nullptr)
			{
				if (aiocb_list[i]->aio_lio_opcode == LIO_READ)
					get_libc_iotrace().Batch_Read_Sync_Start(aiocb_list[i]->aio_nbytes, aiocb_list[i]->aio_offset);
				else if (aiocb_list[i]->aio_lio_opcode == LIO_WRITE)
					get_libc_iotrace().Batch_Write_Sync_Start(aiocb_list[i]->aio_nbytes, aiocb_list[i]->aio_offset);
				else
					continue; // LIO_NOP
			}
		}

#endif

#if BW_LIMIT_POSIX_AIO == 1
		ret = tmio_posix_aio::submit_batch(aiocb_list, nitems);
		if (ret == 0)
			ret = tmio_posix_aio::wait_batch(aiocb_list, nitems);
#else
		ret = __real_lio_listio(mode, aiocb_list, nitems, sevp);
#endif

		if (ret != 0)
			std::cerr << "TMIO > lio_listio(LIO_WAIT) failed or interrupted: " << strerror(errno) << " (errno=" << errno << "). TMIO tracing result might be inaccurate. Better to call the lio_list again" << std::endl;

// End the sync tracing
#if BATCH_LIO == 1
		if (batch_read_iovcnt > 0)
			get_libc_iotrace().Read_Sync_End();
		if (batch_write_iovcnt > 0)
			get_libc_iotrace().Write_Sync_End();
#else
		for (int i = 0; i < nitems; ++i)
		{
			if (aiocb_list[i] != nullptr)
			{
				if (aiocb_list[i]->aio_lio_opcode == LIO_READ)
					get_libc_iotrace().Batch_Read_Sync_End();
				else if (aiocb_list[i]->aio_lio_opcode == LIO_WRITE)
					get_libc_iotrace().Batch_Write_Sync_End();
				else
					continue; // LIO_NOP
			}
		}
#endif
	}
	else if (mode == LIO_NOWAIT)
	{
		for (int i = 0; i < nitems; ++i)
		{
			if (aiocb_list[i] != nullptr)
			{
				get_libc_iotrace().Read_Async_Start(aiocb_list[i]);
				get_libc_iotrace().Write_Async_Start(aiocb_list[i]);
			}
		}

#if BW_LIMIT_POSIX_AIO == 1
		ret = tmio_posix_aio::submit_batch(aiocb_list, nitems);
#else
		ret = __real_lio_listio(mode, aiocb_list, nitems, sevp);
#endif

		// We cannot use aio_err to check each request.
		// If we use it to check a non-initialized aiocb, it would return the same result as success call.
		if (ret != 0)
			std::cerr << "TMIO > lio_listio(LIO_NOWAIT) failed: " << strerror(errno) << " (errno=" << errno << "). TMIO tracing result might be inaccurate. Better to call the lio_list again" << std::endl;
	} else
	{
		std::cerr << "TMIO > lio_listio: unsupported mode: " << mode << ". Only LIO_WAIT and LIO_NOWAIT are supported." << std::endl;
	}
	return (ret);
}

int TMIO_DECL(lio_listio64)(int mode, struct aiocb64 *const aiocb_list[], int nitems, struct sigevent *sevp)
{
	Function_Debug(__PRETTY_FUNCTION__);

	// aiocb and aiocb64 share layout on 64-bit Linux (see aio_return64/aio_suspend64
	// above, which rely on the same assumption).
	return lio_listio(mode, reinterpret_cast<struct aiocb *const *>(aiocb_list), nitems, sevp);
}
//! ----------------------- Sync Read & Write ------------------------------

ssize_t TMIO_DECL(read)(int fd, void *buf, size_t count)
{
	Function_Debug(__PRETTY_FUNCTION__);
	ssize_t ret;

	MAP_OR_FAIL(read);

	get_libc_iotrace().Read_Sync_Start(count);
	ret = __real_read(fd, buf, count);
	get_libc_iotrace().Read_Sync_End();

	return (ret);
}

ssize_t TMIO_DECL(write)(int fd, const void *buf, size_t count)
{
	Function_Debug(__PRETTY_FUNCTION__);
	ssize_t ret;

	MAP_OR_FAIL(write);

	get_libc_iotrace().Write_Sync_Start(count);
	ret = __real_write(fd, buf, count);
	get_libc_iotrace().Write_Sync_End();

	return (ret);
}

ssize_t TMIO_DECL(pread)(int fd, void *buf, size_t count, off_t offset)
{
	Function_Debug(__PRETTY_FUNCTION__);
	ssize_t ret;

	MAP_OR_FAIL(pread);

	get_libc_iotrace().Read_Sync_Start(count, static_cast<off64_t>(offset));
	ret = __real_pread(fd, buf, count, offset);
	get_libc_iotrace().Read_Sync_End();

	return (ret);
}
ssize_t TMIO_DECL(pread64)(int fd, void *buf, size_t count, off64_t offset)
{
	Function_Debug(__PRETTY_FUNCTION__);
	ssize_t ret;

	MAP_OR_FAIL(pread64);

	get_libc_iotrace().Read_Sync_Start(count, offset);
	ret = __real_pread64(fd, buf, count, offset);
	get_libc_iotrace().Read_Sync_End();

	return (ret);
}
ssize_t TMIO_DECL(pwrite)(int fd, const void *buf, size_t count, off_t offset)
{
	Function_Debug(__PRETTY_FUNCTION__);
	ssize_t ret;

	MAP_OR_FAIL(pwrite);

	get_libc_iotrace().Write_Sync_Start(count, static_cast<off64_t>(offset));
	ret = __real_pwrite(fd, buf, count, offset);
	get_libc_iotrace().Write_Sync_End();

	return (ret);
}
ssize_t TMIO_DECL(pwrite64)(int fd, const void *buf, size_t count, off64_t offset)
{
	Function_Debug(__PRETTY_FUNCTION__);
	ssize_t ret;

	MAP_OR_FAIL(pwrite64);

	get_libc_iotrace().Write_Sync_Start(count, offset);
	ret = __real_pwrite64(fd, buf, count, offset);
	get_libc_iotrace().Write_Sync_End();
	return (ret);
}

//! ----------------------- Readv and Writev ------------------------------
ssize_t TMIO_DECL(readv)(int fd, const struct iovec *iov, int iovcnt, off_t offset)
{
	Function_Debug(__PRETTY_FUNCTION__);
	ssize_t ret;

	MAP_OR_FAIL(readv);

	get_libc_iotrace().Read_Sync_Start(iovcnt, static_cast<off64_t>(offset));
	ret = __real_readv(fd, iov, iovcnt, offset);
	get_libc_iotrace().Read_Sync_End();

	return (ret);
}
ssize_t TMIO_DECL(writev)(int fd, const struct iovec *iov, int iovcnt)
{
	Function_Debug(__PRETTY_FUNCTION__);
	ssize_t ret;

	MAP_OR_FAIL(writev);

	get_libc_iotrace().Write_Sync_Start(iovcnt);
	ret = __real_writev(fd, iov, iovcnt);
	get_libc_iotrace().Write_Sync_End();

	return (ret);
}
#ifdef HAVE_PREADV
ssize_t TMIO_DECL(preadv)(int fd, const struct iovec *iov, int iovcnt, off_t offset)
{
	Function_Debug(__PRETTY_FUNCTION__);
	ssize_t ret;

	MAP_OR_FAIL(preadv);

	get_libc_iotrace().Read_Sync_Start(iovcnt, static_cast<off64_t>(offset));
	ret = __real_preadv(fd, iov, iovcnt, offset);
	get_libc_iotrace().Read_Sync_End();

	return (ret);
}
ssize_t TMIO_DECL(preadv64)(int fd, const struct iovec *iov, int iovcnt, off64_t offset)
{
	Function_Debug(__PRETTY_FUNCTION__);
	ssize_t ret;

	MAP_OR_FAIL(preadv64);

	get_libc_iotrace().Read_Sync_Start(iovcnt, offset);
	ret = __real_preadv64(fd, iov, iovcnt, offset);
	get_libc_iotrace().Read_Sync_End();

	return (ret);
}
ssize_t TMIO_DECL(pwritev)(int fd, const struct iovec *iov, int iovcnt, off_t offset)
{
	Function_Debug(__PRETTY_FUNCTION__);
	ssize_t ret;

	MAP_OR_FAIL(pwritev);

	get_libc_iotrace().Write_Sync_Start(iovcnt, static_cast<off64_t>(offset));
	ret = __real_pwritev(fd, iov, iovcnt, offset);
	get_libc_iotrace().Write_Sync_End();
	return (ret);
}
ssize_t TMIO_DECL(pwritev64)(int fd, const struct iovec *iov, int iovcnt, off64_t offset)
{
	Function_Debug(__PRETTY_FUNCTION__);
	ssize_t ret;

	MAP_OR_FAIL(pwritev64);

	get_libc_iotrace().Write_Sync_Start(iovcnt, offset);
	ret = __real_pwritev64(fd, iov, iovcnt, offset);
	get_libc_iotrace().Write_Sync_End();
	return (ret);
}
#ifdef HAVE_PREADV2
ssize_t TMIO_DECL(preadv2)(int fd, const struct iovec *iov, int iovcnt, off_t offset, int flags)
{
	Function_Debug(__PRETTY_FUNCTION__);
	ssize_t ret;

	MAP_OR_FAIL(preadv2);

	get_libc_iotrace().Read_Sync_Start(iovcnt, static_cast<off64_t>(offset));
	ret = __real_preadv2(fd, iov, iovcnt, offset, flags);
	get_libc_iotrace().Read_Sync_End();

	return (ret);
}
ssize_t TMIO_DECL(preadv64v2)(int fd, const struct iovec *iov, int iovcnt, off64_t offset, int flags)
{
	Function_Debug(__PRETTY_FUNCTION__);
	ssize_t ret;

	MAP_OR_FAIL(preadv64v2);

	get_libc_iotrace().Read_Sync_Start(iovcnt, offset);
	ret = __real_preadv64v2(fd, iov, iovcnt, offset, flags);
	get_libc_iotrace().Read_Sync_End();

	return (ret);
}
ssize_t TMIO_DECL(pwritev2)(int fd, const struct iovec *iov, int iovcnt, off_t offset, int flags)
{
	Function_Debug(__PRETTY_FUNCTION__);
	ssize_t ret;

	MAP_OR_FAIL(pwritev2);

	get_libc_iotrace().Write_Sync_Start(iovcnt, static_cast<off64_t>(offset));
	ret = __real_pwritev2(fd, iov, iovcnt, offset, flags);
	get_libc_iotrace().Write_Sync_End();
	return (ret);
}
ssize_t TMIO_DECL(pwritev64v2)(int fd, const struct iovec *iov, int iovcnt, off64_t offset, int flags)
{
	Function_Debug(__PRETTY_FUNCTION__);
	ssize_t ret;

	MAP_OR_FAIL(pwritev64v2);

	get_libc_iotrace().Write_Sync_Start(iovcnt, offset);
	ret = __real_pwritev64v2(fd, iov, iovcnt, offset, flags);
	get_libc_iotrace().Write_Sync_End();
	return (ret);
}
#endif // HAVE_PREADV2
#endif // HAVE_PREADV

#endif // ENABLE_LIBC_TRACE