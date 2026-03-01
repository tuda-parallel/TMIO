#include "iodata.h"

IOdata::IOdata(void): phase(false)
{
}

void IOdata::Mode(int rank, TransactionType tt)
{
    std::lock_guard lock(phase_data_lock);

    transaction_type = tt;

    phase = false;
    rank  = rank;
    transaction_identifier = type_string(tt);
    
    #if ONLINE == 1
    online_counter = 0;
    #endif

#if IODATA_VERBOSE >= 2
    printf("%s > rank %i %s> collecting data for %s%s\n", caller, rank, CYAN, type_string(), BLACK);
#endif
}

/**
 * @brief collects individual I/O operations
 *
 * @param b           [in] number of bytes transfered
 * @param ts          [in] start time of I/O operation
 * @param te          [in] end time of I/O operation
 * @param path        [in] file this I/O operation belongs to
 * @note    Should only be called with data_lock engaged
 * @details Adds IO operation to tracked data
 */
void IOdata::Add_IO_Req(long long b, double ts, double te, [[maybe_unused]] const std::filesystem::path* path)
{
#if BW_LIMIT_GRANULARITY > 1 || PREFETCH
    if (path) {
        path_to_io.try_emplace(*path).first->second.push_back(bandwidth_req.size());
    }
#endif
#if SAME_T_END == 1
    bandwidth_req.push_back(b / (phase_data.back().t_end_req - ts));
#else
    bandwidth_req.push_back(b / (te - ts));
#endif

    bytes.push_back(b);

#if ALL_SAMPLES > 4
    t_req_s.push_back(ts);
    #if SAME_T_END == 1
    t_req_e.push_back(phase_data.back().t_end_req );
    #else
    t_req_e.push_back(te);
    #endif
#endif

#if IODATA_VERBOSE >= 1
#if SAME_T_END == 1
    printf("%s > rank %i %s> %s phase %li > #%lli > req over: %.3f KB handled in %f s -> B(%li,%lli) = %.3f KB/s%s\n", caller, rank, CYAN, transaction_identifier, phase_data.size(), bandwidth_req.size() - count_opertaions_agg(phase_data.size() - 1), (double)b / 1000, phase_data.back().t_end_req - ts, phase_data.size(), bandwidth_req.size() - count_opertaions_agg(phase_data.size() - 1), (double)bandwidth_req.back() / 1000, BLACK);
#else
    printf("%s > rank %i %s> %s phase %li > #%lli > req over: %.3f KB handled in %f s -> B(%li,%lli) = %.3f KB/s%s\n", caller, rank, CYAN, transaction_identifier, phase_data.size(), bandwidth_req.size() - count_opertaions_agg(phase_data.size() - 1), (double)b / 1000, te - ts, phase_data.size(), bandwidth_req.size() - count_opertaions_agg(phase_data.size() - 1), (double)bandwidth_req.back() / 1000, BLACK);
#endif
#endif
#if IODATA_VERBOSE >= 2
#if SAME_T_END == 1
    printf("%s > rank %i %s> %s phase %li > #%lli >> opertation from %f -> %f %s\n", caller, rank, YELLOW, transaction_identifier, phase_data.size(), bandwidth_req.size() - count_opertaions_agg(phase_data.size() - 1), ts, phase_data.back().t_end_req, BLACK);
#else
    printf("%s > rank %i %s> %s phase %li > #%lli >> opertation from %f -> %f %s\n", caller, rank, YELLOW, transaction_identifier, phase_data.size(), bandwidth_req.size() - count_opertaions_agg(phase_data.size() - 1), ts, te, BLACK);
#endif
#endif
    
}

/**
 * @brief collects individual I/O operations
 *
 * @param b           [in] number of bytes transfered
 * @param ts          [in] start time of I/O operation
 * @param te          [in] end time of I/O operation
 * @param path        [in] file this I/O operation belongs to
 * @note    Should only be called with data_lock engaged
 * @details Adds IO operation to tracked data
 */
void IOdata::Add_IO_Act_Impl(long long b, double ts, double te)
{
    bandwidth_act.push_back(b / (te - ts));
#if ALL_SAMPLES > 4
    t_act_s.push_back(ts);
    t_act_e.push_back(te);
#endif


#if IODATA_VERBOSE >= 1
    printf("%s > rank %i %s> %s phase %li > #%lli > act over: %.3f KB handled in %f s -> T(%li,%lli) = %.3f KB/s%s\n", caller, rank, CYAN, transaction_identifier, phase_data.size(), bandwidth_act.size() - count_opertaions_agg(phase_data.size() - 1), (double)b / 1000, te - ts, phase_data.size(), bandwidth_act.size() - count_opertaions_agg(phase_data.size() - 1), (double)bandwidth_act.back() / 1000, BLACK);
#endif
#if IODATA_VERBOSE >= 2
    printf("%s > rank %i %s> %s phase %li > #%lli >> opertation from %f -> %f %s\n", caller, rank, YELLOW, transaction_identifier, phase_data.size(), bandwidth_act.size() - count_opertaions_agg(phase_data.size() - 1), ts, te, BLACK);
#endif
}

/**
 *
 * @details Remove all data traced so far
 */
void IOdata::Clear_IO(void)
{
    std::lock_guard lock(phase_data_lock);
    bandwidth_act.clear();
    bandwidth_req.clear();
    t_act_s.clear();
    t_act_e.clear();
    t_req_s.clear();
    t_req_e.clear();
    phases.clear();
    phase_data.clear();
#if BW_LIMIT_GRANULARITY > 1 || PREFETCH
    path_to_io.clear();
#endif
}

/**
 * @brief collects individual I/O operations
 *
 * @param b           [in] number of bytes transfered
 * @param ts          [in] start time of I/O operation
 * @param te          [in] end time of I/O operation
 * @param path        [in] file this I/O operation belongs to
 *
 * @details Adds IO operation to tracked data
 */
void IOdata::Add_IO_Act(long long b, double ts, double te) {
    std::lock_guard lock(phase_data_lock);
    Add_IO_Act_Impl(b, ts, te);
}

/**
 * @brief indicates that the phases starts. this function works for both async (actual and required) and sync I/O. 
 * 
 * @param t start time of I/O operation
 * @param b bytes transfered 
 * @param of offset
 * 
 * @details every rank doing I/O calls this function whener it start with the I/O operation. For the 
 * first time, a flag \e phase is set and the phase start. All I/O operations are counted part of the phase until 
 * \e Phase_End_Req is reached and the flag \e phase is unset. For the throughout, the end of the 
 * phase is indicated by \e Act_Done. 
 */
void IOdata::Phase_Start(bool condition, double t, long long b, long long of)
{
    std::lock_guard lock(phase_data_lock);
    //if first time, start phase
    if (!phase)
    {
        phase = true;
        phase_data.push_back(tmp);
        phase_data.back().t_start = t;

#if IODATA_VERBOSE >= 1
        printf("%s > rank %i %s> %s phase %li > start%s\n", caller, rank, CYAN, transaction_identifier, phase_data.size(), BLACK);
#endif
    }

    // add transfered bytes
    phase_data.back().data += b;
    // count I/O operations during phase
    phase_data.back().n_op += 1;
    // record current phase
    phases.push_back(phase_data.size());
    
    // record current offset
    //offset.push_back(of);


// #if IODATA_VERBOSE >= 2
//     printf("%s > rank %i > %s phase %li >> total %s bytes till now  %llu bytes\n", caller, rank, transaction_identifier, phase_data.size(), transaction_identifier, Sum<long long>("data"));
// #endif
}


/**
 * @brief Phase_End_Req: called when I/O operation reaches a wait call. Indicates 
 * that required phase ends (before wait). The data collected are store using the function @see Add_IO_Req
 * 
 * @param b transferd bytes by I/O operation
 * @param ts start time of I/O operation 
 * @param te end time of I/O oeration
 * @param path file of the I/O operation
 * 
 * @details \e Phase_Start_Req sets the flag \e phase to active during the first call. During the first call to this function 
 * the flag becomes false and the required phase ends. 
 */
void IOdata::Phase_End_Req(long long b, double ts, double te, [[maybe_unused]] const std::filesystem::path* path)
{

    std::lock_guard lock(phase_data_lock);
    //TODO: add flag to control granualaierty of sampling. Individual I/O operation can be discarded if focus is on phase (remove vectors)
    if (phase){
        phase_data.back().t_end_req = te;
        phase = false;

#if ONLINE == 1 
        //Average: bytes during phase divided by actual I/O time
        phase_data.back().B_avr = phase_data.back().data / (phase_data.back().t_end_req - phase_data.back().t_start);

#if IODATA_VERBOSE >= 1
        printf("%s > rank %i %s> %s phase %li > req phase over >> %.3f KB handled in %.5f sec --> B_avr(%li) = %.3f KB/s %s\n", caller, rank, CYAN, transaction_identifier, phase_data.size(), (double)phase_data.back().data/1000, phase_data.back().t_end_req - phase_data.back().t_start, phase_data.size(), phase_data.back().B_avr / 1000, BLACK);
#endif
#endif
    }
    // add required values to tracked data
    Add_IO_Req(b, ts, te, path);

//Sum: aggregegated bandwidth of individual I/O opertaions
#if ONLINE == 1 
    phase_data.back().B_sum  += bandwidth_req.back();    

#if IODATA_VERBOSE >= 3
    static int counter = 0; 
    counter+=1; 
    if (counter == phase_data.back().n_op){
        printf("%s > rank %i %s> %s phase %li > req phase over >> %.3f KB handled in %.5f sec --> B_sum(%li) = %.3f KB/s %s\n", caller, rank, BLUE, transaction_identifier, phase_data.size(), (double)phase_data.back().data/1000, phase_data.back().t_end_req - phase_data.back().t_start, phase_data.size(), phase_data.back().B_sum / 1000, BLACK);
        counter = 0;
    }
#endif
    
#endif
}


/**
 * @brief called when the actual phase ends. The end is either during tests (reqiuires MPI_test) or standard at the end of the Wait phase. 
 * The data collected are store using the function @see Add_IO_Act_Impl
 * 
 * @param b transferred bytes by I/O operation 
 * @param ts start time of I/O operation    
 * @param te end time of I/O operation
 * @param phase_condition condition indicating that the actual phase is over
 * @param path path of the file this I/O operation belongs to
 */
void IOdata::Phase_End_Act(long long b, double ts, double te, bool phase_condition)
{
    std::lock_guard lock(phase_data_lock);
    // [NOTE] Only true when all the Async I/O operations belong to this phase are over
    // if ONLINE == 1, calculate phase bandwidth
    if (phase_condition)
    {
        phase_data.back().t_end_act   = te;

#if ONLINE == 1
        // avr bytes during phase divided by actual I/O time
        phase_data.back().T_avr = phase_data.back().data/(phase_data.back().t_end_act - phase_data.back().t_start);

#if IODATA_VERBOSE >= 1
        printf("%s > rank %i %s> %s phase %li > act phase over >> %.3f KB handled in %.5f sec --> T_avr(%li) = %.3f MB/s %s\n", caller, rank, CYAN, transaction_identifier, phase_data.size(), (double)phase_data.back().data/1000, phase_data.back().t_end_act - phase_data.back().t_start, phase_data.size(), phase_data.back().T_avr / 1'000'000, BLACK);
#endif
#if IODATA_VERBOSE >= 3
        printf("%s > rank %i %s> %s phase %li > act phase over >> %.3f KB handled in %.5f sec --> T_sum(%li) = %.3f MB/s %s\n", caller, rank, BLUE, transaction_identifier, phase_data.size(), (double)phase_data.back().data/1000, phase_data.back().t_end_act - phase_data.back().t_start, phase_data.size(), phase_data.back().T_sum / 1'000'000, BLACK);
#endif
        
#endif
    }
    
    //TODO: flag to contol granualrtiy of sampling
    //add actual values to tracked data
    Add_IO_Act_Impl(b, ts, te);

//Sum: aggregegated bandwidth of individual I/O opertaions
#if ONLINE == 1 
    phase_data.back().T_sum  += bandwidth_act.back();    
#endif


}

//! ----------------------- Sync ------------------------------
//**********************************************************************
//*                       1. Lost_Time
//**********************************************************************
/**
 * @brief end of sync phase. Calculates throughput if ONLINE flag is passed (see \e ioflags.h)
 * 
 */
void IOdata::Phase_End_Sync(double t)
{
    std::lock_guard lock(phase_data_lock);
    // add phase info
    if (phase)
    {
        //FIXME: make this more dynamic. Average and sum are the same for sync. Sum is right, average is wrong. Also add online and offline
        phase = false;
        
        
        phase_data.back().t_end_act = t;
#if ONLINE == 1
        phase_data.back().T_avr     = phase_data.back().data / (phase_data.back().t_end_act - phase_data.back().t_start);
        phase_data.back().T_sum     = phase_data.back().T_avr;
#endif


#if IODATA_VERBOSE >= 1
        printf("%s > rank %i %s> %s phase %li > sync phase over >> %.3f KB handled in %.5f sec --> T_avr(%li) = %.3f KB/s %s\n", caller, rank, CYAN, transaction_identifier, phase_data.size(), (double)phase_data.back().data/1000, phase_data.back().t_end_act - phase_data.back().t_start, phase_data.size(), phase_data.back().T_avr / 1'000'000, BLACK);
#endif
    }
}


//! ----------------------- Additional function ------------------------------
//**********************************************************************
//*                       1. 
//**********************************************************************
/**
 * @brief 
 * end of sync phase. Calculates throughput if ONLINE flag is passed (see \e ioflags.h)
 * @brief sums \e specified field 
 * 
 * @return long long 
 */

template <class T>
T IOdata::Sum(std::string s)
{
    // calculates sum of transfered bytes over all I/O operations (not phase!)

    long long sum = 0;
    std::shared_lock lock(phase_data_lock);
    for (unsigned int i = 0; i < phase_data.size(); i++)
        sum += phase_data[i].get(s);
    

    return sum;
}
template long long IOdata::Sum<long long>(std::string);
template double IOdata::Sum<double>(std::string);


template <class T>
T IOdata::Max(std::vector<T> a)
{
    T max = 0;
    for (unsigned int i = 0; i < a.size(); i++)
        max = (max > a[i]) ? max : a[i];

    return max;
}
template long long IOdata::Max<long long>(std::vector<long long>);

IOdata::TransactionType IOdata::get_transaction_type() {
    return transaction_type;
}

bool IOdata::is_async() {
    return transaction_type == IOdata::TransactionType::Async_Read ||
            transaction_type == IOdata::TransactionType::Async_Write;
}

bool IOdata::is_write() {
    return transaction_type == IOdata::TransactionType::Sync_Write||
            transaction_type == IOdata::TransactionType::Async_Write;
}

double IOdata::get_last_phase_info(std::string info) {
    std::shared_lock lock(phase_data_lock);
    phase_data.empty()? -1 : phase_data.back().get(info);
}

void IOdata::set_last_phase_info(std::string info, double value) {
    std::lock_guard lock(phase_data_lock);
    if(!phase_data.empty())
        phase_data.back().set(info, value);
}

size_t IOdata::get_phase_count() {
    std::shared_lock lock(phase_data_lock);
    return phase_data.size();
}

std::vector<double> IOdata::get_bandwidth_act() {
    return bandwidth_act;
}

std::vector<double> IOdata::get_bandwidth_req() {
    std::shared_lock lock(phase_data_lock);
    return bandwidth_req;
}

std::vector<double> IOdata::get_t_act_s() {
    std::shared_lock lock(phase_data_lock);
    return t_act_s;
}

std::vector<double> IOdata::get_t_act_e() {
    std::shared_lock lock(phase_data_lock);
    return t_act_e;
}

std::vector<double> IOdata::get_t_req_s() {
    std::shared_lock lock(phase_data_lock);
    return t_req_s;
}

std::vector<double> IOdata::get_t_req_e() {
    std::shared_lock lock(phase_data_lock);
    return t_req_e;
}

void IOdata::gather_phase_data(MPI_Datatype GATHER_collect, collect* all_data, int* num_ops, int* displacement, MPI_Comm IO_WORLD) {
    std::shared_lock lock(phase_data_lock);
    MPI_Gatherv(&phase_data[0], phase_data.size(), GATHER_collect, all_data, num_ops, displacement, GATHER_collect, 0, IO_WORLD);
}

void IOdata::clear_phase_data() {
    std::lock_guard lock(phase_data_lock);
    phase_data.clear();
}

#if BW_LIMIT_GRANULARITY > 1
double IOdata::get_prev_file_bw(const std::filesystem::path& path)
{
    double res = -1.0;
    std::shared_lock lock(phase_data_lock);
    auto it = path_to_io.find(path);
    if (it != path_to_io.end()) {
        size_t index = it->second.back();
        res = bandwidth_req[index];
    }
    return res;
}
#endif

#if BW_LIMIT_GRANULARITY == 3
long long IOdata::get_prev_file_size(const std::filesystem::path& path)
{
    long long res = 0;
    std::shared_lock lock(phase_data_lock);
    auto it = path_to_io.find(path);
    if (it != path_to_io.end()) {
        size_t index = it->second.back();
        res = bytes[index];
    }
    return res;
}
#endif

long long IOdata::count_opertaions(long long a)
{
    long long counter = 0;

    std::shared_lock lock(phase_data_lock);
    for (unsigned int i = 0; i < phases.size(); i++)
    {
        if (phases[i] == a)
            counter++;
    }

    return counter;
}

long long IOdata::count_opertaions_agg(long long a)
{
    long long counter = 0;

    std::shared_lock lock(phase_data_lock);
    for (int i = 0; i < a; i++)
    {
            counter += phase_data[i].n_op;
    } 

    return counter;
}

/**
 *@brief recalculates the bandiwdth of the phase by considering true async I/O (all I/O operation happen at the same time in different threads)
 *       by recalculating the bandwidth with the collected values 
 * 
 */
void IOdata::Bandwidth_In_Phase_Offline(void)
{
    iohf::Function_Debug(__PRETTY_FUNCTION__);
    std::lock_guard lock(phase_data_lock);

    Debug_Info_Bandwidth_In_Phase();

    int counter = 0;
    int loops = phase_data.size();
    double tmp_act = 0;
    double tmp_req = 0;

    for (int i = 0; i < loops; i++)
    {

        if (counter == phases[i])
        {
            tmp_act += bandwidth_act[i];
            if (is_async())
                tmp_req += bandwidth_req[i];
        }
        if (i == loops - 1 || counter != phases[i + 1])
        {
            counter++;

            phase_data[i].T_sum = tmp_act;
            tmp_act = 0;

            if (is_async())
            {
                
                phase_data[i].B_sum = tmp_req;
                tmp_req = 0;
            }
        }
    }

    if (phases.size() > 0){
        for (int i = 0; i <= phases.back(); i++)
        {
            
            phase_data[i].T_avr = phase_data[i].data / (phase_data[i].t_end_act - phase_data[i].t_start);
            
            if (is_async()){
                phase_data[i].B_avr = phase_data[i].data / (phase_data[i].t_end_act - phase_data[i].t_start);
            }
        }
    }

}

void IOdata::Debug_Info_Bandwidth_In_Phase(void)
{
#if IODATA_VERBOSE >= 1
    printf("%s > rank %i %s> %s merge > merging individual bandwidths inside a phase to a single phase bandwidth. %s\n", caller, rank, CYAN, transaction_identifier, BLACK);
#endif
#if IODATA_VERBOSE >= 2
    printf("%s > rank %i %s> %s merge >> merging %li bandwidths to %li phase bandwidths %s\n", caller, rank, CYAN, transaction_identifier, bandwidth_req.size(), phase_data.size(), BLACK);
#endif
#if IODATA_VERBOSE >= 3
    printf("%s > rank %i %s> %s merge >>> act > %li -> %li %s\n", caller, rank, YELLOW, transaction_identifier, bandwidth_act.size(), phase_data.size(), BLACK);
    if (is_async())
        printf("%s > rank %i %s> %s merge >>> req > %li -> %li %s\n", caller, rank, YELLOW, transaction_identifier, bandwidth_req.size(), phase_data.size(), BLACK);
#endif
}

const char* IOdata::type_string(IOdata::TransactionType tt) {
    switch (tt)
    {
    case TransactionType::Async_Read:
        return "async read";
        break;
    case TransactionType::Async_Write:
        return "async write";
        break;
    case TransactionType::Sync_Read:
        return "sync read";
        break;
    case TransactionType::Sync_Write:
        return "sync write";
        break;
    }
}