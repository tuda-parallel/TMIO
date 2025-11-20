#include "iotrace.h"

/**
 * @file iotrace.cpp
 * @brief Contains definitions of methods from the \e Iotrace class.
 * @author Ahmad Tarraf
 * @date 05.08.2021
 */

/*
 * awr: async write required
 * awa: async write actual
 * arr: async read required
 * ara: async read actual
 */
template <typename Tag>
IOtraceBase<Tag>::IOtraceBase(void) : p_aw(&aw), p_ar(&ar), p_sw(&sw), p_sr(&sr)
{
    rank = 0;
    processes = 0;
    open = 0;

    t_async_write_start = std::numeric_limits<double>::quiet_NaN();
    t_sync_write_start = std::numeric_limits<double>::quiet_NaN();
    t_async_read_start = std::numeric_limits<double>::quiet_NaN();
    t_sync_read_start = std::numeric_limits<double>::quiet_NaN();

    size_async_write = 0;
    size_sync_write = 0;
    size_async_read = 0;
    size_sync_read = 0;

    p_aw = &aw;
    p_ar = &ar;
    p_sw = &sw;
    p_sr = &sr;
}

/**
 * @brief sets the attribute rank to the current MPI rank
 */
template <typename Tag>
void IOtraceBase<Tag>::Init(void)
{

    t_0 = MPI_Wtime();
    t_summary = t_0;
    //? create copy of communicator
    MPI_Comm_dup(MPI_COMM_WORLD, &IO_WORLD);
    MPI_Comm_set_errhandler(IO_WORLD, MPI_ERRORS_RETURN);
    // int r,s;
    // MPI_Comm_rank(IO_WORLD, &r);
    // MPI_Comm_size(IO_WORLD, &s);
    // printf("%sIO RANK: %d \t SIZE: %d%s\n",BLUE,r,s,BLACK);

    //? get rank id an size of ranks
    MPI_Comm_rank(IO_WORLD, &rank);
    MPI_Comm_size(IO_WORLD, &processes);

    //? init tracers for the 4 modes:
    p_aw->Mode(rank, 1);    // async write
    p_ar->Mode(rank, 0);    // async read
    p_sw->Mode(rank, 1, 0); // sync write
    p_sr->Mode(rank, 0, 0); // sync read

#if defined BW_LIMIT || defined CUSTOM_MPI
    bw_limit.Init(rank, processes, p_aw, p_ar, p_sw, p_sr);
#endif

    if (rank == 0)
    {
        std::string info = "";
#if defined BW_LIMIT || defined CUSTOM_MPI
        info = bw_limit.Info();
#endif
        printf("\n===========================\n"
               "        TMIO Settings      \n"
               "===========================\n"
               "Test    : %i\n"
               "Calc    : %i\n"
               "Samples : %i\n"
               "Tracing : %s\n"
               "%s\n"
               "%s===========================\n\n",
               TEST, DO_CALC, ALL_SAMPLES, this->kLibName, iohf::Get_File_Format(FILE_FORMAT).c_str(), info.c_str());
    }
#if IOTRACE_VERBOSE >= 1
    printf("%s > rank %i / %i %s> I/O tracer initiated %s\n", caller, rank, processes - 1, BLUE, BLACK);
#endif
}

/**
 * @brief displays a summary of the results to the out stream.
 *  @param finalize: if true, summary is called through MPI_finalize -> remove all unended data
 */
template <typename Tag>
void IOtraceBase<Tag>::Summary(void)
{
    // iohf::Function_Debug(__PRETTY_FUNCTION__);
    delta_t_app = delta_t_app + (MPI_Wtime() - t_summary);
    // printf("%s > rank %i > generating I/O summary start %f \n", caller, rank,delta_t_app);
    Time_Info(std::string(this->kLibName) + " > Summary > started at");
#if IOTRACE_VERBOSE >= 1
    // if (rank == 0)
    // printf("%s > rank %i %s> Elapsed time: %e s %s\n", caller, rank, GREEN, MPI_Wtime() - t_0, BLACK);
    printf("%s > rank %i > generating I/O summary \n", caller, rank);
#endif

    // close phases if flie close was skipped
    if (p_sw->phase || p_sr->phase)
    {
        p_sw->Phase_End_Sync(t_sync_write_end);
        p_sr->Phase_End_Sync(t_sync_read_end);
        if (rank == 0)
            printf("%sWarning: File was not closed. Close file to obtained accurate information for sync I/O operations%s\n ", RED, BLACK);
    }

#if ONLINE == 0 // consider individual requests
    p_aw->Bandwidth_In_Phase_Offline();
    p_ar->Bandwidth_In_Phase_Offline();
    p_sw->Bandwidth_In_Phase_Offline();
    p_sr->Bandwidth_In_Phase_Offline();
#endif
    // number of I/O operations each rank performed ({async write, async read, sync write, sync read})
    n_struct n = {
        (int)p_aw->phase_data.size(),
        (int)p_ar->phase_data.size(),
        (int)p_sw->phase_data.size(),
        (int)p_sr->phase_data.size()};

    // Gather all n from all ranks
    n_struct *all_n = ioanalysis::Gather_N_OP(n, rank, processes, IO_WORLD);

#if IOTRACE_VERBOSE >= 2
    printf("%s > rank %i > generating I/O summary %s>> n.aw %i, n.ar %i, n.sw %i, n.sr %i, %s\n", caller, rank, GREEN, n.aw, n.ar, n.sw, n.sr, BLACK);
#endif
    Time_Info("Offline Calculation done >");

    // find total number of read and write phases over all processes
    n_struct n_phase;
    ioanalysis::Sum_N(all_n, n_phase, rank, processes);

    // Extracts arrays contating number of phases from all_n
    int *all_n_aw = ioanalysis::Get_N_From_ALL_N(p_aw, all_n, rank, processes);
    int *all_n_ar = ioanalysis::Get_N_From_ALL_N(p_ar, all_n, rank, processes);
    int *all_n_sw = ioanalysis::Get_N_From_ALL_N(p_sw, all_n, rank, processes);
    int *all_n_sr = ioanalysis::Get_N_From_ALL_N(p_sr, all_n, rank, processes);

    // get all data
    collect *all_aw = ioanalysis::Gather_Collect(p_aw, all_n_aw, rank, processes, IO_WORLD, finalize);
    collect *all_ar = ioanalysis::Gather_Collect(p_ar, all_n_ar, rank, processes, IO_WORLD, finalize);
    collect *all_sw = ioanalysis::Gather_Collect(p_sw, all_n_sw, rank, processes, IO_WORLD, finalize);
    collect *all_sr = ioanalysis::Gather_Collect(p_sr, all_n_sr, rank, processes, IO_WORLD, finalize);

// Communication test
#if IOTRACE_VERBOSE > 0
    int flag = 0;
    if (rank != 0)
        MPI_Recv(&flag, 1, MPI_INT, rank - 1, 100, IO_WORLD, MPI_STATUS_IGNORE);

    fflush(stdout);

    if (rank < processes - 1)
        MPI_Send(&flag, 1, MPI_INT, rank + 1, 100, IO_WORLD);
#endif
    //-----------------------------------
    Time_Info("Gather collect done >");

    statistics s_aw(all_aw, all_n_aw, rank, processes, true, true);
    statistics s_ar(all_ar, all_n_ar, rank, processes, false, true);
    statistics s_sw(all_sw, all_n_sw, rank, processes, true);
    statistics s_sr(all_sr, all_n_sr, rank, processes, false);
    Time_Info("statistics init done >");

// Gather metrics at thread level (b_ind,t_ind,..)
#if ALL_SAMPLES > 4
    s_aw.Gather_Ind_Bandwidth(rank, processes, p_aw->bandwidth_act, p_aw->bandwidth_req, p_aw->t_act_s, p_aw->t_act_e, p_aw->t_req_s, p_aw->t_req_e, IO_WORLD);
    s_ar.Gather_Ind_Bandwidth(rank, processes, p_ar->bandwidth_act, p_ar->bandwidth_req, p_ar->t_act_s, p_ar->t_act_e, p_ar->t_req_s, p_ar->t_req_e, IO_WORLD);
    s_sw.Gather_Ind_Bandwidth(rank, processes, p_sw->bandwidth_act, p_sw->bandwidth_req, p_sw->t_act_s, p_sw->t_act_e, p_sw->t_req_s, p_sw->t_req_e, IO_WORLD);
    s_sr.Gather_Ind_Bandwidth(rank, processes, p_sr->bandwidth_act, p_sr->bandwidth_req, p_sr->t_act_s, p_sr->t_act_e, p_sr->t_req_s, p_sr->t_req_e, IO_WORLD);
#endif
    Time_Info("Rank_Bandwidth calculation done >");

    // calculate statistics
    if (rank == 0)
    {
#if IOTRACE_VERBOSE >= 1
        printf("%s > rank %i > generating I/O summary %s> calculating statistics \n %s", caller, rank, BLUE, BLACK);
#endif

#if DO_CALC > 0 // Calculate overlapping bandwidth + rank and application metrics
        s_aw.Compute();
        s_ar.Compute();
        s_sw.Compute();
        s_sr.Compute();
#else // compute only rank statistics
        if (finalize && !online_file_generation)
        {
            s_aw.Compute_Rank_Metrics();
            s_ar.Compute_Rank_Metrics();
            s_sw.Compute_Rank_Metrics();
            s_sr.Compute_Rank_Metrics();
        }
#endif
    }
    Time_Info("Statistics compute done >");

    // printf("%s > rank %i > generating I/O summary end  %f \n", caller, rank,MPI_Wtime() - t_0);

    //? Overhead calculation
    //?-------------------------
    double *time = Overhead_Calculation();

    //? Print
    //?-------------------------
    if (rank == 0)
    {

        // if (finalize){
#if IOTRACE_VERBOSE >= 1
        printf("%s > rank %i > generating I/O summary %s> printing file %s\n", caller, rank, BLUE, BLACK);
#endif

        // double time_rank0[3] = {delta_t_app, delta_t_io_overhead, (MPI_Wtime() - t_0) - delta_t_app};
        double time_rank0[3] = {delta_t_app, delta_t_io_overhead, (MPI_Wtime() - t_summary) - delta_t_app};
        iotime io_time(time, time_rank0, s_sr, s_ar, s_sw, s_aw);
        if (finalize)
        {
            ioprint::Summary(this->kLibName, processes, s_sr, s_ar, s_sw, s_aw, io_time);
            if (online_file_generation == false)
#if FILE_FORMAT >= 1
                ioprint::Binary(this->kLibName, processes, s_sr, s_ar, s_sw, s_aw, io_time);
#else
                ioprint::Json(this->kLibName, processes, s_sr, s_ar, s_sw, s_aw, io_time);
#endif
        }
        else
            online_file_generation = true;

        if (online_file_generation == true)
        {
#if FILE_FORMAT >= 1
            ioprint::Binary(this->kLibName, processes, s_sr, s_ar, s_sw, s_aw, io_time);
#else
            ioprint::Jsonl(this->kLibName, processes, s_sr, s_ar, s_sw, s_aw, io_time);
#endif
        }
        Time_Info("Printing done >");
        s_aw.Clean();
        s_ar.Clean();
        s_sw.Clean();
        s_sr.Clean();
        free(all_n_aw);
        free(all_n_ar);
        free(all_n_sw);
        free(all_n_sr);
        free(all_n);
        free(all_aw);
        free(all_ar);
        free(all_sw);
        free(all_sr);
        free(time);
    }
    if (!finalize)
    {
        t_summary = MPI_Wtime() - t_0;
        delta_t_app = 0;
        delta_t_io_overhead = 0;

        p_sw->Clear_IO();
        p_sr->Clear_IO();
        p_aw->Clear_IO();
        p_ar->Clear_IO();

#if defined BW_LIMIT || defined CUSTOM_MPI
        bw_limit.Reset();
#endif
    }
    // printf("%s > rank %i > generating I/O summary end 2 %f \n", caller, rank,MPI_Wtime() - t_0);
}

//! ------------------------------ Async write tracing -------------------------------
//************************************************************************************
//*                               1. Write_Async_Start
//************************************************************************************

template <typename Tag>
void IOtraceBase<Tag>::Write_Async_Start_Impl(RequestIDType requestID, long long size, long long offset, double start_time)
{
    Overhead_Start(start_time);
    // get write timestamp
    async_write_time.push_back(start_time);

    // determnine write size
    async_write_size.push_back(size);

    // phase start if first request. Add phase data and offset
    p_aw->Phase_Start(async_write_request.empty(), async_write_time.back(), async_write_size.back(), offset);

    // save request flag and set request counter (required and actual to one)
    async_write_request.push_back(AsyncRequest<Tag>(requestID));
    async_write_queue_req.push_back(1);
    async_write_queue_act.push_back(1);

    // Logging
    IOtraceBase<Tag>::Log<VerbosityLevel::BASIC_LOG>(
        "%s > rank %i %s>> started async write @ %.2f s %s\n", caller, rank,
        GREEN, t_async_write_start, BLACK);
    IOtraceBase<Tag>::Log<VerbosityLevel::BASIC_LOG>(
        "%s > rank %i %s>>> has offset %lli %s\n", caller, rank, YELLOW, offset,
        BLACK);
    Overhead_End();
}

//************************************************************************************
//*                               2. Write_Async_End
//************************************************************************************
template <typename Tag>
void IOtraceBase<Tag>::Write_Async_End_Impl(RequestIDType request, int write_status)
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

            IOtraceBase<Tag>::LogWithAction<VerbosityLevel::DETAILED_LOG>([&]()
                                                                          {
                static long int counter = 1;
                IOtraceBase<Tag>::Log<VerbosityLevel::DETAILED_LOG>(
                    "%s > rank %i %s>> Async ended (act ended). Active async write requests %li/%li %s\n", 
                    caller, rank, GREEN, async_write_request.size(), counter++, BLACK); });
        }
    }

    IOtraceBase<Tag>::LogWithCondition<VerbosityLevel::BASIC_LOG>(
        write_status == 0,
        "%s > rank %i %s>>>> testing for asnc write completion %s\n", caller, rank, RED, BLACK);

    Overhead_End();
}

//************************************************************************************
//*                               3. Write_Async_Required
//************************************************************************************
template <typename Tag>
void IOtraceBase<Tag>::Write_Async_Required_Impl(RequestIDType request)
{
    IOtraceBase<Tag>::Log<VerbosityLevel::BASIC_LOG>(
        "%s > rank %i %s>> Wait reached (Req waiting) async write @ %.8f s, (Request: %ld)\n", caller, rank,
        GREEN, MPI_Wtime() - t_0, request);
    Overhead_Start(MPI_Wtime() - t_0);
    if (Check_Request_Write(request, &t_async_write_start, &size_async_write, 1))
    {
        p_aw->Phase_End_Req(size_async_write, t_async_write_start, MPI_Wtime() - t_0);

        IOtraceBase<Tag>::Log<VerbosityLevel::BASIC_LOG>(
            "%s > rank %i > write %lli bytes \n", caller, rank, size_async_write);
    }
    Overhead_End();
}

//! ------------------------------ Async read tracing -------------------------------
//************************************************************************************
//*                               1. Read_Async_Start
//************************************************************************************
template <typename Tag>
void IOtraceBase<Tag>::Read_Async_Start_Impl(RequestIDType requestID, long long size, long long offset, double start_time)
{
    Overhead_Start(start_time);
    // get read timestamp
    async_read_time.push_back(start_time);

    // determnine read size
    async_read_size.push_back(size);

    // phase start if first request. Add phase data and offset
    p_ar->Phase_Start(async_read_request.empty(), async_read_time.back(), async_read_size.back(), offset);

    // save request flag and set request counter (required and actual to one)
    async_read_request.push_back(AsyncRequest<Tag>(requestID));
    async_read_queue_req.push_back(1);
    async_read_queue_act.push_back(1);

    // Logging
    IOtraceBase<Tag>::Log<VerbosityLevel::BASIC_LOG>(
        "%s > rank %i %s>> started async read @ %.2f s %s\n", caller, rank,
        GREEN, t_async_read_start, BLACK);
    IOtraceBase<Tag>::Log<VerbosityLevel::BASIC_LOG>(
        "%s > rank %i %s>>> has offset %lli %s\n", caller, rank, YELLOW, offset,
        BLACK);
    Overhead_End();
}

//************************************************************************************
//*                               2. Read_Async_End
//************************************************************************************
template <typename Tag>
void IOtraceBase<Tag>::Read_Async_End_Impl(RequestIDType request, int read_status)
{
    // Print the request being waited on
    IOtraceBase<Tag>::Log<VerbosityLevel::BASIC_LOG>(
        "%s > rank %i %s>> Wait reached (Act waiting) async read @ %.8f s, (Request: %ld)\n", caller, rank,
        GREEN, MPI_Wtime() - t_0, request);
    Overhead_Start(MPI_Wtime() - t_0);

    // actual read ended signilized by flag of MPI_Test or at the end of MPI_Wait. This flag will always be true if the I/O operation ended
    if (read_status == 1)
    {
        //  first time the status of the actual read is quarried. Solves the problem of several MPI_Test
        if (Check_Request_Read(request, &t_async_read_start, &size_async_read, 2))
        {
            // add values to traced data and add phase values if condition is true:
            // Act_Done: if empty request reutrns 1 (act finished after wait) and if all request are done (= 0, act finished before wait) returns true
            // p_ar->Phase_End_Act(size_async_read, t_async_read_start, MPI_Wtime() - t_0,(async_read_request.empty() || (async_read_queue_act.size() == 1 && async_read_queue_act.back() == 0)));
            p_ar->Phase_End_Act(size_async_read, t_async_read_start, MPI_Wtime() - t_0, Act_Done(1));

            IOtraceBase<Tag>::Log<VerbosityLevel::BASIC_LOG>(
                "%s > rank %i > read %lli bytes \n", caller, rank, size_async_read);
            IOtraceBase<Tag>::Log<VerbosityLevel::BASIC_LOG>(
                "%s > rank %i %s>> read async requests ended async read @ %.2f s %s\n", caller, rank,
                GREEN, MPI_Wtime() - t_0, BLACK);
        }
    }

    IOtraceBase<Tag>::LogWithCondition<VerbosityLevel::DEBUG_LOG>(read_status == 0,
                                                                  "%s > rank %i %s>>>> testing for async read completion %s\n", caller, rank, RED, BLACK);

    Overhead_End();
}

//************************************************************************************
//*                               3. Read_Async_Required
//************************************************************************************
template <typename Tag>
void IOtraceBase<Tag>::Read_Async_Required_Impl(RequestIDType request)
{
    // Print the request being waited on
    IOtraceBase<Tag>::Log<VerbosityLevel::BASIC_LOG>(
        "%s > rank %i %s>> Wait reached (Req waiting) async read @ %.8f s, (Request: %ld)\n", caller, rank,
        GREEN, MPI_Wtime() - t_0, request);
    Overhead_Start(MPI_Wtime() - t_0);
    if (Check_Request_Read(request, &t_async_read_start, &size_async_read, 1))
    {   
        // Print an error message if start time is larger than end time
        double req_end_time = MPI_Wtime() - t_0;
        if (req_end_time < t_async_read_start) {
            IOtraceBase<Tag>::Log<VerbosityLevel::BASIC_LOG>(
                "%s > rank %i %s>>>> Warning: Async read required end time %.8f s is smaller than start time %.8f s %s\n", caller, rank, RED, req_end_time, t_async_read_start, BLACK);
            req_end_time = t_async_read_start; // set end time to start time to avoid negative durations
        }

        p_ar->Phase_End_Req(size_async_read, t_async_read_start, req_end_time);

        IOtraceBase<Tag>::Log<VerbosityLevel::BASIC_LOG>(
            "%s > rank %i %s>> active read async requests %li %s\n", caller, rank, GREEN, async_read_request.size(), BLACK);
    }
    Overhead_End();
}

//! ------------------------------ Sync write tracing -------------------------------
//************************************************************************************
//*                               1. Write_Sync_Start
//************************************************************************************
template <typename Tag>
void IOtraceBase<Tag>::Write_Sync_Start_Impl(long long size, long long offset, double start_time)
{
    // get write timestamp
    t_sync_write_start = Overhead_Start(start_time);

    // determnine write size
    size_sync_write = size;

#if SYNC_MODE == 1
    p_sw->Phase_Start(p_sw->flag && !(p_sw->phase), t_sync_write_start, size_sync_write, offset);
#else
    p_sw->Phase_Start(true, t_sync_write_start, size_sync_write, offset);
#endif

    IOtraceBase<Tag>::Log<VerbosityLevel::BASIC_LOG>(
        "%s > rank %i > will write %lli bytes \n", caller, rank, size);
    IOtraceBase<Tag>::Log<VerbosityLevel::DETAILED_LOG>(
        "%s > rank %i %s>> started sync write @ %.2f s %s\n", caller, rank,
        GREEN, t_sync_write_start, BLACK);
    Overhead_End();
}

//************************************************************************************
//*                               2. Write_Sync_End
//************************************************************************************
template <typename Tag>
void IOtraceBase<Tag>::Write_Sync_End_Impl(void)
{
    t_sync_write_end = Overhead_Start(MPI_Wtime() - t_0);

    p_sw->Add_Io(0, size_sync_write, t_sync_write_start, t_sync_write_end);

#if SYNC_MODE == 0
    p_sw->Phase_End_Sync(t_sync_write_end);
    IOtraceBase<Tag>::Log<VerbosityLevel::DETAILED_LOG>(
        "%s > rank %i %s>> ended   sync write @ %f s %s\n", caller, rank, GREEN, t_sync_write_end, BLACK);
#endif

    Overhead_End();
}

//! ------------------------------ Sync read tracing -------------------------------
//************************************************************************************
//*                               1. Read_Sync_Start
//************************************************************************************
template <typename Tag>
void IOtraceBase<Tag>::Read_Sync_Start_Impl(long long size, long long offset, double start_time)
{
    // get read timestamp
    t_sync_read_start = Overhead_Start(start_time);

    // determnine read size
    size_sync_read = size;

#if SYNC_MODE == 1
    p_sr->Phase_Start(p_sr->flag && !(p_sr->phase), t_sync_read_start, size_sync_read, offset);
#else
    p_sr->Phase_Start(true, t_sync_read_start, size_sync_read, offset);
#endif

    IOtraceBase<Tag>::Log<VerbosityLevel::BASIC_LOG>(
        "%s > rank %i > will read %lli bytes \n", caller, rank, size);
    IOtraceBase<Tag>::Log<VerbosityLevel::DETAILED_LOG>(
        "%s > rank %i %s>> started sync read @ %.2f s %s\n", caller, rank,
        GREEN, t_sync_read_start, BLACK);
    IOtraceBase<Tag>::Log<VerbosityLevel::DEBUG_LOG>(
        "%s > rank %i %s>>> has offset %lli %s\n", caller, rank, YELLOW, offset,
        BLACK);

    Overhead_End();
}

//************************************************************************************
//*                               2. Read_Sync_End
//************************************************************************************
template <typename Tag>
void IOtraceBase<Tag>::Read_Sync_End_Impl(void)
{
    t_sync_read_end = Overhead_Start(MPI_Wtime() - t_0);

    p_sr->Add_Io(0, size_sync_read, t_sync_read_start, t_sync_read_end);

#if SYNC_MODE == 0
    p_sr->Phase_End_Sync(t_sync_read_end);
    IOtraceBase<Tag>::Log<VerbosityLevel::DETAILED_LOG>(
        "%s > rank %i %s>> ended   sync read @ %f s %s\n", caller, rank, GREEN, t_sync_read_end, BLACK);
#endif

    Overhead_End();
}

//! ------------------------------ Open & Close -------------------------------
//************************************************************************************
//*                               1. Open
//************************************************************************************
template <typename Tag>
void IOtraceBase<Tag>::Open(void)
{
    open = 1;

#if SYNC_MODE == 1
    p_sw->flag = true;
    p_sr->flag = true;
#endif

#if IOTRACE_VERBOSE >= 2
    printf("%s > rank %i %s>> opened the file %s\n", caller, rank, GREEN, BLACK);
#endif
}

//************************************************************************************
//*                               2. Close
//************************************************************************************
template <typename Tag>
void IOtraceBase<Tag>::Close(void)
{

    if (open == 1)
    {
        open = 0;
#if SYNC_MODE == 1
        p_sw->Phase_End_Sync(t_sync_write_end);
        p_sr->Phase_End_Sync(t_sync_read_end);
#endif
#if IOTRACE_VERBOSE >= 2
        printf("%s > rank %i %s>> closed the file %s\n", caller, rank, GREEN, BLACK);
#endif
    }
}

//! ------------------------------ Queue & Core functions ----------------------------
//************************************************************************************
//*                               1. Check_Request_Write
//************************************************************************************
/**
 * @brief handles async write requests.Checks if the requests (req and act) for the bandwidth and throughput ended.
 * bandwidth requests are saved in the vector \e async_write_queue_req
 * throughput requests are saved in the vector \e async_write_queue_act.
 * The actual and the required I/O are allowed each to quary this status only once.
 * after that false is returned (solves problem with several MPI_Test/Wait)
 * if both I/O (req and actual) finished, the request is deleted.
 * @param request [in] pointer of the current reuqest to check
 * @param start_time [out] start time of the request
 * @param size [out] number of \e bytes transfered in
 * @param mode [in]  1 -> required |  2 -> actual
 * @return \e true for the first time the async I/O operation ended.
 */
template <typename Tag>
bool IOtraceBase<Tag>::
    Check_Request_Write(RequestIDType request, double *start_time, long long *size, int mode)
{

    if (!async_write_request.empty())
    {
        for (unsigned int i = 0; i < async_write_request.size(); i++)
        {
            if (async_write_request[i].check_request(request))
            {
                if (mode == 1)
                {
                    if (async_write_queue_req[i] == 0)
                        return false;
                    else
                        --async_write_queue_req[i]; // required queue
                }
                else if (mode == 2)
                {
                    if (async_write_queue_act[i] == 0)
                        return false;
                    else
                        --async_write_queue_act[i]; // actual queue
                }
                *start_time = async_write_time[i];
                *size = async_write_size[i];

                if (async_write_queue_req[i] == 0 && async_write_queue_act[i] == 0) // finished request > delete from queue
                {
                    async_write_time.erase(async_write_time.begin() + i);
                    async_write_size.erase(async_write_size.begin() + i);
                    async_write_request.erase(async_write_request.begin() + i);
                    async_write_queue_req.erase(async_write_queue_req.begin() + i);
                    async_write_queue_act.erase(async_write_queue_act.begin() + i);
                }

                return true;
            }
        }
    }
    return false;
}

//************************************************************************************
//*                               2. Check_Request_Read
//************************************************************************************
/**
 * @brief handles async read requests. Checks if the requests (req and act) for the bandwidth and throughput ended.
 * bandwidth requests are saved in the vector \e async_write_queue_req
 * throughput requests are saved in the vector \e async_write_queue_act.
 * The actual and the required I/O are allowed each to quary this status only once.
 * after that false is returned (solves problem with several MPI_Test/Wait)
 * if both I/O (req and actual) finished, the request is deleted.
 * @param request [in] pointer of the current reuqest to check
 * @param start_time [out] start time of the request
 * @param size [out] number of \e bytes transfered in
 * @param mode [in]  1 -> required |  2 -> actual
 * @return \e true for the first time the async I/O operation ended.
 */
template <typename Tag>
bool IOtraceBase<Tag>::Check_Request_Read(RequestIDType request, double *start_time, long long *size, int mode)
{
    // FIXME: Add pthread lock to protect the following variables
    if (!async_read_request.empty())
    {
        for (unsigned int i = 0; i < async_read_request.size(); i++)
        {
            if (async_read_request[i].check_request(request))
            {
                if (mode == 1)
                {
                    if (async_read_queue_req[i] == 0)
                        return false;
                    else
                        --async_read_queue_req[i]; // required queue
                }
                if (mode == 2)
                {
                    if (async_read_queue_act[i] == 0)
                        return false;
                    else
                        --async_read_queue_act[i]; // actual queue
                }
                *start_time = async_read_time[i];
                *size = async_read_size[i];

                if (async_read_queue_req[i] == 0 && async_read_queue_act[i] == 0) // finished request > delete from queue
                {
                    async_read_time.erase(async_read_time.begin() + i);
                    async_read_size.erase(async_read_size.begin() + i);
                    async_read_request.erase(async_read_request.begin() + i);
                    async_read_queue_req.erase(async_read_queue_req.begin() + i);
                    async_read_queue_act.erase(async_read_queue_act.begin() + i);
                }
                return true;
            }
        }
    }
    return false;
}

//************************************************************************************
//*                               3. Act_Done
//************************************************************************************
/**
 * @brief checks if all actual requests write or read (see \e mode) ended.
 * bandwidth requests are saved in the vector \e async_write_queue_req
 * @param mode [in]  1 -> write |  2 -> read
 * @return \e false if requests read/write (see mode) are active. Else true.
 */
template <typename Tag>
bool IOtraceBase<Tag>::Act_Done(int mode)
{
    bool flag = true;

    if (mode == 0)
    { // write
        for (unsigned int i = 0; i < async_write_queue_act.size(); i++)
        {
            if (async_write_queue_act[i] != 0)
                flag = false;
        }
    }

    else
    { // read
        for (unsigned int i = 0; i < async_read_queue_act.size(); i++)
        {
            if (async_read_queue_act[i] != 0)
                flag = false;
        }
    }

    return flag;
}

//************************************************************************************
//*                               4. Get_Relevant_Ranks
//************************************************************************************
/**
 * @brief get ranks that perform I/O
 *
 * @param fh [in] filepointer.
 * @return number of ranks that performed I/O on the file.
 */
template <typename Tag>
int IOtraceBase<Tag>::Get_Relevant_Ranks(MPI_File fh)
{
    MPI_Group tmpGroup;
    int size;
    MPI_File_get_group(fh, &tmpGroup);
    MPI_Group_size(tmpGroup, &size);
    MPI_Group_free(&tmpGroup);
    // std::cout << "Ranks doing I/O: " << size << std::endl;
    return size;
}

//! ------------------------------- Overhead Tracing----------------------------------
//************************************************************************************
//*                               5. Overhead_Start
//************************************************************************************
template <typename Tag>
double IOtraceBase<Tag>::Overhead_Start(double t)
{

#if OVERHEAD == 1
    t_overhead = t;
#endif

    return t;
}

//************************************************************************************
//*                               6. Overhead_End
//************************************************************************************
template <typename Tag>
void IOtraceBase<Tag>::Overhead_End(void)
{

#if OVERHEAD == 1
    delta_t_io_overhead += MPI_Wtime() - t_0 - t_overhead;
#endif
};

//************************************************************************************
//*                               7. Overhead_Calculation
//************************************************************************************
/**
 * @brief calculates the overhead time. iF flag \OVERHEAD is provided, overhead time
 * during the runtime of the application is calculated in addion to the overhead at the end of the application
 * @details the returned vector contains [application_runtime, overhead_runtime, overhead_post_runtimme]
 * @return double*
 */
template <typename Tag>
double *IOtraceBase<Tag>::Overhead_Calculation(void)
{
// iohf::Function_Debug(__PRETTY_FUNCTION__);
#if IOTRACE_VERBOSE >= 1
    printf("%s > rank %i > generating I/O summary %s> calculating overhead \n %s", caller, rank, BLUE, BLACK);
#endif

    double *time_array = NULL;

    int n_time = 2;
#if OVERHEAD == 1
    n_time = 3;
#endif

    double tmp_time[n_time];
    tmp_time[0] = delta_t_app; // application runtime including in-period overhead, not include summary overhead

#if OVERHEAD == 1
    tmp_time[2] = delta_t_io_overhead; // in-period overhead during applicaiton runtime
#endif

    // tmp_time[1] = (MPI_Wtime() - t_0) - delta_t_app; // overhead after application finishes
    tmp_time[1] = (MPI_Wtime() - t_summary) - delta_t_app; // summary overhead after application finishes

    if (rank == 0)
        time_array = (double *)malloc(sizeof(double) * n_time);

    MPI_Reduce(tmp_time, time_array, n_time, MPI_DOUBLE, MPI_SUM, 0, IO_WORLD);

#if IOTRACE_VERBOSE >= 2
#if OVERHEAD == 1
    printf("%s > rank %i > generating I/O summary %s>> values are: [%e %e %e] \n %s", caller, rank, GREEN, tmp_time[0], tmp_time[1], tmp_time[2], BLACK);
    if (rank == 0)
        printf("%s > rank %i > generating I/O summary %s>> Aggregated values are: [%e %e %e] \n %s", caller, rank, GREEN, time_array[0], time_array[1], time_array[2], BLACK);
#else
    printf("%s > rank %i > generating I/O summary %s>> values are: [%e %e] \n %s", caller, rank, GREEN, tmp_time[0], tmp_time[1], BLACK);
    if (rank == 0)
        printf("%s > rank %i > generating I/O summary %s>> Aggregated values are: [%e %e] \n %s", caller, rank, GREEN, time_array[0], time_array[1], BLACK);
#endif
#endif

    return time_array;
}

template <typename Tag>
void IOtraceBase<Tag>::Time_Info(std::string s)
{
#ifdef TIME_VERBOSE
    if (rank == 0)
    {
        // static double t_passed = MPI_Wtime() - t_0;
        static double t_passed = MPI_Wtime() - t_summary;
        printf("%s > rank %i %s> IOtrace > %s time: %.4e s --> passed time %.4f s %s\n", caller, rank, YELLOW, s.c_str(), MPI_Wtime() - t_0, (MPI_Wtime() - t_0) - t_passed, BLACK);
        // t_passed = MPI_Wtime() - t_0;
        t_passed = MPI_Wtime() - t_summary;
    }
#endif
}

//! ------------------------------ Set flags -------------------------------
//************************************************************************************
//*                               3. Set (bool)
//************************************************************************************
/**
 * @brief assigns finalize flag.
 *
 * @param Name of flag
 * @param ture of false
 */
template <typename Tag>
void IOtraceBase<Tag>::Set(std::string flag, bool value)
{

    if (flag == "finalize")
        finalize = value;
    else
        printf("not supported assignment");
}

//! ---------------------- Bw limit with Custom MPI implementaiton -------------------
//************************************************************************************
//*                               Bw_limit
//************************************************************************************

#ifdef BW_LIMIT
template <typename Tag>
void IOtraceBase<Tag>::Apply_Limit(void)
{
    Overhead_Start(MPI_Wtime() - t_0);
    bw_limit.Limit_Async();
    Overhead_End();
}
#endif

//! ##### modify T and duration in case custom MPI version
//************************************************************************************
//*                    Replace values from MPI_Test*
//************************************************************************************
#ifdef CUSTOM_MPI
template <typename Tag>
void IOtraceBase<Tag>::Replace_Test(void)
{
    Overhead_Start(MPI_Wtime() - t_0);
    bw_limit.Set_Throughput();
    Overhead_End();
}
#endif

// Explicit instantiation for MPI_Tag
template class IOtraceBase<MPI_Tag>;

#if ENABLE_LIBC_TRACE == 1
// Explicit instantiation for Libc_Tag
template class IOtraceBase<Libc_Tag>;
#endif // ENABLE_LIBC_TRACE

#if ENABLE_IOURING_TRACE == 1
// Explicit instantiation for IOuring_Tag
template class IOtraceBase<IOuring_Tag>;
#endif // ENABLE_IOURING_TRACE