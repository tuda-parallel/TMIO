#include "iotrace.h"

//! ------------------------------ Async write tracing -------------------------------
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
    // TODO: Implement the async write start tracing for libc
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
void IOtraceLibc::Write_Async_End(const struct aiocb *aiocbp, int write_status)
{
    // TODO: Implement the async write end tracing for libc
}

/**
 * @brief This function complements the functionality of @ref IOtraceMPI::Write_Async_Required.
 *        But would read the aiocb structure to get the I/O operation details.
 *
 * @param aiocbp  [in] const pointer to the aiocb structure containing the I/O operation details
 *
 * @see IOtraceMPI::Write_Async_Required
 */
void IOtraceLibc::Write_Async_Required(const struct aiocb *aiocbp)
{
    // TODO: Implement the async write required tracing for libc
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
void IOtraceLibc::Read_Async_Start(const struct aiocb *aiocbp)
{
    // TODO: Implement the async read start tracing for libc
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
void IOtraceLibc::Read_Async_End(const struct aiocb *aiocbp, int read_status)
{
    // TODO: Implement the async read end tracing for libc
}
/**
 * @brief This function complements the functionality of @ref IOtraceMPI::Read_Async_Required.
 *        But would read the aiocb structure to get the I/O operation details.
 *
 * @param aiocbp  [in] const pointer to the aiocb structure containing the I/O operation details
 *
 * @see IOtraceMPI::Read_Async_Required
 */
void IOtraceLibc::Read_Async_Required(const struct aiocb *aiocbp)
{
    // TODO: Implement the async read required tracing for libc
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
void IOtraceLibc::Write_Sync_Start(size_t count, off64_t offset)
{
    t_sync_write_start = Overhead_Start(MPI_Wtime() - t_0);
    size_sync_write = count * 1; // in B

#if SYNC_MODE == 1
    p_sw->Phase_Start(p_sw->flag && !(p_sw->phase), t_sync_write_start, size_sync_write, offset);
#else
    p_sw->Phase_Start(true, t_sync_write_start, size_sync_write, static_cast<long long>(offset));
#endif

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
void IOtraceLibc::Write_Sync_End(void)
{
    t_sync_write_end = Overhead_Start(MPI_Wtime() - t_0);

    p_sw->Add_Io(0, size_sync_write, t_sync_write_start, t_sync_write_end);
#if SYNC_MODE == 0
    p_sw->Phase_End_Sync(t_sync_write_end);
    IOtraceLibc::Log<VerbosityLevel::DETAILED_LOG>(
        "%s > rank %i %s>> ended   sync write @ %f s %s\n", caller, rank, GREEN, t_sync_write_end, BLACK);
#endif

    Overhead_End();
}
//! ------------------------------ Sync read tracing -------------------------------
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
    t_sync_read_start = Overhead_Start(MPI_Wtime() - t_0);
    size_sync_read = count * 1; // in B

#if SYNC_MODE == 1
    p_sr->Phase_Start(p_sr->flag && !(p_sr->phase), t_sync_read_start, size_sync_read, offset);
#else
    p_sr->Phase_Start(true, t_sync_read_start, size_sync_read, static_cast<long long>(offset));
#endif

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
    t_sync_read_end = Overhead_Start(MPI_Wtime() - t_0);

    p_sr->Add_Io(0, size_sync_read, t_sync_read_start, t_sync_read_end);

#if SYNC_MODE == 0
    p_sr->Phase_End_Sync(t_sync_read_end);
    IOtraceLibc::Log<VerbosityLevel::DETAILED_LOG>(
        "%s > rank %i %s>> ended   sync read @ %f s %s\n", caller, rank, GREEN, t_sync_read_end, BLACK);
#endif

    Overhead_End();
}
