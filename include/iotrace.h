#ifndef IOTRACE_H
#define IOTRACE_H

#include "tmio_helper_functions.h"
#include <shared_mutex>
#include <mutex>
#include <cstdarg>
#include <atomic>
#include <unordered_map>
#include <thread>
#include <condition_variable>
#include <vector>
#include <cassert>
#include <unordered_set>

#include "ioflags.h"
#if ENABLE_IOURING_TRACE == 1
#include <liburing.h>
#endif // ENABLE_IOURING_TRACE

#if defined BW_LIMIT || defined CUSTOM_MPI
#include "bw_limit.h"
#else
#include "ioanalysis.h"
#endif

/**
 *  IO trace class
 * @file   iotrace.h
 * @author Ahmad Tarraf
 * @date   05.08.2021
 */

// * @brief Dynamic tag dispatching of IOtrace
// * @details This is a template class that uses a tag to determine the type of IOtrace to use.
// * The tag can be either MPI_Tag or Libc_Tag, which correspond to MPI I/O and libc I/O respectively.
// * The IOtraceBase class is a template class that provides the base functionality for both MPI and libc I/O tracing.
// * The IOtraceMPI and IOtraceLibc classes inherit from IOtraceBase and provide the specific implementation for each type of I/O.
// * The IOtraceTraits class is a template specialization that provides the appropriate RequestType for each tag.
// ! NOTE: Add explicit specialization (in the end of `iotrace_base.cxx`) for each tag to avoid linker errors.
template <typename T>
struct IOtraceTraits;

struct MPI_Tag
{
};
template <>
struct IOtraceTraits<MPI_Tag>
{
	using RequestType = MPI_Request;
	using RequestIDType = MPI_Request *;

	static constexpr const char *Name = "MPI";
};

#if ENABLE_LIBC_TRACE == 1
struct Libc_Tag
{
};
template <>
struct IOtraceTraits<Libc_Tag>
{
	using RequestType = const struct aiocb;
	using RequestIDType = const struct aiocb *;

	static constexpr const char *Name = "Libc";
};
#endif // ENABLE_LIBC_TRACE

#if ENABLE_IOURING_TRACE == 1
struct IOuring_Tag
{
};
template <>
struct IOtraceTraits<IOuring_Tag>
{
	using RequestType = __u64; 
	// Though lib_uring uses void* as user_data in io_uring_sqe_set_data and io_uring_cqe_get_data
	// But for `struct io_uring_sqe`, the type of user_data is defined as __u64, 
	// so we use __u64 here to avoid casting issues.
	using RequestIDType = __u64; 

	static constexpr const char *Name = "IOuring";
};
#endif // ENABLE_IOURING_TRACE

template <typename Tag>
class IOtraceBase
{
protected:
	// Only allow derived classes to instantiate
	IOtraceBase(void);
	IOtraceBase(const IOtraceBase &) = delete;
	void operator=(const IOtraceBase &) = delete;

public:
	using RequestType = typename IOtraceTraits<Tag>::RequestType;
	using RequestIDType = typename IOtraceTraits<Tag>::RequestIDType;

	static constexpr const char *kLibName = IOtraceTraits<Tag>::Name;

	// Mandatory for initialization of IOdata
	void Init(void);
	void Open(void);
	void Summary(void);
	void Close(void);

	int Get_Relevant_Ranks(MPI_File fh);

	//*************************************
	//* Set Functions
	//*************************************
	void Set(std::string, bool);

#if defined BW_LIMIT
	void Apply_Limit(void);
#elif defined CUSTOM_MPI
	void Replace_Test(void);
#endif

protected:
	int rank;			 // current MPI rank
	int processes;		 // number of ranks
	double open;		 // flag indicating file status
	int data_size_read;	 // store size of variable for read
	int data_size_write; // store size of variable for write

	// FIXME: Use function local varaibel to replace the global ones for multiple threads
	double t_async_write_start; // time stamp for start of async write operation
	double t_sync_write_start;	// time stamp for start of sync write operation
	double t_async_read_start;	// time stamp for start of async read operation
	double t_sync_read_start;	// time stamp for start of sync read operation
	double t_sync_read_end;
	double t_sync_write_end;

	// FIXME: Use function local varaibel to replace the global ones for multiple threads
	long long size_async_write; // size of async write operation in KB
	long long size_sync_write;	// size of sync write operation in KB
	long long size_async_read;	// size of async read operation in KB
	long long size_sync_read;	// size of async read operation in KB

	double t_0;						// start time of app (for each rank)
	double delta_t_app = 0;			// elapsed app running time since last IOtrace::Summary calling (for each rank)
	double t_overhead = 0;			// time when IOtrace::Overhead_Start is called, relatived to t_0 (for each rank)
	double delta_t_io_overhead = 0; // elapsed in-period overhead during io tracing since last IOtrace::Summary calling (for each rank)
	double t_summary = 0;			// elapsed time (for each rank) FIXME: Looks should be the MPI_Wtime when last time IOtrace::Summary is finishing its called

	bool online_file_generation = false; // elapsed time (for each rank)
	bool finalize = false;

	// FIXME: Add pthread lock to protect the following variables
	// ques for async tracing
	std::vector<double> async_write_time;
	std::vector<long long> async_write_size;
	std::vector<int> async_write_queue_req;
	std::vector<int> async_write_queue_act;
	std::vector<RequestIDType> async_write_request;
	mutable std::shared_mutex async_write_vecs_lock; // Read-write lock for async write fields

	std::vector<double> async_read_time;
	std::vector<long long> async_read_size;
	std::vector<int> async_read_queue_req;
	std::vector<int> async_read_queue_act;
	std::vector<RequestIDType> async_read_request;
	mutable std::shared_mutex async_read_vecs_lock; // Read-write lock for async read fields

	IOdata aw, ar, sw, sr;
	IOdata *p_aw;
	IOdata *p_ar;
	IOdata *p_sw;
	IOdata *p_sr;

#if defined BW_LIMIT || defined CUSTOM_MPI
	Bw_limit bw_limit;
#endif

	char caller[12] = "\tIOtrace";
	MPI_Comm IO_WORLD;

	//*************************************
	//* Write tracing
	//*************************************
    void Write_Async_Start_Impl(RequestIDType requestID, long long size, long long offset, double start_time);
    void Write_Async_End_Impl(RequestIDType request, int write_status);
    void Write_Async_Required_Impl(RequestIDType request);
    void Write_Sync_Start_Impl(long long size, long long offset, double start_time);
    void Write_Sync_End_Impl();

	//*************************************
	//* Read tracing
	//*************************************
    void Read_Async_Start_Impl(RequestIDType requestID, long long size, long long offset, double start_time);
    void Read_Async_End_Impl(RequestIDType request, int read_status);
    void Read_Async_Required_Impl(RequestIDType request);
    void Read_Sync_Start_Impl(long long size, long long offset, double start_time);
    void Read_Sync_End_Impl();

	//*************************************
	//* Request monitoring
	//*************************************
	bool Check_Request_Write(RequestIDType, double *, long long *, int mode);
	bool Check_Request_Read(RequestIDType, double *, long long *, int mode);
	bool Act_Done(int mode = 0);

	//*************************************
	//* Overhead
	//*************************************
	double Overhead_Start(double);
	void Overhead_End(void);
	double *Overhead_Calculation(void);

	//*************************************
	//* Monitore ellapsed time
	//*************************************
	void Time_Info(std::string);

	//*************************************
	//* Debug
	//*************************************

public:
	template <VerbosityLevel Level>
	inline void Log(const char *format, ...)
	{
		if constexpr (static_cast<int>(IOTRACE_VERBOSITY) >= static_cast<int>(Level))
		{
			va_list args;
			va_start(args, format);
			vprintf(format, args);
			va_end(args);
		}
	}

	template <VerbosityLevel Level, typename Callable>
	inline void LogWithSideEffects(Callable &&sideEffect, const char *format, ...)
	{
		if constexpr (static_cast<int>(IOTRACE_VERBOSITY) >= static_cast<int>(Level))
		{
			sideEffect();
			Log<Level>(format);
		}
	}

	template <VerbosityLevel Level>
	inline void LogWithCondition(bool condition, const char *format, ...)
	{
		if constexpr (static_cast<int>(IOTRACE_VERBOSITY) >= static_cast<int>(Level))
			if (condition)
				Log<Level>(format);
	}

	template <VerbosityLevel Level, typename Callable>
	inline void LogWithAction(Callable &&action)
	{
		if constexpr (static_cast<int>(IOTRACE_VERBOSITY) >= static_cast<int>(Level))
			action();
	}
};

/**
* @class IOtrace
* @brief Construct a new iotime object. Used to trace all I/O calls.
* @note the async_write_vecs_lock and async_read_vecs_lock to protect the async write and read vectors
*       the aw, ar, sw, sr has its own lock

* @details
* \e IOtrace constructor. Initilizes all variables to 0
* \e Init sets the attribute rank to the current MPI rank
*
* write async trace functions
*       \e Write_Async_Start     sets variables at async write I/O call
*       \e Write_Async_End       sets variables at end of async write I/O operation (@ wait or test)
*       \e Write_Async_Required  sets variables of required async write I/O at first call of wait
*
* write sync trace functions
*       \e Write_Sync_Start  sets variables at sync write I/O call
*       \e Write_Sync_End    sets variables at end aof sync write I/O call
*
* read sync trace functions
*       \e Write_Sync_Start  sets variables at sync I/O read  call
*       \e Write_Sync_End    sets variables at end aof sync I/O read  call
*
* \e Get_Relevant_Ranks: extract the ranks accesing a file pointer
* ********************************************************
*/

class IOtraceMPI final : public IOtraceBase<MPI_Tag>
{
public:
	//*************************************
	//* MPI Write tracing
	//*************************************
	void Write_Async_Start(int, MPI_Datatype, MPI_Request *, MPI_Offset offset = 0);
	void Write_Async_End(MPI_Request *, int write_status = 1);
	void Write_Async_Required(MPI_Request *);
	void Write_Sync_Start(int, MPI_Datatype, MPI_Offset offset = 0);
	void Write_Sync_End(void);

	//*************************************
	//* MPI Read tracing
	//*************************************
	void Read_Async_Start(int, MPI_Datatype, MPI_Request *, MPI_Offset offset = 0);
	void Read_Async_End(MPI_Request *request, int read_status = 1);
	void Read_Async_Required(MPI_Request *);
	void Read_Sync_Start(int, MPI_Datatype, MPI_Offset offset = 0);
	void Read_Sync_End(void);
};

#if ENABLE_LIBC_TRACE == 1
class IOtraceLibc final : public IOtraceBase<Libc_Tag>
{
private:
	bool batch_reading = false; // Flag to indicate if batch reading is in progress
	bool batch_writing = false; // Flag to indicate if batch writing is in progress
	std::atomic_bool tracing_enabled_{false}; // When need to trace the I/O operations before main(), checking of this flag would be skipped
public:
	void Enable_Tracing(void)
	{
		tracing_enabled_.store(true, std::memory_order_relaxed);
	}

	//*************************************
	//* Libc Write tracing
	//*************************************
	void Write_Async_Start(const struct aiocb *aiocbp);
	void Write_Async_Start(const struct aiocb64 *aiocbp);
	void Write_Async_End(const struct aiocb *aiocbp, int write_status = 1);
	void Write_Async_End(const struct aiocb64 *aiocbp, int write_status = 1);
	void Write_Async_Required(const struct aiocb *aiocbp);
	void Write_Async_Required(const struct aiocb64 *aiocbp);
	void Write_Sync_Start(size_t count, off64_t offset = 0);
	void Batch_Write_Sync_Start(size_t count, off64_t offset = 0);
	void Write_Sync_End(void);
	void Batch_Write_Sync_End();

	//*************************************
	//* Libc Read tracing
	//*************************************
	void Read_Async_Start(const struct aiocb *aiocbp);
	void Read_Async_Start(const struct aiocb64 *aiocbp);
	void Read_Async_End(const struct aiocb *aiocbp, int read_status = 1);
	void Read_Async_End(const struct aiocb64 *aiocbp, int read_status = 1);
	void Read_Async_Required(const struct aiocb *aiocbp);
	void Read_Async_Required(const struct aiocb64 *aiocbp);
	void Read_Sync_Start(size_t count, off64_t offset = 0);
	void Batch_Read_Sync_Start(size_t count, off64_t offset = 0);
	void Read_Sync_End(void);
	void Batch_Read_Sync_End();
};
#endif // ENABLE_LIBC_TRACE

#if ENABLE_IOURING_TRACE == 1
class IOtraceIOuring final : public IOtraceBase<IOuring_Tag>
{
public:
    IOtraceIOuring();
    ~IOtraceIOuring();

    // To be called from io_uring_queue_init wrapper.
    void Register_Ring(struct io_uring *ring);
    // To be called from io_uring_queue_exit wrapper.
    void Unregister_Ring(struct io_uring *ring);

    // To be called from io_uring_submit wrapper.
    void Process_Submissions(struct io_uring *ring);

    // To be called from io_uring_wait_cqe* wrappers for more precise reaping.
    void Process_Completions(struct io_uring *ring);

    // To be called from io_uring_wait_cqe* wrappers to mark all pending requests as required.
    void Mark_All_Pending_As_Required(struct io_uring *ring);

private:
    // --- Core Data Structures ---

    // The item our SPSC queue will hold. It must contain all necessary info.
    struct StagedRequest {
        struct io_uring* ring;
        __u64 user_data;
        long long total_size;
        off_t offset;
        bool is_write;
		double submission_time;
    };

    // Cache-friendly state for each tracked io_uring instance.
    struct RingTraceState {
        struct io_uring* ring_ptr;
        unsigned last_known_sq_tail;
        unsigned last_known_cq_head;

        // Sets to track in-flight requests submitted to the kernel.
        std::unordered_set<__u64> pending_writes;
        std::unordered_set<__u64> pending_reads;

        RingTraceState(struct io_uring* ring)
            : ring_ptr(ring), last_known_sq_tail(0), last_known_cq_head(0) {}
        
        size_t pending_request_count() const {
            return pending_writes.size() + pending_reads.size();
        }
    };

    // A cache-friendly SPSC (Single-Producer, Single-Consumer) lock-free queue.
    template <typename T>
    class SPSCQueue {
    public:
        explicit SPSCQueue(size_t size) : size_(size), mask_(size - 1), buffer_(size) {
            assert((size != 0) && ((size & (size - 1)) == 0) && "Size must be a power of two.");
            head_.store(0, std::memory_order_relaxed);
            tail_.store(0, std::memory_order_relaxed);
        }

        bool push(const T& value) {
            const auto current_tail = tail_.load(std::memory_order_relaxed);
            if (current_tail - head_.load(std::memory_order_acquire) >= size_) {
                return false; // Full
            }
            buffer_[current_tail & mask_] = value;
            tail_.store(current_tail + 1, std::memory_order_release);
            return true;
        }

        bool pop(T& value) {
            const auto current_head = head_.load(std::memory_order_relaxed);
            if (current_head == tail_.load(std::memory_order_acquire)) {
                return false; // Empty
            }
            value = buffer_[current_head & mask_];
            head_.store(current_head + 1, std::memory_order_release);
            return true;
        }

        bool is_empty() const {
            return head_.load(std::memory_order_acquire) == tail_.load(std::memory_order_acquire);
        }

    private:
		alignas(64) std::atomic<size_t> head_;
		alignas(64) std::atomic<size_t> tail_;
        const size_t size_;
        const size_t mask_;
        std::vector<T> buffer_;
    };

    // --- Private Member Variables ---

	SPSCQueue<StagedRequest> staged_requests_queue_; // The handoff queue
    std::vector<RingTraceState> tracked_rings_;
    
    // Synchronization primitives
    std::mutex ring_processing_lock_;
    std::condition_variable polling_cond_var_;
    
    // Polling thread management
    std::thread polling_thread_;
    std::atomic<bool> keep_polling_;

    // --- Private Helper Functions ---

    // Finds the state for a given ring, returns nullptr if not found.
    RingTraceState* find_ring_state(struct io_uring* ring);

    // Core processing logic, protected by the mutex.
    void process_rings_locked();

	 // Helper to process the SPSC queue
	void drain_staged_requests_locked();
    
    // Modularized helpers for processing SQ and CQ rings.
    void scan_and_stage_new_requests_locked(RingTraceState& state);
    void reap_completions_locked(RingTraceState& state);

    // The polling thread's main loop.
    void polling_loop();

    // Old async start/end functions, now with Required.
    void Write_Async_Start(RequestIDType requestID, long long size, long long offset, double start_time);
    void Write_Async_End(RequestIDType requestID, int status);
    void Write_Async_Required(RequestIDType requestID);
    void Read_Async_Start(RequestIDType requestID, long long size, long long offset, double start_time);
    void Read_Async_End(RequestIDType requestID, int status);
    void Read_Async_Required(RequestIDType requestID);
};
#endif // ENABLE_IOURING_TRACE

#endif // IOTRACE_H