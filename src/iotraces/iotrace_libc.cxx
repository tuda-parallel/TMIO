#include "iotrace.h"
#include <aio.h>

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
    async_write_time.push_back(Overhead_Start(MPI_Wtime() - t_0));

    // Determine write size from aiocb structure
    async_write_size.push_back(aiocbp->aio_nbytes);

    // Phase start if first request. Add phase data and offset
    p_aw->Phase_Start(async_write_request.empty(), async_write_time.back(), async_write_size.back(), aiocbp->aio_offset);

    // Save request flag and set request counter (required and actual to one)
    async_write_request.push_back(aiocbp);
    async_write_queue_req.push_back(1);
    async_write_queue_act.push_back(1);

    IOtraceLibc::LogWithAction<VerbosityLevel::BASIC_LOG>([&]()
                                                          {
                                                              static long int counter = 1;
                                                              IOtraceLibc::Log<VerbosityLevel::BASIC_LOG>(
                                                                  "%s > rank %i > #%li will async write %zu bytes\n",
                                                                  caller, rank, counter++, async_write_size.back()); });
    IOtraceLibc::Log<VerbosityLevel::DETAILED_LOG>(
        "%s > rank %i %s>> started async write @ %f s %s\n", caller, rank, GREEN, async_write_time.back(), BLACK);
    IOtraceLibc::Log<VerbosityLevel::DEBUG_LOG>(
        "%s > rank %i %s>>> has offset %lli %s\n", caller, rank, YELLOW, aiocbp->aio_offset, BLACK);
}

void IOtraceLibc::Write_Async_Start(const struct aiocb64 *aiocbp) {
    async_write_time.push_back(Overhead_Start(MPI_Wtime() - t_0));

    // Determine write size from aiocb structure
    async_write_size.push_back(aiocbp->aio_nbytes);

    // Phase start if first request. Add phase data and offset
    p_aw->Phase_Start(async_write_request.empty(), async_write_time.back(), async_write_size.back(), aiocbp->aio_offset);

    // Save request flag and set request counter (required and actual to one)
    async_write_request.push_back(reinterpret_cast<const struct aiocb *>(aiocbp)); // Cast aiocb64 to aiocb
    // Note: aiocb64 is a 64-bit version of aiocb, so we cast it to aiocb for compatibility
    // This is safe as long as the aiocb64 structure is compatible with aiocb in terms of fields used here
    async_write_queue_req.push_back(1);
    async_write_queue_act.push_back(1);

    IOtraceLibc::LogWithAction<VerbosityLevel::BASIC_LOG>([&]()
                                                          {
                                                              static long int counter = 1;
                                                              IOtraceLibc::Log<VerbosityLevel::BASIC_LOG>(
                                                                  "%s > rank %i > #%li will async write %zu bytes\n",
                                                                  caller, rank, counter++, async_write_size.back()); });
    IOtraceLibc::Log<VerbosityLevel::DETAILED_LOG>(
        "%s > rank %i %s>> started async write @ %f s %s\n", caller, rank, GREEN, async_write_time.back(), BLACK);
    IOtraceLibc::Log<VerbosityLevel::DEBUG_LOG>(
        "%s > rank %i %s>>> has offset %lli %s\n", caller, rank, YELLOW, aiocbp->aio_offset, BLACK);
    
    Overhead_End();
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
    Overhead_Start(MPI_Wtime() - t_0);

    // Actual write ended signilized by flag of MPI_Test or at the end of MPI_Wait. This flag will always be true if the I/O operation ended
    if (write_status == 1)
    {
        if (Check_Request_Write(aiocbp, &t_async_write_start, &size_async_write, 2))
        {
            p_aw->Phase_End_Act(size_async_write, t_async_write_start, MPI_Wtime() - t_0, Act_Done(0));

            IOtraceLibc::Log<VerbosityLevel::DETAILED_LOG>(
                "%s > rank %i %s>> active write async requests %li %s\n", caller, rank, GREEN, async_write_request.size(), BLACK);
            IOtraceLibc::Log<VerbosityLevel::DEBUG_LOG>(
                "%s > rank %i %s>> write async requests ended at %f %s\n", caller, rank, RED, p_aw->phase_data.back().t_end_act, BLACK);
        }
    }
    IOtraceLibc::LogWithCondition<VerbosityLevel::DEBUG_LOG>(
        write_status == 0,
        "%s > rank %i %s>>>> testing for async write completion %s\n", caller, rank, RED, BLACK);
    Overhead_End();
}

void IOtraceLibc::Write_Async_End(const struct aiocb64 *aiocbp, int write_status)
{
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
void IOtraceLibc::Write_Async_Required(const struct aiocb *aiocbp)
{
    Overhead_Start(MPI_Wtime() - t_0);

    if (Check_Request_Write(aiocbp, &t_async_write_start, &size_async_write, 1))
    {
        p_aw->Phase_End_Req(size_async_write, t_async_write_start, MPI_Wtime() - t_0);

        IOtraceLibc::Log<VerbosityLevel::DETAILED_LOG>(
            "%s > rank %i %s>> active write async requests %li%s\n", caller, rank, GREEN, async_write_request.size(), BLACK);
    }
    Overhead_End();
}

void IOtraceLibc::Write_Async_Required(const struct aiocb64 *aiocbp)
{
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
void IOtraceLibc::Read_Async_Start(const struct aiocb *aiocbp)
{
    async_read_time.push_back(Overhead_Start(MPI_Wtime() - t_0));

    // Determine read size from aiocb structure
    async_read_size.push_back(aiocbp->aio_nbytes);

    // Phase start if first request. Add phase data and offset
    p_ar->Phase_Start(async_read_request.empty(), async_read_time.back(), async_read_size.back(), aiocbp->aio_offset);

    // Save request flag and set request counter (required and actual to one)
    async_read_request.push_back(aiocbp);
    async_read_queue_req.push_back(1);
    async_read_queue_act.push_back(1);

    IOtraceLibc::LogWithAction<VerbosityLevel::BASIC_LOG>([&]()
                                                          {
                                                              static long int counter = 1;
                                                              IOtraceLibc::Log<VerbosityLevel::BASIC_LOG>(
                                                                  "%s > rank %i > #%li will async read %zu bytes\n",
                                                                  caller, rank, counter++, async_read_size.back()); });
    IOtraceLibc::Log<VerbosityLevel::DETAILED_LOG>(
        "%s > rank %i %s>> started async read @ %f s %s\n", caller, rank, GREEN, async_read_time.back(), BLACK);
    IOtraceLibc::Log<VerbosityLevel::DEBUG_LOG>(
        "%s > rank %i %s>>> has offset %lli %s\n", caller, rank, YELLOW, aiocbp->aio_offset, BLACK);
    Overhead_End();
}

void IOtraceLibc::Read_Async_Start(const struct aiocb64 *aiocbp) {
    async_read_time.push_back(Overhead_Start(MPI_Wtime() - t_0));

    // Determine read size from aiocb structure
    async_read_size.push_back(aiocbp->aio_nbytes);

    // Phase start if first request. Add phase data and offset
    p_ar->Phase_Start(async_read_request.empty(), async_read_time.back(), async_read_size.back(), aiocbp->aio_offset);

    // Save request flag and set request counter (required and actual to one)
    async_read_request.push_back(reinterpret_cast<const struct aiocb *>(aiocbp)); // Cast aiocb64 to aiocb
    // Note: aiocb64 is a 64-bit version of aiocb, so we cast it to aiocb for compatibility
    // This is safe as long as the aiocb64 structure is compatible with aiocb in terms of fields used here
    async_read_queue_req.push_back(1);
    async_read_queue_act.push_back(1);

    IOtraceLibc::LogWithAction<VerbosityLevel::BASIC_LOG>([&]()
                                                          {
                                                              static long int counter = 1;
                                                              IOtraceLibc::Log<VerbosityLevel::BASIC_LOG>(
                                                                  "%s > rank %i > #%li will async read %zu bytes\n",
                                                                  caller, rank, counter++, async_read_size.back()); });
    IOtraceLibc::Log<VerbosityLevel::DETAILED_LOG>(
        "%s > rank %i %s>> started async read @ %f s %s\n", caller, rank, GREEN, async_read_time.back(), BLACK);
    IOtraceLibc::Log<VerbosityLevel::DEBUG_LOG>(
        "%s > rank %i %s>>> has offset %lli %s\n", caller, rank, YELLOW, aiocbp->aio_offset, BLACK);
    
    Overhead_End();
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
    Overhead_Start(MPI_Wtime() - t_0);

    if (read_status == 1)
    {
        if (Check_Request_Read(aiocbp, &t_async_read_start, &size_async_read, 2))
        {
            p_ar->Phase_End_Act(size_async_read, t_async_read_start, MPI_Wtime() - t_0, Act_Done(0));

            IOtraceLibc::LogWithAction<VerbosityLevel::DETAILED_LOG>([&]()
                                                                     {
                IOtraceLibc::Log<VerbosityLevel::DETAILED_LOG>(
                    "%s > rank %i %s>> active read async requests %li %s\n", caller, rank, GREEN, async_read_request.size(), BLACK);
                    iohf::Disp(async_read_queue_req, async_read_queue_req.size(), std::string(caller) + " > rank " + std::to_string(rank) + " >> async_read_queue_req ", 1); });

            IOtraceLibc::Log<VerbosityLevel::DEBUG_LOG>(
                "%s > rank %i %s>> read async requests ended at %f %s\n", caller, rank, RED, p_ar->phase_data.back().t_end_act, BLACK);
        }
    }
    IOtraceLibc::LogWithCondition<VerbosityLevel::DEBUG_LOG>(
        read_status == 0,
        "%s > rank %i %s>>>> testing for async read completion %s\n", caller, rank, RED, BLACK);
    Overhead_End();
}
void IOtraceLibc::Read_Async_End(const struct aiocb64 *aiocbp, int read_status)
{
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
void IOtraceLibc::Read_Async_Required(const struct aiocb *aiocbp)
{
    Overhead_Start(MPI_Wtime() - t_0);

    if (Check_Request_Read(aiocbp, &t_async_read_start, &size_async_read, 1))
    {
        p_ar->Phase_End_Req(size_async_read, t_async_read_start, MPI_Wtime() - t_0);

        IOtraceLibc::Log<VerbosityLevel::DETAILED_LOG>(
            "%s > rank %i %s>> active read async requests %li%s\n", caller, rank, GREEN, async_read_request.size(), BLACK);
    }
    Overhead_End();
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
