#include "iotrace.h"

//! ------------------------------ Async write tracing -------------------------------
//************************************************************************************
//*                               1. Write_Async_Start
//************************************************************************************
/**
 * @brief starts tracing the async write call. Takes timestamp of function call once entered.
 * @param count    [in] counting variable from write operations. number of variables of type datarype to write
 * @param datatype [in] data type of the variables to write
 * @param request  [in] write request
 * @param offset   [in,optional] offset of the I/O operation
 */
void IOtraceMPI::Write_Async_Start(int count, MPI_Datatype datatype, MPI_Request *request, MPI_Offset offset)
{
    // get write timestamp
    async_write_time.push_back(Overhead_Start(MPI_Wtime() - t_0));

    // determnine write size
    MPI_Type_size(datatype, &data_size_write);
    async_write_size.push_back(count * data_size_write);

    // phase start if first request. Add phase data and offset
    // [Note] Start of Async Phrase is control by if `async_write_time` is empty, instead of `phrase`
    // [Note] Since Sync/Async are tracked separately, not need to consider the merge of both
    p_aw->Phase_Start(async_write_request.empty(), async_write_time.back(), async_write_size.back(), offset);

    // save request flag and set request counter (required and actual to one)
    async_write_request.push_back(request);
    async_write_queue_req.push_back(1);
    async_write_queue_act.push_back(1);

    IOtraceMPI::LogWithAction<VerbosityLevel::BASIC_LOG>([&]()
                                                         {
        static long int counter = 1;
        IOtraceMPI::Log<VerbosityLevel::BASIC_LOG>(
            "%s > rank %i > #%li will async write %i x %i = %lli bytes\n",
            caller, rank, counter++, count, data_size_write, async_write_size.back()); });
    IOtraceMPI::Log<VerbosityLevel::DETAILED_LOG>(
        "%s > rank %i %s>> started asnyc write @ %f s %s\n", caller, rank, GREEN, async_write_time.back(), BLACK);
    IOtraceMPI::Log<VerbosityLevel::DEBUG_LOG>(
        "%s > rank %i %s>>> has offset %lli %s\n", caller, rank, YELLOW, offset, BLACK);

    Overhead_End();
}

//************************************************************************************
//*                               2. Write_Async_End
//************************************************************************************
/**
 * @brief ends tracing the async write call. Takes timestamp of function call as endtime for the I/O operation.
 * @param request [in] request is compared to write request from Write_Async_Start
 * @param write_status [in,optinoal] if set to 1, indicates that the request is over. Used when tracing a MPI_Test call
 */
void IOtraceMPI::Write_Async_End(MPI_Request *request, int write_status)
{
    Overhead_Start(MPI_Wtime() - t_0);

    // actual write ended signilized by flag of MPI_Test or at the end of MPI_Wait. This flag will always be true if the I/O operation ended
    if (write_status == 1)
    {
        //  first time the status of the actual write is quarried. Solves the problem of several MPI_Test
        if (Check_Request_Write(request, &t_async_write_start, &size_async_write, 2))
        {
            // add values to traced data and add phase values if condition is true:
            // Act_Done: if empty request reutrns 1 (act finished after wait) and if all request are done (= 0, act finished before wait) returns true
            // p_aw->Phase_End_Act(size_async_write, t_async_write_start, MPI_Wtime() - t_0,(async_write_request.empty() || (async_write_queue_act.size() == 1 && async_write_queue_act.back() == 0)));
            p_aw->Phase_End_Act(size_async_write, t_async_write_start, MPI_Wtime() - t_0, Act_Done(0));

            IOtraceMPI::Log<VerbosityLevel::DETAILED_LOG>(
                "%s > rank %i %s>> active write async requests %li %s\n", caller, rank, GREEN, async_write_request.size(), BLACK);
            IOtraceMPI::Log<VerbosityLevel::DEBUG_LOG>(
                "%s > rank %i %s>> write async requests ended at %f %s\n", caller, rank, RED, p_aw->phase_data.back().t_end_act, BLACK);
        }
    }

    IOtraceMPI::LogWithCondition<VerbosityLevel::DEBUG_LOG>(
        write_status == 0,
        "%s > rank %i %s>>>> testing for asnc write completion %s\n", caller, rank, RED, BLACK);

    Overhead_End();
}

//************************************************************************************
//*                               3. Write_Async_Required
//************************************************************************************
/**
 * @brief ends tracing the required async write call. Takes timestamp of function call as endtime for the I/O operation.
 * Once this function is called, the required Phase ends
 * @param request [in] request is compared to read request from Read_Async_Start
 */
void IOtraceMPI::Write_Async_Required(MPI_Request *request)
{
    Overhead_Start(MPI_Wtime() - t_0);

    if (Check_Request_Write(request, &t_async_write_start, &size_async_write, 1))
    {
        p_aw->Phase_End_Req(size_async_write, t_async_write_start, MPI_Wtime() - t_0);

        IOtraceMPI::Log<VerbosityLevel::DETAILED_LOG>(
            "%s > rank %i %s>> active write async requests %li %s\n", caller, rank, GREEN, async_write_request.size(), BLACK);
    }
    Overhead_End();
}

//! ------------------------------ Async read tracing -------------------------------
//************************************************************************************
//*                               1. Read_Async_Start
//************************************************************************************
/**
 * @brief starts tracing the async read call. Takes timestamp of function call.
 * @param count    [in] counting variable from write operations. number of variables of type datarype to write
 * @param datatype [in] data type of the variables to write
 * @param request  [in] write request
 * @param offset   [in,optional] offset of the I/O operation
 */
void IOtraceMPI::Read_Async_Start(int count, MPI_Datatype datatype, MPI_Request *request, MPI_Offset offset)
{
    // get read timestamp
    async_read_time.push_back(Overhead_Start(MPI_Wtime() - t_0));

    // determnine read size
    MPI_Type_size(datatype, &data_size_read);
    async_read_size.push_back(count * data_size_read);

    // phase start if first request. Add phase data and offset
    p_ar->Phase_Start(async_read_request.empty(), async_read_time.back(), async_read_size.back(), offset);

    // save request flag and set request counter (required and actual to one)
    async_read_request.push_back(request);
    async_read_queue_req.push_back(1);
    async_read_queue_act.push_back(1);

    IOtraceMPI::LogWithAction<VerbosityLevel::BASIC_LOG>([&]()
                                                         {
        static long int counter = 1;
        IOtraceMPI::Log<VerbosityLevel::BASIC_LOG>(
            "%s > rank %i > #%li will asnyc read %i x %i = %lli bytes \n",
                    caller, rank, counter++, count, data_size_read, async_read_size.back()); });
    IOtraceMPI::Log<VerbosityLevel::DETAILED_LOG>(
        "%s > rank %i %s>> started asnyc read @ %f s %s\n", caller, rank, GREEN, async_read_time.back(), BLACK);
    IOtraceMPI::Log<VerbosityLevel::DEBUG_LOG>(
        "%s > rank %i %s>>> has offset %lli %s\n", caller, rank, YELLOW, offset, BLACK);
    Overhead_End();
}

//************************************************************************************
//*                               2. Read_Async_End
//************************************************************************************
/*!
 * @brief ends tracing the async read call. Takes timestamp of function call as endtime for the I/O operation.
 * @param request [in] request is compared to read request from Read_Async_Start
 * @param read_status [in,optinoal] if set to 1, indicates that the request is over. Used when tracing a MPI_Test call
 */
void IOtraceMPI::Read_Async_End(MPI_Request *request, int read_status)
{

    Overhead_Start(MPI_Wtime() - t_0);

    if (read_status == 1)
    { // read ended
        if (Check_Request_Read(request, &t_async_read_start, &size_async_read, 2))
        {
            // add values to traced data and add phase values if condition is true
            // p_ar->Phase_End_Act(size_async_read, t_async_read_start, MPI_Wtime() - t_0, (async_read_request.empty() || (async_read_queue_act.size() == 1 && async_read_queue_act.back() == 0)));
            // Act_Done: if empty request reutrns 1 (act finished after wait) and if all request are done (= 0, act finished before wait) returns true
            p_ar->Phase_End_Act(size_async_read, t_async_read_start, MPI_Wtime() - t_0, Act_Done(1));
            // std::cout << "Act_Done return" << Act_Done(1) << std::endl;

            IOtraceMPI::LogWithAction<VerbosityLevel::DETAILED_LOG>([&]()
                                                                    {
                IOtraceMPI::Log<VerbosityLevel::DETAILED_LOG>(
                    "%s > rank %i %s>> active read async requests %li %s\n", caller, rank, GREEN, async_read_request.size(), BLACK);
                iohf::Disp(async_read_queue_req, async_read_queue_req.size(), std::string(caller) + " > rank " + std::to_string(rank) + " >> async_read_queue_req ", 1); });
            IOtraceMPI::Log<VerbosityLevel::DEBUG_LOG>(
                "%s > rank %i %s>> read async requests ended at %f %s\n", caller, rank, RED, p_ar->phase_data.back().t_end_act, BLACK);
        }
    }

    IOtraceMPI::LogWithCondition<VerbosityLevel::DEBUG_LOG>(read_status == 0,
                                                            "%s > rank %i %s>>>> testing for async read completion %s\n", caller, rank, RED, BLACK);

    Overhead_End();
}

//************************************************************************************
//*                               3. Read_Async_Required
//************************************************************************************
/**
 * @brief ends tracing the required async read call. Takes timestamp of function call as endtime for the I/O operation.
 * Once this function is called, the required Phase ends
 * @param request [in] request is compared to read request from Read_Async_Start
 */
void IOtraceMPI::Read_Async_Required(MPI_Request *request)
{
    Overhead_Start(MPI_Wtime() - t_0);

    if (Check_Request_Read(request, &t_async_read_start, &size_async_read, 1))
    {
        p_ar->Phase_End_Req(size_async_read, t_async_read_start, MPI_Wtime() - t_0);

        IOtraceMPI::Log<VerbosityLevel::DETAILED_LOG>(
            "%s > rank %i %s>> active read async requests %li%s\n", caller, rank, GREEN, async_read_request.size(), BLACK);
    }
    Overhead_End();
}

//! ------------------------------ Sync write tracing -------------------------------
//************************************************************************************
//*                               1. Write_Sync_Start
//************************************************************************************
/**
 * @brief starts tracing the sync write call. Takes timestamp of function call
 * @param count   : counting variable from write operations. number of variables of type datarype to write
 * @param datatype: data type of the variables to write
 * @param offset   [in,optional] offset of the I/O operation
 */
void IOtraceMPI::Write_Sync_Start(int count, MPI_Datatype datatype, MPI_Offset offset)
{

    // get write timestamp
    t_sync_write_start = Overhead_Start(MPI_Wtime() - t_0);

    // determnine write size
    MPI_Type_size(datatype, &data_size_write);
    size_sync_write = count * data_size_write; // in B

#if SYNC_MODE == 1
    p_sw->Phase_Start(p_sw->flag && !(p_sw->phase), t_sync_write_start, size_sync_write, offset);
#else
    p_sw->Phase_Start(true, t_sync_write_start, size_sync_write, offset);
#endif

    // Logging
    IOtraceMPI::Log<VerbosityLevel::BASIC_LOG>(
        "%s > rank %i > will write %i x %i bytes \n", caller, rank, count, data_size_write);
    IOtraceMPI::Log<VerbosityLevel::DETAILED_LOG>(
        "%s > rank %i %s>> started sync write @ %.2f s %s\n", caller, rank, GREEN, t_sync_write_start, BLACK);
    IOtraceMPI::Log<VerbosityLevel::DEBUG_LOG>(
        "%s > rank %i %s>>> has offset %lli %s\n", caller, rank, YELLOW, offset, BLACK);

    Overhead_End();
}

//************************************************************************************
//*                               2. Write_Sync_End
//************************************************************************************
/**
 * @brief ends tracing the write call. Takes timestamp of function call
 */
void IOtraceMPI::Write_Sync_End(void)
{
    t_sync_write_end = Overhead_Start(MPI_Wtime() - t_0);

    p_sw->Add_Io(0, size_sync_write, t_sync_write_start, t_sync_write_end);

#if SYNC_MODE == 0
    p_sw->Phase_End_Sync(t_sync_write_end);
    IOtraceMPI::Log<VerbosityLevel::DETAILED_LOG>(
        "%s > rank %i %s>> ended   sync write @ %f s %s\n", caller, rank, GREEN, t_sync_write_end, BLACK);
#endif

    Overhead_End();
}

//! ------------------------------ Sync read tracing -------------------------------
//************************************************************************************
//*                               1. Read_Sync_Start
//************************************************************************************
/**
 * @brief starts tracing the sync read call. Takes timestamp of function call
 * @param count    [in] counting variable from read operations. number of variables of type datarype to read
 * @param datatype [in] data type of the variables to read
 * @param offset   [in,optional] offset of the I/O operation
 */
void IOtraceMPI::Read_Sync_Start(int count, MPI_Datatype datatype, MPI_Offset offset)
{
    // get read timestamp
    t_sync_read_start = Overhead_Start(MPI_Wtime() - t_0);

    // determnine read size
    MPI_Type_size(datatype, &data_size_read);
    size_sync_read = count * data_size_read; // in B

#if SYNC_MODE == 1
    p_sr->Phase_Start(p_sr->flag && !(p_sr->phase), t_sync_read_start, size_sync_read, offset);
#else
    p_sr->Phase_Start(true, t_sync_read_start, size_sync_read, offset);
#endif

    // Logging
    IOtraceMPI::Log<VerbosityLevel::BASIC_LOG>(
        "%s > rank %i > will read %i x %i bytes \n", caller, rank, count, data_size_read);
    IOtraceMPI::Log<VerbosityLevel::DETAILED_LOG>(
        "%s > rank %i %s>> started sync read @ %.2f s %s\n", caller, rank, GREEN, t_sync_read_start, BLACK);
    IOtraceMPI::Log<VerbosityLevel::DEBUG_LOG>(
        "%s > rank %i %s>>> has offset %lli %s\n", caller, rank, YELLOW, offset, BLACK);

    Overhead_End();
}

//************************************************************************************
//*                               2. Read_Sync_End
//************************************************************************************
/**
 * @brief ends tracing the sync read call. Takes timestamp of function call
 */
void IOtraceMPI::Read_Sync_End(void)
{
    t_sync_read_end = Overhead_Start(MPI_Wtime() - t_0);

    p_sr->Add_Io(0, size_sync_read, t_sync_read_start, t_sync_read_end);

#if SYNC_MODE == 0
    p_sr->Phase_End_Sync(t_sync_read_end);
    IOtraceMPI::Log<VerbosityLevel::DETAILED_LOG>(
        "%s > rank %i %s>> ended   sync read @ %f s %s\n", caller, rank, GREEN, t_sync_read_end, BLACK);
#endif

    Overhead_End();
}
