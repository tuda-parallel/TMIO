#include "iotrace.h"

#if ENABLE_LIBC_TRACE == 1
#include <aio.h>
#include <atomic>

#if IO_BEFORE_MAIN == 0
/**
 * @def BEFORE_MAIN_GUARD_FUNCTION
 * @brief If IO_BEFORE_MAIN is defined, this macro checks the atomic
 *        `tracing_enabled_` flag and returns from the function if it's false.
 *        Otherwise, it expands to nothing, incurring zero runtime cost.
 *
 *        Also use do while to prevent dangling else issues.
 */
#define BEFORE_MAIN_GUARD_FUNCTION()                           \
    do                                                         \
    {                                                          \
        if (!tracing_enabled_.load(std::memory_order_relaxed)) \
        {                                                      \
            return;                                            \
        }                                                      \
    } while (0)
#else
#define BEFORE_MAIN_GUARD_FUNCTION()
#endif
//! ------------------------------ Async write tracing -------------------------------
//************************************************************************************
//*                               1. Write_Async_Start
//************************************************************************************
/**
 * @brief This function complements the functionality of @ref IOtraceMPI::Write_Async_Start.
 *        But would read the aiocb structure to get the I/O operation details.
 *
 * @param aiocbp  [in] const pointer to the aiocb structure containing the I/O operation details
 *
 * @see IOtraceMPI::Write_Async_Start
 */
void IOtraceLibc::Write_Async_Start(const struct aiocb *aiocbp)
{
    BEFORE_MAIN_GUARD_FUNCTION();

    double start_time = MPI_Wtime() - t_0;
    long long total_size = aiocbp->aio_nbytes;

    IOtraceLibc::LogWithAction<VerbosityLevel::BASIC_LOG>([&]()
                                                          {
        static long int counter = 1;
        IOtraceLibc::Log<VerbosityLevel::BASIC_LOG>(
            "%s > rank %i > #%li will asnyc write %i x %i = %lli bytes \n",
                caller, rank, counter++, 1, total_size, total_size); });

    Write_Async_Start_Impl(aiocbp, total_size, aiocbp->aio_offset, start_time);
}

void IOtraceLibc::Write_Async_Start(const struct aiocb64 *aiocbp)
{
    BEFORE_MAIN_GUARD_FUNCTION();

    Write_Async_Start(reinterpret_cast<const struct aiocb *>(aiocbp)); // Cast aiocb64 to aiocb
}

/**
 * @brief This function complements the functionality of @ref IOtraceMPI::Write_Async_End.
 *        But would read the aiocb structure to get the I/O operation details.
 *
 * @param aiocbp  [in] const pointer to the aiocb structure containing the I/O operation details
 * @param write_status [in,optional] status of the write operation, default is 1 (indicating success)
 *
 * @see IOtraceMPI::Write_Async_End
 */
//************************************************************************************
//*                               2. Write_Async_End
//************************************************************************************
void IOtraceLibc::Write_Async_End(const struct aiocb *aiocbp, int write_status)
{
    BEFORE_MAIN_GUARD_FUNCTION();

    Write_Async_End_Impl(aiocbp, write_status);
}

void IOtraceLibc::Write_Async_End(const struct aiocb64 *aiocbp, int write_status)
{
    BEFORE_MAIN_GUARD_FUNCTION();

    Write_Async_End(reinterpret_cast<const struct aiocb *>(aiocbp), write_status); // Cast aiocb64 to aiocb
}

/**
 * @brief This function complements the functionality of @ref IOtraceMPI::Write_Async_Required.
 *        But would read the aiocb structure to get the I/O operation details.
 *
 * @param aiocbp  [in] const pointer to the aiocb structure containing the I/O operation details
 *
 * @see IOtraceMPI::Write_Async_Required
 */
//************************************************************************************
//*                               3. Write_Async_Required
//************************************************************************************
void IOtraceLibc::Write_Async_Required(const struct aiocb *aiocbp)
{
    BEFORE_MAIN_GUARD_FUNCTION();

    Write_Async_Required_Impl(aiocbp);
}

void IOtraceLibc::Write_Async_Required(const struct aiocb64 *aiocbp)
{
    BEFORE_MAIN_GUARD_FUNCTION();

    Write_Async_Required(reinterpret_cast<const struct aiocb *>(aiocbp)); // Cast aiocb64 to aiocb
}
//! ------------------------------ Async read tracing -------------------------------
/**
 * @brief This function complements the functionality of @ref IOtraceMPI::Read_Async_Start.
 *        But would read the aiocb structure to get the I/O operation details.
 *
 * @param aiocbp  [in] const pointer to the aiocb structure containing the I/O operation details
 *
 * @see IOtraceMPI::Read_Async_Start
 */
//************************************************************************************
//*                               1. Read_Async_Start
//************************************************************************************
void IOtraceLibc::Read_Async_Start(const struct aiocb *aiocbp)
{
    BEFORE_MAIN_GUARD_FUNCTION();

    double start_time = MPI_Wtime() - t_0;
    data_size_read = 1; // in B
    long long total_size = aiocbp->aio_nbytes;

    IOtraceLibc::LogWithAction<VerbosityLevel::BASIC_LOG>([&]()
                                                          {
        static long int counter = 1;
        IOtraceLibc::Log<VerbosityLevel::BASIC_LOG>(
            "%s > rank %i > #%li will async read %lli bytes\n",
            caller, rank, counter++, total_size); });

    Read_Async_Start_Impl(aiocbp, total_size, aiocbp->aio_offset, start_time);
}

void IOtraceLibc::Read_Async_Start(const struct aiocb64 *aiocbp)
{
    BEFORE_MAIN_GUARD_FUNCTION();

    Read_Async_Start(reinterpret_cast<const struct aiocb *>(aiocbp)); // Cast aiocb64 to aiocb
}

/**
 * @brief This function complements the functionality of @ref IOtraceMPI::Read_Async_End.
 *        But would read the aiocb structure to get the I/O operation details.
 *
 * @param aiocbp  [in] const pointer to the aiocb structure containing the I/O operation details
 * @param read_status [in,optional] status of the read operation, default is 1 (indicating success)
 *
 * @see IOtraceMPI::Read_Async_End
 */
//************************************************************************************
//*                               2. Read_Async_End
//************************************************************************************
void IOtraceLibc::Read_Async_End(const struct aiocb *aiocbp, int read_status)
{
    BEFORE_MAIN_GUARD_FUNCTION();

    Read_Async_End_Impl(aiocbp, read_status);
}

void IOtraceLibc::Read_Async_End(const struct aiocb64 *aiocbp, int read_status)
{
    BEFORE_MAIN_GUARD_FUNCTION();

    Read_Async_End(reinterpret_cast<const struct aiocb *>(aiocbp), read_status); // Cast aiocb64 to aiocb
}

/**
 * @brief This function complements the functionality of @ref IOtraceMPI::Read_Async_Required.
 *        But would read the aiocb structure to get the I/O operation details.
 *
 * @param aiocbp  [in] const pointer to the aiocb structure containing the I/O operation details
 *
 * @see IOtraceMPI::Read_Async_Required
 */
//************************************************************************************
//*                               3. Read_Async_Required
//************************************************************************************
void IOtraceLibc::Read_Async_Required(const struct aiocb *aiocbp)
{
    BEFORE_MAIN_GUARD_FUNCTION();

    Read_Async_Required_Impl(aiocbp);
}
void IOtraceLibc::Read_Async_Required(const struct aiocb64 *aiocbp)
{
    Read_Async_Required(reinterpret_cast<const struct aiocb *>(aiocbp)); // Cast aiocb64 to aiocb
}

//! ------------------------------ Sync write tracing -------------------------------

/**
 * @brief This function complements the functionality of @ref IOtraceMPI::Write_Sync_Start.
 *
 * @param count    [in] *bytes* count of write operations
 * @param offset   [in,optional] offset of the I/O operation, default is 0
 *
 * @see IOtraceMPI::Write_Sync_Start
 */
//************************************************************************************
//*                               1. Write_Sync_Start
//************************************************************************************
void IOtraceLibc::Write_Sync_Start(size_t count, off64_t offset)
{
    BEFORE_MAIN_GUARD_FUNCTION();

    // get write timestamp
    double start_time = MPI_Wtime() - t_0;
    t_sync_write_start = Overhead_Start(start_time);

    data_size_write = 1; // in B
    size_sync_write = count * 1; // in B
    Write_Sync_Start_Impl(size_sync_write, offset, start_time);
}

/**
 * @brief What "batch" means is not the IO operation per se is executed in one shot, but that multiple
 *       write operations are grouped together and executed in a single batch **one by one**.
 *       So this function would be call multiple times to trace a batch of write operations.
 *
 * @param count    [in] *bytes* count of write operations
 * @param offset   [in,optional] offset of the I/O operation, default is 0
 *
 * @see IOtraceMPI::Batch_Write_Sync_Start
 */
void IOtraceLibc::Batch_Write_Sync_Start(size_t count, off64_t offset)
{
    BEFORE_MAIN_GUARD_FUNCTION();

    t_sync_write_start = Overhead_Start(MPI_Wtime() - t_0);
    size_sync_write = count * 1; // in B

    // Phase start if first request. Add phase data and offset
    p_sw->Phase_Start(!batch_writing, t_sync_write_start, size_sync_write, offset);

    batch_writing = true;

    // Logging
    IOtraceLibc::Log<VerbosityLevel::BASIC_LOG>(
        "%s > rank %i > will write %zu bytes \n", caller, rank, count);
    IOtraceLibc::Log<VerbosityLevel::DETAILED_LOG>(
        "%s > rank %i %s>> started sync write @ %.2f s %s\n", caller, rank, GREEN, t_sync_write_start, BLACK);
    IOtraceLibc::Log<VerbosityLevel::DEBUG_LOG>(
        "%s > rank %i %s>>> has offset %lli %s\n", caller, rank, YELLOW, offset, BLACK);
    Overhead_End();
}

/**
 * @brief This function complements the functionality of @ref IOtraceMPI::Write_Sync_End.
 *
 * @see IOtraceMPI::Write_Sync_End
 */
//************************************************************************************
//*                               2. Write_Sync_End
//************************************************************************************
void IOtraceLibc::Write_Sync_End(void)
{
    BEFORE_MAIN_GUARD_FUNCTION();

    Write_Sync_End_Impl();
}

void IOtraceLibc::Batch_Write_Sync_End()
{
    BEFORE_MAIN_GUARD_FUNCTION();

    t_sync_write_end = Overhead_Start(MPI_Wtime() - t_0);

    if (!batch_writing)
    {
        IOtraceLibc::Log<VerbosityLevel::BASIC_LOG>(
            "%s > rank %i > No batch writing in progress, cannot end sync write.\n", caller, rank);
        return;
    }

    p_sw->Add_Io(0, size_sync_write, t_sync_write_start, t_sync_write_end);

#if SYNC_MODE == 0
    p_sw->Phase_End_Sync(t_sync_write_end);
    IOtraceLibc::Log<VerbosityLevel::DETAILED_LOG>(
        "%s > rank %i %s>> ended   sync write @ %f s %s\n", caller, rank, GREEN, t_sync_write_end, BLACK);
#endif

    batch_writing = false;

    IOtraceLibc::Log<VerbosityLevel::BASIC_LOG>(
        "%s > rank %i > Batch sync write ended successfully.\n", caller, rank);
    IOtraceLibc::Log<VerbosityLevel::DEBUG_LOG>(
        "%s > rank %i %s>>> has offset %lli %s\n", caller, rank, YELLOW, p_sw->phase_data.back().t_end_act, BLACK);
    IOtraceLibc::Log<VerbosityLevel::DEBUG_LOG>(
        "%s > rank %i %s>>> batch writing ended at %f %s\n", caller, rank, RED, p_sw->phase_data.back().t_end_act, BLACK);
    IOtraceLibc::Log<VerbosityLevel::DEBUG_LOG>(
        "%s > rank %i %s>>> batch writing size %lli %s\n", caller, rank, YELLOW, size_sync_write, BLACK);
    IOtraceLibc::Log<VerbosityLevel::DEBUG_LOG>(
        "%s > rank %i %s>>> batch writing start time %f %s\n", caller, rank, YELLOW, t_sync_write_start, BLACK);
    IOtraceLibc::Log<VerbosityLevel::DEBUG_LOG>(
        "%s > rank %i %s>>> batch writing end time %f %s\n", caller, rank, YELLOW, t_sync_write_end, BLACK);
    Overhead_End();
}
//! ------------------------------ Sync read tracing -------------------------------
//************************************************************************************
//*                               1. Read_Sync_Start
//************************************************************************************
/**
 * @brief This function complements the functionality of @ref IOtraceMPI::Read_Sync_Start.
 *
 * @param count    [in] *bytes* count of read operations
 * @param offset   [in,optional] offset of the I/O operation, default is 0
 *
 * @see IOtraceMPI::Read_Sync_Start
 */
void IOtraceLibc::Read_Sync_Start(size_t count, off64_t offset)
{
    BEFORE_MAIN_GUARD_FUNCTION();

    double start_time = MPI_Wtime() - t_0;
    t_sync_read_start = Overhead_Start(start_time);

    data_size_read = 1; // in B
    size_sync_read = count * 1; // in B
    Read_Sync_Start_Impl(size_sync_read, offset, start_time);
}

/**
 * @brief What "batch" means is not the IO operation per se is executed in one shot, but that multiple
 *        read operations are grouped together and executed in a single batch **one by one**.
 *        So this function would be call multiple times to trace a batch of read operations.
 *
 * @param count    [in] *bytes* count of read operations
 * @param offset   [in,optional] offset of the I/O operation, default is 0
 *
 * @see IOtraceMPI::Batch_Read_Sync_Start
 */
//************************************************************************************
//*                               2. Read_Sync_End
//************************************************************************************
void IOtraceLibc::Batch_Read_Sync_Start(size_t count, off64_t offset)
{
    BEFORE_MAIN_GUARD_FUNCTION();

    t_sync_read_start = Overhead_Start(MPI_Wtime() - t_0);

    data_size_read = 1; // in B
    size_sync_read = count * 1; // in B

    // Phase start if first request. Add phase data and offset
    p_sr->Phase_Start(!batch_reading, t_sync_read_start, size_sync_read, offset);

    batch_reading = true;
    // Logging
    IOtraceLibc::Log<VerbosityLevel::BASIC_LOG>(
        "%s > rank %i > will read %zu bytes \n", caller, rank, count);
    IOtraceLibc::Log<VerbosityLevel::DETAILED_LOG>(
        "%s > rank %i %s>> started sync read @ %.2f s %s\n", caller, rank, GREEN, t_sync_read_start, BLACK);
    IOtraceLibc::Log<VerbosityLevel::DEBUG_LOG>(
        "%s > rank %i %s>>> has offset %lli %s\n", caller, rank, YELLOW, offset, BLACK);

    Overhead_End();
}

/**
 * @see IOtraceMPI::Read_Sync_End
 */
void IOtraceLibc::Read_Sync_End(void)
{
    BEFORE_MAIN_GUARD_FUNCTION();

    Read_Sync_End_Impl();
}

void IOtraceLibc::Batch_Read_Sync_End()
{
    BEFORE_MAIN_GUARD_FUNCTION();

    t_sync_read_end = Overhead_Start(MPI_Wtime() - t_0);

    if (!batch_reading)
    {
        IOtraceLibc::Log<VerbosityLevel::BASIC_LOG>(
            "%s > rank %i > No batch reading in progress, cannot end sync read.\n", caller, rank);
        return;
    }

    p_sr->Add_Io(0, size_sync_read, t_sync_read_start, t_sync_read_end);

#if SYNC_MODE == 0
    p_sr->Phase_End_Sync(t_sync_read_end);
    IOtraceLibc::Log<VerbosityLevel::DETAILED_LOG>(
        "%s > rank %i %s>> ended   sync read @ %f s %s\n", caller, rank, GREEN, t_sync_read_end, BLACK);
#endif

    batch_reading = false;

    IOtraceLibc::Log<VerbosityLevel::BASIC_LOG>(
        "%s > rank %i > Batch sync read ended successfully.\n", caller, rank);
    IOtraceLibc::Log<VerbosityLevel::DEBUG_LOG>(
        "%s > rank %i %s>>> has offset %lli %s\n", caller, rank, YELLOW, p_sr->phase_data.back().t_end_act, BLACK);
    IOtraceLibc::Log<VerbosityLevel::DEBUG_LOG>(
        "%s > rank %i %s>>> batch reading ended at %f %s\n", caller, rank, RED, p_sr->phase_data.back().t_end_act, BLACK);
    IOtraceLibc::Log<VerbosityLevel::DEBUG_LOG>(
        "%s > rank %i %s>>> batch reading size %lli %s\n", caller, rank, YELLOW, size_sync_read, BLACK);
    IOtraceLibc::Log<VerbosityLevel::DEBUG_LOG>(
        "%s > rank %i %s>>> batch reading start time %f %s\n", caller, rank, YELLOW, t_sync_read_start, BLACK);
    IOtraceLibc::Log<VerbosityLevel::DEBUG_LOG>(
        "%s > rank %i %s>>> batch reading end time %f %s\n", caller, rank, YELLOW, t_sync_read_end, BLACK);
    Overhead_End();
}
#endif // ENABLE_LIBC_TRACE