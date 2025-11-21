#include "iodata.h"

IOdata::IOdata(void): phase(false)
{
}

void IOdata::Mode(int r, bool a, bool b)
{
    // true = write || false = read
    w_or_r_flag = a;
    if (a)
        strcpy(w_or_r, "write");
    else
        strcpy(w_or_r, "read");

    // true = async || false = sync
    a_or_s_flag = b;
    if (b)
        strcpy(a_or_s, "async");
    else
        strcpy(a_or_s, "sync");

    phase = false;
    rank  = r;
    
    #if ONLINE == 1
    online_counter = 0;
    #endif

#if IODATA_VERBOSE >= 2
    printf("%s > rank %i %s> collecting data for %s %s%s\n", caller, rank, CYAN, w_or_r, a_or_s, BLACK);
#endif
}

/**
 * @brief collects individual I/O operations
 *
 * @param req_or_act  [in] 1 for required || 0 for actual
 * @param b           [in] number of bytes transfered
 * @param ts          [in] start time of I/O operation
 * @param te          [in] end time of I/O operation
 *
 * @details Adds IO operation to tracked data
 */
void IOdata::Add_Io(bool req_or_act, long long b, double ts, double te)
{

    if (req_or_act)
    {
#if SAME_T_END == 1
        bandwidth_req.push_back(b / (phase_data.back().t_end_req - ts));
#else
        bandwidth_req.push_back(b / (te - ts));
#endif

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
        printf("%s > rank %i %s> %s %s phase %li > #%lli > req over: %.3f KB handled in %f s -> B(%li,%lli) = %.3f KB/s%s\n", caller, rank, CYAN, a_or_s, w_or_r, phase_data.size(), bandwidth_req.size() - count_opertaions_agg(phase_data.size() - 1), (double)b / 1000, phase_data.back().t_end_req - ts, phase_data.size(), bandwidth_req.size() - count_opertaions_agg(phase_data.size() - 1), (double)bandwidth_req.back() / 1000, BLACK);
#else
        printf("%s > rank %i %s> %s %s phase %li > #%lli > req over: %.3f KB handled in %f s -> B(%li,%lli) = %.3f KB/s%s\n", caller, rank, CYAN, a_or_s, w_or_r, phase_data.size(), bandwidth_req.size() - count_opertaions_agg(phase_data.size() - 1), (double)b / 1000, te - ts, phase_data.size(), bandwidth_req.size() - count_opertaions_agg(phase_data.size() - 1), (double)bandwidth_req.back() / 1000, BLACK);
#endif
#endif
#if IODATA_VERBOSE >= 2
#if SAME_T_END == 1
        printf("%s > rank %i %s> %s %s phase %li > #%lli >> opertation from %f -> %f %s\n", caller, rank, YELLOW, a_or_s, w_or_r, phase_data.size(), bandwidth_req.size() - count_opertaions_agg(phase_data.size() - 1), ts, phase_data.back().t_end_req, BLACK);
#else
        printf("%s > rank %i %s> %s %s phase %li > #%lli >> opertation from %f -> %f %s\n", caller, rank, YELLOW, a_or_s, w_or_r, phase_data.size(), bandwidth_req.size() - count_opertaions_agg(phase_data.size() - 1), ts, te, BLACK);
#endif
#endif
    }
    else
    {
        bandwidth_act.push_back(b / (te - ts));
#if ALL_SAMPLES > 4
        t_act_s.push_back(ts);
        t_act_e.push_back(te);
#endif

#if IODATA_VERBOSE >= 1
        printf("%s > rank %i %s> %s %s phase %li > #%lli > act over: %.3f KB handled in %f s -> T(%li,%lli) = %.3f KB/s%s\n", caller, rank, CYAN, a_or_s, w_or_r, phase_data.size(), bandwidth_act.size() - count_opertaions_agg(phase_data.size() - 1), (double)b / 1000, te - ts, phase_data.size(), bandwidth_act.size() - count_opertaions_agg(phase_data.size() - 1), (double)bandwidth_act.back() / 1000, BLACK);
#endif
#if IODATA_VERBOSE >= 2
        printf("%s > rank %i %s> %s %s phase %li > #%lli >> opertation from %f -> %f %s\n", caller, rank, YELLOW, a_or_s, w_or_r, phase_data.size(), bandwidth_act.size() - count_opertaions_agg(phase_data.size() - 1), ts, te, BLACK);
#endif
    }
}




/**
 *
 * @details Remove all data traced so far
 */
void IOdata::Clear_IO(void)
{
    bandwidth_act.clear();
    bandwidth_req.clear();
    t_act_s.clear();
    t_act_e.clear();
    t_req_s.clear();
    t_req_e.clear();
    phases.clear();
    phase_data.clear();
}


/**
 * @brief indicates that the phases starts. this function works for both async (actual and required) and sync I/O. 
 * 
 * @param condition 
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
    //if first time, start phase
    if (condition)
    {
        phase = true;
        phase_data.push_back(tmp);
        phase_data.back().t_start = t;

#if IODATA_VERBOSE >= 1
        printf("%s > rank %i %s> %s %s phase %li > start%s\n", caller, rank, CYAN, a_or_s, w_or_r, phase_data.size(), BLACK);
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
//     printf("%s > rank %i > %s phase %li >> total %s bytes till now  %llu bytes\n", caller, rank, w_or_r, phase_data.size(), w_or_r,Sum<long long>("data"));
// #endif
}


/**
 * @brief Phase_End_Req: called when I/O operation reaches a wait call. Indicates 
 * that required phase ends (before wait). The data collected are store using the function @see Add_Io
 * 
 * @param b transferd bytes by I/O operation
 * @param ts start time of I/O operation 
 * @param te end time of I/O oeration
 *
 * @details \e Phase_Start_Req sets the flag \e phase to active during the first call. During the first call to this function 
 * the flag becomes false and the required phase ends. 
 */
void IOdata::Phase_End_Req(long long b, double ts, double te)
{
    //TODO: add flag to control granualaierty of sampling. Individual I/O operation can be discarded if focus is on phase (remove vectors)
    if (phase){
        phase_data.back().t_end_req = te;
        phase = false;

#if ONLINE == 1 
        //Average: bytes during phase divided by actual I/O time
        phase_data.back().B_avr = phase_data.back().data / (phase_data.back().t_end_req - phase_data.back().t_start);

#if IODATA_VERBOSE >= 1
        printf("%s > rank %i %s> %s %s phase %li > req phase over >> %.3f KB handled in %.5f sec --> B_avr(%li) = %.3f KB/s %s\n", caller, rank, CYAN, a_or_s, w_or_r, phase_data.size(), (double)phase_data.back().data/1000, phase_data.back().t_end_req - phase_data.back().t_start, phase_data.size(), phase_data.back().B_avr / 1000, BLACK);
#endif

        
#endif
    }

    // add required values to tracked data
    Add_Io(1, b, ts, te);

//Sum: aggregegated bandwidth of individual I/O opertaions
#if ONLINE == 1 
    phase_data.back().B_sum  += bandwidth_req.back();    

#if IODATA_VERBOSE >= 3
    static int counter = 0; 
    counter+=1; 
    if (counter == phase_data.back().n_op){
        printf("%s > rank %i %s> %s %s phase %li > req phase over >> %.3f KB handled in %.5f sec --> B_sum(%li) = %.3f KB/s %s\n", caller, rank, BLUE, a_or_s, w_or_r, phase_data.size(), (double)phase_data.back().data/1000, phase_data.back().t_end_req - phase_data.back().t_start, phase_data.size(), phase_data.back().B_sum / 1000, BLACK);
        counter = 0;
    }
#endif
    
#endif
}


/**
 * @brief called when the actual phase ends. The end is either during tests (reqiuires MPI_test) or standard at the end of the Wait phase. 
 * The data collected are store using the function @see Add_Io
 * 
 * @param b transferred bytes by I/O operation 
 * @param ts start time of I/O operation    
 * @param te end time of I/O operation
 * @param phase_condition condition indicating that the actual phase is over
 */
void IOdata::Phase_End_Act(long long b, double ts, double te, bool phase_condition)
{

    // if ONLINE == 1, calculate phase bandwidth
    if (phase_condition)
    {
        phase_data.back().t_end_act   = te;

#if ONLINE == 1
        // avr bytes during phase divided by actual I/O time
        phase_data.back().T_avr = phase_data.back().data/(phase_data.back().t_end_act - phase_data.back().t_start);

#if IODATA_VERBOSE >= 1
        printf("%s > rank %i %s> %s %s phase %li > act phase over >> %.3f KB handled in %.5f sec --> T_avr(%li) = %.3f MB/s %s\n", caller, rank, CYAN, a_or_s, w_or_r, phase_data.size(), (double)phase_data.back().data/1000, phase_data.back().t_end_act - phase_data.back().t_start, phase_data.size(), phase_data.back().T_avr / 1'000'000, BLACK);
#endif
#if IODATA_VERBOSE >= 3
        printf("%s > rank %i %s> %s %s phase %li > act phase over >> %.3f KB handled in %.5f sec --> T_sum(%li) = %.3f MB/s %s\n", caller, rank, BLUE, a_or_s, w_or_r, phase_data.size(), (double)phase_data.back().data/1000, phase_data.back().t_end_act - phase_data.back().t_start, phase_data.size(), phase_data.back().T_sum / 1'000'000, BLACK);
#endif
        
#endif
    }
    
    //TODO: flag to contol granualrtiy of sampling
    //add actual values to tracked data
    Add_Io(0, b, ts, te);

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
        printf("%s > rank %i %s> %s %s phase %li > sync phase over >> %.3f KB handled in %.5f sec --> T_avr(%li) = %.3f KB/s %s\n", caller, rank, CYAN, a_or_s, w_or_r, phase_data.size(), (double)phase_data.back().data/1000, phase_data.back().t_end_act - phase_data.back().t_start, phase_data.size(), phase_data.back().T_avr / 1'000'000, BLACK);
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

long long IOdata::count_opertaions(long long a)
{
    long long counter = 0;

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
            if (a_or_s_flag)
                tmp_req += bandwidth_req[i];
        }
        if (i == loops - 1 || counter != phases[i + 1])
        {
            counter++;

            phase_data[i].T_sum = tmp_act;
            tmp_act = 0;

            if (a_or_s_flag)
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
        
        if (a_or_s_flag){
            phase_data[i].B_avr = phase_data[i].data / (phase_data[i].t_end_act - phase_data[i].t_start);
        }
    }
}

}

void IOdata::Debug_Info_Bandwidth_In_Phase(void)
{
#if IODATA_VERBOSE >= 1
    printf("%s > rank %i %s> %s %s merge > merging individual bandwidths inside a phase to a single phase bandwidth. %s\n", caller, rank, CYAN, a_or_s, w_or_r, BLACK);
#endif
#if IODATA_VERBOSE >= 2
    printf("%s > rank %i %s> %s %s merge >> merging %li bandwidths to %li phase bandwidths %s\n", caller, rank, CYAN, a_or_s, w_or_r, bandwidth_req.size(), phase_data.size(), BLACK);
#endif
#if IODATA_VERBOSE >= 3
    printf("%s > rank %i %s> %s %s merge >>> act > %li -> %li %s\n", caller, rank, YELLOW, a_or_s, w_or_r, bandwidth_act.size(), phase_data.size(), BLACK);
    if (a_or_s_flag)
        printf("%s > rank %i %s> %s %s merge >>> req > %li -> %li %s\n", caller, rank, YELLOW, a_or_s, w_or_r, bandwidth_req.size(), phase_data.size(), BLACK);
#endif
}