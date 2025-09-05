#include "iotrace.h"
#include <liburing.h>

//! ------------------------------ Async write tracing -------------------------------
void IOtraceIOuring::Write_Async_Start(const struct io_uring_sqe *sqe) 
{
    // get write timestamp
    async_write_time.push_back(Overhead_Start(MPI_Wtime() - t_0));

    // determnine write size
    async_write_size.push_back(sqe->len);

    // phase start if first request. Add phase data and offset
    // [Note] Start of Async Phrase is control by if `async_write_time` is empty, instead of `phrase`
    // [Note] Since Sync/Async are tracked separately, not need to consider the merge of both
    p_aw->Phase_Start(async_write_request.empty(), async_write_time.back(), async_write_size.back(), sqe->off);

    // save request flag and set request counter (required and actual to one)
    async_write_request.push_back(sqe->user_data);
    async_write_queue_req.push_back(1);
    async_write_queue_act.push_back(1);

    IOtraceIOuring::LogWithAction<VerbosityLevel::BASIC_LOG>([&]()
                                                            {
                                                                static long int counter = 1;
                                                                IOtraceIOuring::Log<VerbosityLevel::BASIC_LOG>(
                                                                    "%s > rank %i > #%li will async write %i bytes\n",
                                                                    caller, rank, counter++, sqe->len); });
    IOtraceIOuring::Log<VerbosityLevel::DETAILED_LOG>(
        "%s > rank %i %s>> started asnyc write @ %f s %s\n", caller, rank, GREEN, async_write_time.back(), BLACK);
    IOtraceIOuring::Log<VerbosityLevel::DEBUG_LOG>(
        "%s > rank %i %s>>> has offset %lli %s\n", caller, rank, YELLOW, sqe->off, BLACK);
 
    Overhead_End();
}