#ifndef IO_DATA_H
#define IO_DATA_H

#include <stdio.h>
#include <string.h>
#include <unordered_map>
#include <filesystem>
#include <shared_mutex>
#include <mutex>
#include "ioprint.h"

/**
 *  IO trace class
 * @file   iotrace.h
 * @author Ahmad Tarraf
 * @date   01.12.2021
 */

    /**
    * @class This is a class to collect all I/O related metrics. 
    * @brief IO class contaning data. This class collects the data for each rank. 
    * Async I/O have two vectors (required and actual) 
    * while sync I/O store all collected values in the actual vectors while the required remains empty. 
    * @see IO     : constructor. Initilizes all variables to 0
    * @see Summary: displays a summary of the results to the out stream.
    *
    */
class IOdata{
public:
    enum class TransactionType {
        Async_Write,
        Async_Read,
        Sync_Write,
        Sync_Read,
    };

private:
    
    //* Variables:
    //************
    int  rank;                // current rank
    TransactionType transaction_type;  // type of transaction this data belongs to
    mutable std::shared_mutex phase_data_lock; // FIX: More fine grained locking
    const char* transaction_identifier;
    
    //*******************************
    //* I/O information during phase
    //*******************************
    std:: vector<double>    bandwidth_act;  // bandwidth of individual I/O operations  
    std:: vector<double>    bandwidth_req;  // bandwidth of individual I/O operations  
    std:: vector<double>    t_act_s;  // actual start time 
    std:: vector<double>    t_act_e;  // actaul end time
    std:: vector<double>    t_req_s;  // required start time (usually same as t_act_s)
    std:: vector<double>    t_req_e;  // required end time
    std:: vector<long long> bytes;    // bytes transfered by the I/O operation
    std:: vector<int>       phases;   // phase the current I/O operation belongs to
#if BW_LIMIT_GRANULARITY > 1
    std:: unordered_map<std::filesystem::path, std::vector<size_t>> path_to_io;   // I/O operations for each file
#endif
    //*******************************
    //* Phase information 
    //*******************************   
    bool phase;
    std:: vector<collect> phase_data;
public:
    collect tmp;

    //* Methods:
    //************
    IOdata();
    void Mode(int,TransactionType); // set if read or write and if actual or required
    //? phase start
    void Phase_Start(bool,double,long long,long long );
    
    //? clear all I/O traces
    void Clear_IO(void);
    void Add_IO_Act(long long,double,double);
    
    //? for Async tracing 
    void Phase_End_Act(long long,double,double,bool);
    void Phase_End_Req(long long,double,double, const std::filesystem::path*);
    
    //? for Sync tracing 
    void Phase_End_Sync(double);

    //? statistics
    template <class T>
    T Sum(std::string);
    template <class T>
    T Max(std::vector<T>);

    std::vector<double> get_bandwidth_act();
    std::vector<double> get_bandwidth_req();
    std::vector<double> get_t_act_s();
    std::vector<double> get_t_act_e();
    std::vector<double> get_t_req_s();
    std::vector<double> get_t_req_e();
    
    void gather_phase_data(MPI_Datatype, collect*, int*, int*, MPI_Comm);
    void clear_phase_data();

    TransactionType get_transaction_type();
    const char* type_string(TransactionType);
    bool is_async();
    bool is_write();
    double get_last_phase_info(std::string);
    void set_last_phase_info(std::string, double);
    size_t get_phase_count();

#if BW_LIMIT_GRANULARITY > 1
    double get_prev_file_bw(const std::filesystem::path&);
#endif
#if BW_LIMIT_GRANULARITY == 3
    long long get_prev_file_size(const std::filesystem::path&);
#endif
    
    //? calucalte the Bandwidth after the application finishes
    void Bandwidth_In_Phase_Offline(void);

private: 
    char caller[12] = "\tIOdata "; // name of the class
    long long count_opertaions(long long); //counts operation in a phase
    long long count_opertaions_agg(long long);  //counts all operations bellow input
    long long online_counter;

    //? add I/O traces
    void Add_IO_Act_Impl(long long,double,double);
    void Add_IO_Req(long long,double,double, const std::filesystem::path*);

    //? Debug
    void Debug_Info_Bandwidth_In_Phase(void);
};

#endif // IO_DATA_H