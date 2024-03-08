#include "freq_analysis.h"


/**
 *  IO trace class
 * @file   iotrace.h
 * @author Ahmad Tarraf
 * @date   05.08.2021
 */

/**
 * @brief class that captures statistics. Used to capture sync and async write/read. For async, bandwidth and throughput need an instance
 * 
 * @param req_or_act describes if the bandwidth (req) or the throughput (act) is captured by an instance of this calss
 */
class statistics{
    
    public:

    bool flag_req; // flag indicating if requirment should be calculated   
    bool w_or_r;
        
    //metrics calculation
    iometrics throughput; // throughtput iometrics
    iometrics bandwidth;  // bandwidth metrics 

    //? data from phases (rank level)
    collect* all_data;

    
	//? Phase overlap (app level) (only if calc is on)
    double* throughput_avr_phase; // throughput for every overlapping phase 
    double* throughput_sum_phase;
    double* bandwidth_avr_phase; // bandwidth for every overlapping phase 
    double* bandwidth_sum_phase;
    int*  phases_of_ranks; // vector containing number of phases each rank had
    int*    n_overlap_act; // number overall overlap accros different phases
    int*    n_overlap_req; // number overall overlap accros different phases
    std::vector<std::vector<int>> phase_overlap_act; // stores the overlaping index 
    std::vector<double> phase_time_act; //store the time intervals
    std::vector<std::vector<int>> phase_overlap_req; // stores the overlaping index 
    std::vector<double> phase_time_req; //store the time intervals
    


    //? arrays for ind I/O data
    double *all_b = NULL; // for individual bandwidth of every IO operation
    double *all_t = NULL; // for individual throughput of every IO operation
    double *all_t_act_s = NULL;
    double *all_t_act_e = NULL;
    double *all_t_req_s = NULL;
    double *all_t_req_e = NULL;



    //* Metrics common between throughput and bandiwdth
    // number of ranks
    int     procs_io; // only procs that did io
    int     procs;    // all procs
    
    // dft overhead
    double dft_time;

    //? phase info 
    int          max_phases; //max number of phases a rank had 
    int          agg_phases; // total aggregated phases of all ranks
    long long    max_ops;    //max operations a rank had during a phase
    long long    max_ops_rank;    //max io operations a rank had during the entire time
    long long    agg_ops;  // aggregated io operations over all ranks     
    
    //? bytes info 
    long long   max_bytes;        // max bytes transferred by a rank
    long long   max_bytes_phase; // max bytes transferred during a phase
    long long   agg_bytes;        // aggregated bytes over entire application       

    

    #if FILE_FORMAT > 1
    void msgpack_pack(msgpack::packer<msgpack::sbuffer>& pk) const;
    #endif

    //! Functions
    //!-------------
    statistics();
    statistics(collect* ,int *, int, int, bool flag=false, bool flag2=false);
    ~statistics();
    
    //? Phase detection
    void Remove_Phase(int s = 0, int e = 0);
    void Overlap(std::vector<std::vector<int>>& , std::vector<double>& , std::string mode);
    double *Phase_Bandwidth(std::vector<std::vector<int>> &,std::string);
    void Phase_Detection(void);
    void Gather_Ind_Bandwidth(int, int, std::vector<double> ,std::vector<double>,std::vector<double> ,std::vector<double>,std::vector<double> ,std::vector<double> , MPI_Comm);
    
    //? compute metrics
    void Compute(void);
    void Compute_Metrics(void);
    void Compute_App_Metrics(void);
    void Compute_Rank_Metrics(void);
    void Compute_Rank_Metrics_Core(bool,bool);
    void Clean(void);

    //? Time 
    double Lost_Time();
	double Total_Time(std::string mode="t_end_act");
    
    
};
    
