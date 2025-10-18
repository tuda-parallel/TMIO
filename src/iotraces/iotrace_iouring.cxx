#include <liburing.h>
#include <sys/uio.h> // For struct iovec
#include <stdexcept> // For std::runtime_error
#include <string>    // For std::to_string
#include <cstdlib>   // For abort()
#include "iotrace.h"

namespace
{
    // struct to hold parsed SQE info
    struct UringSqeInfo
    {
        bool is_write;
        long long total_size;
        off_t offset;
        __u64 user_data;
    };

    /**
     * @brief Parses an io_uring_sqe to extract common tracing information.
     * @param sqe The Submission Queue Entry to parse.
     * @return A UringSqeInfo struct containing the parsed data.
     * @throws std::runtime_error if the sqe->opcode is not supported for tracing.
     */
    UringSqeInfo parse_sqe_info(const struct io_uring_sqe *sqe)
    {
        UringSqeInfo info = {false, 0, 0, 0};
        info.offset = sqe->off;
        info.user_data = sqe->user_data;

        switch (sqe->opcode)
        {
        case IORING_OP_WRITEV:
        case IORING_OP_READV:
        {
            info.is_write = (sqe->opcode == IORING_OP_WRITEV);
            const struct iovec *iovs = reinterpret_cast<const struct iovec *>(sqe->addr);
            for (unsigned i = 0; i < sqe->len; ++i)
            {
                info.total_size += iovs[i].iov_len;
            }
            break;
        }

        case IORING_OP_WRITE_FIXED:
        case IORING_OP_WRITE:
        case IORING_OP_READ_FIXED:
        case IORING_OP_READ:
        {
            info.is_write = (sqe->opcode == IORING_OP_WRITE_FIXED || sqe->opcode == IORING_OP_WRITE);
            info.total_size = sqe->len;
            break;
        }

        default:
            // Panic by throwing an exception for unsupported opcodes.
            throw std::runtime_error("Unsupported io_uring opcode for tracing: " + std::to_string(sqe->opcode));
        }

        return info;
    }

} // anonymous namespace

// ===================================================================================
// == Constructor / Destructor
// ===================================================================================

IOtraceIOuring::IOtraceIOuring()
    : keep_polling_(true),
      staged_requests_queue_(1024) // Size must be a power of two
{
    polling_thread_ = std::thread(&IOtraceIOuring::polling_loop, this);
}

IOtraceIOuring::~IOtraceIOuring()
{
    if (polling_thread_.joinable())
    {
        keep_polling_ = false;
        polling_cond_var_.notify_one();
        polling_thread_.join();
    }
}

// ===================================================================================
// == Public API
// ===================================================================================

void IOtraceIOuring::Register_Ring(struct io_uring *ring)
{
    std::lock_guard<std::mutex> lock(ring_processing_lock_);
    if (find_ring_state(ring) == nullptr)
    {
        tracked_rings_.emplace_back(ring);
    }
}

void IOtraceIOuring::Unregister_Ring(struct io_uring *ring)
{
    std::lock_guard<std::mutex> lock(ring_processing_lock_);
    tracked_rings_.erase(
        std::remove_if(tracked_rings_.begin(), tracked_rings_.end(),
                       [ring](const RingTraceState &state)
                       { return state.ring_ptr == ring; }),
        tracked_rings_.end());
}

void IOtraceIOuring::Process_Submissions(struct io_uring *ring)
{
    double start_time = MPI_Wtime() - t_0;
    // First, scan the SQ ring to find what's new. We do this outside the lock.
    RingTraceState *state = find_ring_state(ring);
    if (!state)
    {
        // Ring not registered yet, can't process.
        // This case should be handled by ensuring Register_Ring is always called first.
        return;
    }

    struct io_uring_sq *sq = &ring->sq;
    unsigned current_tail = sq->sqe_tail;
    unsigned last_known_tail = state->last_known_sq_tail;

    // Now, attempt to become the worker.
    if (ring_processing_lock_.try_lock())
    {
        // SUCCESS: We got the lock. Process new SQEs directly and then process all other work.
        for (unsigned i = last_known_tail; i < current_tail; ++i)
        {
            const struct io_uring_sqe *sqe = &sq->sqes[i & sq->ring_mask];
            try
            {
                UringSqeInfo info = parse_sqe_info(sqe);
                if (info.is_write)
                {
                    state->pending_writes.insert(info.user_data);
                    Write_Async_Start(info.user_data, info.total_size, info.offset, start_time);
                }
                else
                {
                    state->pending_reads.insert(info.user_data);
                    Read_Async_Start(info.user_data, info.total_size, info.offset, start_time);
                }
            }
            catch (const std::runtime_error &e)
            { /* Ignore */
            }
        }
        // After processing, claim all the new SQEs.
        state->last_known_sq_tail = current_tail;

        // Now that we've handled the immediate work, process everything else.
        process_rings_locked();
        ring_processing_lock_.unlock();
    }
    else
    {
        // FAILURE: The polling thread is busy. Handoff new SQEs to the lock-free queue.
        for (unsigned i = last_known_tail; i < current_tail; ++i)
        {
            const struct io_uring_sqe *sqe = &sq->sqes[i & sq->ring_mask];
            try
            {
                UringSqeInfo info = parse_sqe_info(sqe);
                double submission_time = MPI_Wtime() - t_0;
                StagedRequest req = {ring, info.user_data, info.total_size, info.offset, info.is_write, submission_time};
                if (!staged_requests_queue_.push(req))
                {
                    Log<VerbosityLevel::BASIC_LOG>("%s > rank %i > TMIO warning: io_uring staging queue is full. Request may be delayed.\n", caller, rank);
                    // If the queue is full, we must break. The requests that were not pushed
                    // will be processed on a subsequent call because last_known_sq_tail has not been advanced for them.
                    break;
                }
                // Atomically "claim" the SQE that was just successfully pushed.
                // This is the key to preventing lost or duplicated requests.
                state->last_known_sq_tail++;
            }
            catch (const std::runtime_error &e)
            { /* Ignore */
                // Also advance the tail for unsupported opcodes to avoid processing them again.
                state->last_known_sq_tail++;
            }
        }
        // IMPORTANT: We don't update last_known_sq_tail here. The worker thread will do that
        // after it has processed these requests from the SPSC queue.
    }

    // Always notify the polling thread.
    polling_cond_var_.notify_one();
}

void IOtraceIOuring::Process_Completions(struct io_uring *ring)
{
    // This function can also use the try_lock pattern for responsiveness.
    if (ring_processing_lock_.try_lock())
    {
        RingTraceState *state = find_ring_state(ring);
        if (state)
        {
            reap_completions_locked(*state);
        }
        ring_processing_lock_.unlock();
    }
    polling_cond_var_.notify_one();
}

void IOtraceIOuring::Mark_All_Pending_As_Required(struct io_uring *ring)
{
    std::lock_guard<std::mutex> lock(ring_processing_lock_);
    RingTraceState *state = find_ring_state(ring);
    if (!state)
        return;

    for (const auto &req_id : state->pending_writes)
    {
        Write_Async_Required(req_id);
    }
    for (const auto &req_id : state->pending_reads)
    {
        Read_Async_Required(req_id);
    }
}

// ===================================================================================
// == Private Helper Functions
// ===================================================================================

IOtraceIOuring::RingTraceState *IOtraceIOuring::find_ring_state(struct io_uring *ring)
{
    for (auto &state : tracked_rings_)
    {
        if (state.ring_ptr == ring)
        {
            return &state;
        }
    }
    return nullptr;
}

void IOtraceIOuring::drain_staged_requests_locked()
{
    StagedRequest req;
    while (staged_requests_queue_.pop(req))
    {
        RingTraceState *state = find_ring_state(req.ring);
        if (!state)
            continue; // Ring was unregistered in the meantime
        if (req.is_write)
        {
            if (state->pending_writes.find(req.user_data) == state->pending_writes.end())
            {
                state->pending_writes.insert(req.user_data);
                Write_Async_Start(req.user_data, req.total_size, req.offset, req.submission_time);
            }
        }
        else
        {
            if (state->pending_reads.find(req.user_data) == state->pending_reads.end())
            {
                state->pending_reads.insert(req.user_data);
                Read_Async_Start(req.user_data, req.total_size, req.offset, req.submission_time);
            }
        }
    }
}

void IOtraceIOuring::process_rings_locked()
{
    // First, drain the handoff queue.
    drain_staged_requests_locked();

    // Then, process all rings for completions.
    for (auto &state : tracked_rings_)
    {
        reap_completions_locked(state);
    }
}

/**
 * @brief Checks a ring for completed requests and processes them.
 */
void IOtraceIOuring::reap_completions_locked(RingTraceState &state)
{
    struct io_uring *ring = state.ring_ptr;
    struct io_uring_cq *cq = &ring->cq;
    unsigned head = state.last_known_cq_head;
    unsigned cqe_count = 0;

    // Manually implement the loop instead of using the io_uring_for_each_cqe macro
    unsigned ring_tail = io_uring_smp_load_acquire(cq->ktail);
    struct io_uring_cqe *cqe;

    while (head != ring_tail)
    {
        cqe = &cq->cqes[head & cq->ring_mask];
        cqe_count++;
        __u64 user_data = cqe->user_data;

        if (state.pending_writes.erase(user_data))
        {
            Write_Async_End(user_data, cqe->res);
        }
        else if (state.pending_reads.erase(user_data))
        {
            Read_Async_End(user_data, cqe->res);
        }
        head++;
    }

    const unsigned ring_head = *cq->khead;
    if (ring_head > state.last_known_cq_head + cq->ring_sz)
    {
        // If the khead is go over the last known head, might be the risk of losting some completions.
        Log<VerbosityLevel::BASIC_LOG>("%s > rank %i > TMIO warning: io_uring completion queue head jumped from %u to %u. Some completions may have been lost.\n",
                                       caller, rank, state.last_known_cq_head, ring_head);
    }

    state.last_known_cq_head = head;
}

void IOtraceIOuring::polling_loop()
{
    std::unique_lock<std::mutex> lock(ring_processing_lock_, std::defer_lock);

    while (keep_polling_)
    {
        lock.lock();
        polling_cond_var_.wait(lock, [this]
                               {
            bool has_pending_kernel_reqs = false;
            for(const auto& state : tracked_rings_) {
                if (state.pending_request_count() > 0) {
                    has_pending_kernel_reqs = true;
                    break;
                }
            }
            // Wake up if stopping, if the handoff queue has items, or if requests are in-flight.
            return !keep_polling_ || !staged_requests_queue_.is_empty() || has_pending_kernel_reqs; });

        if (!keep_polling_)
        {
            lock.unlock();
            break;
        }

        process_rings_locked();
        lock.unlock();
    }
}

// ===================================================================================
// == Private Implementation Details (Simplified)
// ===================================================================================
void IOtraceIOuring::Write_Async_Start(RequestIDType requestID, long long size, long long offset, double start_time)
{
    Write_Async_Start_Impl(requestID, size, offset, start_time);
}
void IOtraceIOuring::Write_Async_End(RequestIDType requestID, int status)
{
    int write_status;
    if (status >= 0)
    {
        write_status = 1; // Success
    }
    else
    {
        write_status = 0; // Propagate error code
    }
    Write_Async_Required(requestID);
    Write_Async_End_Impl(requestID, write_status);
}
void IOtraceIOuring::Read_Async_Start(RequestIDType requestID, long long size, long long offset, double start_time)
{
    Read_Async_Start_Impl(requestID, size, offset, start_time);
}
void IOtraceIOuring::Read_Async_End(RequestIDType requestID, int status)
{
    int read_status;
    if (status >= 0)
    {
        read_status = 1; // Success
    }
    else
    {
        read_status = 0; // Propagate error code
    }
    Read_Async_Required(requestID);
    Read_Async_End_Impl(requestID, read_status);
}
void IOtraceIOuring::Write_Async_Required(RequestIDType requestID)
{
    Write_Async_Required_Impl(requestID);
}
void IOtraceIOuring::Read_Async_Required(RequestIDType requestID)
{
    Read_Async_Required_Impl(requestID);
}