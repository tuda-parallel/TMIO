#ifndef IOFLAGS 
#define IOFLAGS

//* ENABLE TRACING
//*******************************
#ifndef ENABLE_MPI_TRACE
#define ENABLE_MPI_TRACE 1 // set to 1 to enable tracing
// 0: disable tracing
// 1: enable tracing
#endif

#ifndef ENABLE_LIBC_TRACE
#define ENABLE_LIBC_TRACE 0 // set to 1 to enable tracing
// 0: disable tracing
// 1: enable tracing
#endif

#ifndef ENABLE_IOURING_TRACE
#define ENABLE_IOURING_TRACE 1 // set to 1 to enable tracing
// 0: disable tracing
// 1: enable tracing
#endif

//* DEBUG Flags
//*******************************
#ifndef DEBUG 
#define DEBUG 1 // set debug level for tmio.cxx
#endif

#ifndef IODATA_VERBOSE
#define IODATA_VERBOSE 0//set debug level for iodata.cxx
#endif

#ifndef IOTRACE_VERBOSE
#define IOTRACE_VERBOSE 0 //set debug level for iodata.cxx
enum class VerbosityLevel {
    NONE_LOG = 0,   // No logging
    BASIC_LOG = 1,  // Basic information
    DETAILED_LOG = 2, // More detailed information
    DEBUG_LOG = 3,  // Debug-level information
    TRACE_LOG = 4   // Very verbose, trace-level information
};

constexpr VerbosityLevel IOTRACE_VERBOSITY = static_cast<VerbosityLevel>(IOTRACE_VERBOSE);
#endif

#ifndef IOANALYSIS_VERBOSE
#define IOANALYSIS_VERBOSE 0 //set debug level for ioanalysis.cxx
#endif

#ifndef IOFLUSH_VERBOSE 
#define IOFLUSH_VERBOSE 0 //set debug flag for ioflush.cxx
#endif

#ifndef HDEBUG
#define HDEBUG 0 //controls debug of  hfunctions.cxx
#endif

#ifndef BW_LIMIT_VERBOSE
#define BW_LIMIT_VERBOSE 0 //controls debug of bw_limit in  bw_limit.cxx
#endif

// #define TIME_VERBOSE  //Trace time of rank 0 



//* Print Flags
//*******************************
#ifndef OVERLAP_GRAPH 
#define OVERLAP_GRAPH 0 // prints bandwidth overlap graph
#endif

#ifndef ALL_SAMPLES 
#define ALL_SAMPLES 5 // print samples (Phase bandwidth) to json file in ioprint.cxx
// 0: skips printing
// 1: prints all overlapping phase bandwidth/throughput across ranks (b_overlap_sum and b_overlap_avr)
// 2: 1 + prints act/req time when overlapping phases change (added or remove from stack) (t_overlap)
// 3: 2 + prints phase bandwidth and throughput of all ranks (B_sum, B_avr,T_avr, T_sum) 
// 4: 3 + prints start, act, and req time of phase bandwidth/throughput of all ranks (t_start and t_act for T_avr and t_start and t_req for B_sum)
// 5: 4 + prints single I/O operaitons of every rank over all phases (all_b all_t_req_s all_t_req_e and all_t, all_t_act_s, all_t_act_e)
#endif



//* Calculation Flags
//*******************************
#ifndef ONLINE
#define ONLINE 1 // in iodata.cxx and iotrace.cxx
// 0: offline -> calculate phase bandwidth at end
// 1: online  -> calculate phase bandwidth during runtime
#endif

#ifndef SAME_T_END // same end time for phase in iodata.cxx
#define SAME_T_END 1
#endif

#ifndef SKIP_LAST_WRITE
#define SKIP_LAST_WRITE 0 // skips n last writes from calulcation. Values are printed as NaN
#endif

#ifndef SKIP_LAST_READ
#define SKIP_LAST_READ 0 // skips n last reads from calulcation. Values are printed as NaN
#endif

#ifndef SKIP_FIRST_WRITE
#define SKIP_FIRST_WRITE 0 // skips n first writes from calulcation. Values are printed as NaN
#endif

#ifndef SKIP_FIRST_READ
#define SKIP_FIRST_READ 0 // skips n first reads from calulcation. Values are printed as NaN
#endif

#ifndef SHOW_AVR
#define SHOW_AVR 1 // shows average calculated when grouping the bandwidth over all ranks: overlaping bandwidths of ranks
#endif

#ifndef SHOW_SUM
#define SHOW_SUM 0 // shows sum calculated when grouping the throughput over all ranks: overlaping bandwidths of ranks
#endif

#ifndef SYNC_MODE // sets when the sync phase starts and ends for each rank
#define SYNC_MODE 0
// 0: sync phase starts and ends for each sync IO opertation
// 1: sync phase starts at first sync IO operation after the file is opened and ends when the file is closed
#endif

#ifndef FUNCTION_INFO
#define FUNCTION_INFO 3
// 0: No function tracing
// 1: Directly print the function tracing to stdout
// 2: Enhanced function tracing, which includes:
//    - Process ID and thread ID
//    - Some Function with also shows parameters
//    - Process-safe function tracing (i.e. clasified base on Rank)
//    - Print after TMIO running time profile is finished (i.e. after MPI_Finalize())
// 3: Main thread function tracing, which base on INFO level 2, while only printing the main thread function tracing
#endif

#ifndef OVERHEAD
#define OVERHEAD 1 // if set, overhead during the runtime of the application is additionally calculated to the overhead at the end of the application
#endif

#if !defined(TEST) && !defined(BW_LIMIT) && !defined(CUSTOM_MPI)
// if set the throughput is captured during the test phases. In case CUSTOM_MPI or BW_LIMIT are defined, the throughput 
// is obtained through the custome MPI library (see Bandwidth Limit)
#define TEST 1 
#else 
#define TEST 0
#endif

#ifndef DO_CALC
#define DO_CALC 0 // if set the 0 overlapping calculation is performed, only the data is collected
// DO_CALC is not supported in jsonl mode
#endif



//* DFT (Depreciated)
//*******************************
#ifndef DFT //TODO: do some kind of check for the flags
#define DFT 0 // if set, a DFT is performed on the phases (DO_CALC must be set to 1)
// 1: performs DFT and delivers sampled bandwidth (b_sampled), Amplitude (A), angle (phi), and settings (start/end time, N samples, T_s sample rate)
// 2: 1 + original signal ( b and t)
#endif

#ifndef DFT_TIME_WINDOW
#define DFT_TIME_WINDOW 1 //Todo: not implemented
// 0: standard mode considers the entire sampling time
// 1: Adaptive mode. Based on the dominant frequency, the time window is changed 
#endif

#ifndef FREQ
#define FREQ -1
// if DFT is active and this value is above 0, the DFT is performed with this sampling frequency
// specify negative value for auto detecting sampling frequency (lowest bandwidth change)
#endif

#ifndef FREQ_LIMIT
#define  FREQ_LIMIT 10
// set the limit for the frequency in case FREQ is set to a negative value
#endif


#ifndef CONFIDENCE_CHECK
#define CONFIDENCE_CHECK 0 // the signal is devided into equal sized chunks and the presence of the dominant frequency is examined
// if set to above 0, the confidence check is executed
#endif



//* Bandwidth Limit  
//*******************************
// Limits the BW. Needs the library to be compiled with the UC3M MPI version: /d/git/tarraf/bw_limit/mpich-4.0.3/mpich-bin/bin/mpicxx
// One of the bellow two needs to be enabled for. Set them in the Makefile
// 1. Limits the BW with the requirements. For that, define BW_LIMIT 
// > make library CXX_DEBUG+="-DBW_LIMIT" MPICXX=..
// 2. Overwrites throughput and duraion when mpich from UC3m is used (no limitation). For that, define CUSTOM_MPI 
// > make library CXX_DEBUG+="-DCUSTOM_MPI" MPICXX=..
// These options are only accessible through the Makefile to avoiud errors as a custom MPI version is needed.
//
// For sync limit, besides defining BW_LIMIT, the variables below need to be defined (in bytes/sec): 
#if defined BW_LIMIT
#ifndef BW_LIMIT_SYNC_READ
#define BW_LIMIT_SYNC_READ 0 
#endif
#ifndef BW_LIMIT_SYNC_WRITE
#define BW_LIMIT_SYNC_WRITE 1'000 
#endif
// Defines the Limiting strategy
#ifndef BW_LIMIT_STRATEGY
#define BW_LIMIT_STRATEGY 2 // 0: always (default) -- 1: increase only -- 2: limit the down side 
#endif
// Tolerance value to scale the desired values
#ifndef TOL
#define TOL 1.1
#endif
#endif



//* Output File     
//*******************************
#ifndef FILE_FORMAT
#define FILE_FORMAT 0
// 0 FILE_FORMAT "jsonl"
// 1 FILE_FORMAT "binary" 
// 2 FILE_FORMAT "msgpack" 
// 3 FILE_FORMAT "zmq" 
#endif




//* Other Settings
//*******************************
// openMP: 
//#define OPENMP

//colors output: 
#define COLOR_OUTPUT 


// * Batch IO Settings
#ifndef BATCH_LIO
#define BATCH_LIO 1
// 0: Track each sync IO operation separately
// 1: Batch all IO operations when call `lio_listio` with `LIO_WAIT` model
#endif

// * IO Before main() function
//*******************************
#ifndef IO_BEFORE_MAIN
#define IO_BEFORE_MAIN 0
// 0: No record IO before main function
// 1: Record IO before main function
#endif


#endif
