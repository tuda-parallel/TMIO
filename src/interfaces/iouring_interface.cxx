#include "tmio.h"
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <stdio.h>
#include <time.h>
#include <liburing.h>

#if ENABLE_IOURING_TRACE == 1
IOtraceIOuring &get_iouring_iotrace()
{
    // This is thread_local, ensuring each thread gets its own instance of the tracer.
    // For single-threaded MPI, this will effectively be a single instance.
    static thread_local IOtraceIOuring instance;
    return instance;
}

// --- Forward Declarations for all intercepted functions ---
TMIO_FORWARD_DECL(io_uring_queue_init, int, (unsigned entries, struct io_uring *ring, unsigned flags));
TMIO_FORWARD_DECL(io_uring_queue_init_params, int, (unsigned entries, struct io_uring *ring, struct io_uring_params *p));
TMIO_FORWARD_DECL(io_uring_queue_exit, void, (struct io_uring *ring));

TMIO_FORWARD_DECL(io_uring_submit, int, (struct io_uring *ring));
TMIO_FORWARD_DECL(io_uring_submit_and_wait, int, (struct io_uring *ring, unsigned wait_nr));
TMIO_FORWARD_DECL(io_uring_submit_and_wait_timeout, int, (struct io_uring *ring, struct io_uring_cqe **cqe_ptr, unsigned wait_nr, struct __kernel_timespec *ts, sigset_t *sigmask));

TMIO_FORWARD_DECL(io_uring_wait_cqes, int, (struct io_uring *ring, struct io_uring_cqe **cqe_ptr, unsigned wait_nr, struct __kernel_timespec *ts, sigset_t *sigmask));
TMIO_FORWARD_DECL(io_uring_wait_cqe_timeout, int, (struct io_uring *ring, struct io_uring_cqe **cqe_ptr, struct __kernel_timespec *ts));

TMIO_FORWARD_DECL(__io_uring_get_cqe, int, (struct io_uring *ring, struct io_uring_cqe **cqe_ptr, unsigned submit, unsigned wait_nr, sigset_t *sigmask));

TMIO_FORWARD_DECL(io_uring_peek_batch_cqe, unsigned, (struct io_uring *ring, struct io_uring_cqe **cqes, unsigned count));


// --- Interception Implementations ---

// Initialization and Exit
int TMIO_DECL(io_uring_queue_init)(unsigned entries, struct io_uring *ring, unsigned flags)
{
    Function_Debug(__PRETTY_FUNCTION__);
    MAP_OR_FAIL(io_uring_queue_init);

    int ret = __real_io_uring_queue_init(entries, ring, flags);
    if (ret == 0)
    {
        get_iouring_iotrace().Register_Ring(ring);
    }
    return (ret);
}

int TMIO_DECL(io_uring_queue_init_params)(unsigned entries, struct io_uring *ring, struct io_uring_params *p)
{
    Function_Debug(__PRETTY_FUNCTION__);
    MAP_OR_FAIL(io_uring_queue_init_params);

    int ret = __real_io_uring_queue_init_params(entries, ring, p);
    if (ret == 0)
    {
        get_iouring_iotrace().Register_Ring(ring);
    }
    return (ret);
}

void TMIO_DECL(io_uring_queue_exit)(struct io_uring *ring)
{
    Function_Debug(__PRETTY_FUNCTION__);
    MAP_OR_FAIL(io_uring_queue_exit);

    get_iouring_iotrace().Unregister_Ring(ring);
    __real_io_uring_queue_exit(ring);
}


// Submission
int TMIO_DECL(io_uring_submit)(struct io_uring *ring)
{
    Function_Debug(__PRETTY_FUNCTION__);
    MAP_OR_FAIL(io_uring_submit);

    get_iouring_iotrace().Process_Submissions(ring);
    int ret = __real_io_uring_submit(ring);
    return (ret);
}

int TMIO_DECL(io_uring_submit_and_wait)(struct io_uring *ring, unsigned wait_nr)
{
    Function_Debug(__PRETTY_FUNCTION__);
    MAP_OR_FAIL(io_uring_submit_and_wait);

    get_iouring_iotrace().Process_Submissions(ring);
    get_iouring_iotrace().Mark_All_Pending_As_Required(ring);
    int ret = __real_io_uring_submit_and_wait(ring, wait_nr);
    get_iouring_iotrace().Process_Completions(ring);
    return (ret);
}

int TMIO_DECL(io_uring_submit_and_wait_timeout)(struct io_uring *ring, struct io_uring_cqe **cqe_ptr, unsigned wait_nr, struct __kernel_timespec *ts, sigset_t *sigmask)
{
    Function_Debug(__PRETTY_FUNCTION__);
    MAP_OR_FAIL(io_uring_submit_and_wait_timeout);

    get_iouring_iotrace().Process_Submissions(ring);
    get_iouring_iotrace().Mark_All_Pending_As_Required(ring);
    int ret = __real_io_uring_submit_and_wait_timeout(ring, cqe_ptr, wait_nr, ts, sigmask);
    get_iouring_iotrace().Process_Completions(ring);
    return (ret);
}


// Completion
int TMIO_DECL(io_uring_wait_cqes)(struct io_uring *ring, struct io_uring_cqe **cqe_ptr, unsigned wait_nr, struct __kernel_timespec *ts, sigset_t *sigmask)
{
    Function_Debug(__PRETTY_FUNCTION__);
    MAP_OR_FAIL(io_uring_wait_cqes);

    get_iouring_iotrace().Mark_All_Pending_As_Required(ring);
    int ret = __real_io_uring_wait_cqes(ring, cqe_ptr, wait_nr, ts, sigmask);
    get_iouring_iotrace().Process_Completions(ring);
    return (ret);
}

int TMIO_DECL(io_uring_wait_cqe_timeout)(struct io_uring *ring, struct io_uring_cqe **cqe_ptr, struct __kernel_timespec *ts)
{
    Function_Debug(__PRETTY_FUNCTION__);
    MAP_OR_FAIL(io_uring_wait_cqe_timeout);

    get_iouring_iotrace().Mark_All_Pending_As_Required(ring);
    int ret = __real_io_uring_wait_cqe_timeout(ring, cqe_ptr, ts);
    get_iouring_iotrace().Process_Completions(ring);
    return (ret);
}

// NOTE: io_uring_wait_cqe and io_uring_peek_cqe are static inline functions.
// They call __io_uring_get_cqe internally, which we intercept here.
int TMIO_DECL(__io_uring_get_cqe)(struct io_uring *ring, struct io_uring_cqe **cqe_ptr, unsigned submit, unsigned wait_nr, sigset_t *sigmask)
{
    Function_Debug(__PRETTY_FUNCTION__);
    MAP_OR_FAIL(__io_uring_get_cqe);

    // If the call includes a submit, we must process submissions first.
    if (submit > 0) {
        get_iouring_iotrace().Process_Submissions(ring);
    }

    // If the call is waiting (wait_nr > 0), we must mark pending requests as required.
    if (wait_nr > 0) {
        get_iouring_iotrace().Mark_All_Pending_As_Required(ring);
    } else {
        // If it's just a peek (wait_nr == 0), we should still process any ready completions.
        get_iouring_iotrace().Process_Completions(ring);
    }

    int ret = __real___io_uring_get_cqe(ring, cqe_ptr, submit, wait_nr, sigmask);

    // After the call returns, process any new completions.
    get_iouring_iotrace().Process_Completions(ring);

    return ret;
}

unsigned TMIO_DECL(io_uring_peek_batch_cqe)(struct io_uring *ring, struct io_uring_cqe **cqes, unsigned count)
{
    Function_Debug(__PRETTY_FUNCTION__);
    MAP_OR_FAIL(io_uring_peek_batch_cqe);

    get_iouring_iotrace().Process_Completions(ring);
    unsigned ret = __real_io_uring_peek_batch_cqe(ring, cqes, count);
    get_iouring_iotrace().Process_Completions(ring);
    return ret;
}

#endif // ENABLE_IOURING_TRACE == 1