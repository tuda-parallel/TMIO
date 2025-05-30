#ifndef IOTRACE_H
#define IOTRACE_H

#include "tmio_helper_functions.h"
#include <shared_mutex> // For std::shared_mutex
#include <mutex>		// For std::mutex

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

	static constexpr const char *Name = "MPI";
};

struct Libc_Tag
{
};
template <>
struct IOtraceTraits<Libc_Tag>
{
	using RequestType = struct aiocb;

	static constexpr const char *Name = "Libc";
};

template <typename Tag>
class IOtraceBase
{
public:
	using RequestType = typename IOtraceTraits<Tag>::RequestType;
	using RequestPtr = RequestType *;

	static constexpr const char *kLibName = IOtraceTraits<Tag>::Name;

	IOtraceBase();
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
	std::vector<RequestPtr> async_write_request;
	mutable std::shared_mutex async_write_vecs_lock; // Read-write lock for async write fields

	std::vector<double> async_read_time;
	std::vector<long long> async_read_size;
	std::vector<int> async_read_queue_req;
	std::vector<int> async_read_queue_act;
	std::vector<RequestPtr> async_read_request;
	mutable std::shared_mutex async_read_vecs_lock; // Read-write lock for async read fields

	IOdata aw, ar, sw, sr;
	IOdata *p_aw = &aw;
	IOdata *p_ar = &ar;
	IOdata *p_sw = &sw;
	IOdata *p_sr = &sr;

#if defined BW_LIMIT || defined CUSTOM_MPI
	Bw_limit bw_limit;
#endif

	char caller[12] = "\tIOtrace";
	MPI_Comm IO_WORLD;

	//*************************************
	//* Request monitoring
	//*************************************
	bool Check_Request_Write(RequestPtr, double *, long long *, int mode);
	bool Check_Request_Read(RequestPtr, double *, long long *, int mode);
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

class IOtraceMPI : public IOtraceBase<MPI_Tag>
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

class IOtraceLibc : public IOtraceBase<Libc_Tag>
{
public:
	//*************************************
	//* TODO: Libc Write tracing
	//*************************************
	void Write_Sync_Start(size_t count, off_t offset = 0);
	void Write_Sync_End(void);

	//*************************************
	//* TODO: Libc Read tracing
	//*************************************
	void Read_Sync_Start(size_t count, off_t offset = 0);
	void Read_Sync_End(void);
};

#endif // IOTRACE_H