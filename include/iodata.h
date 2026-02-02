#ifndef IO_DATA_H
#define IO_DATA_H

#include <stdio.h>
#include <string.h>
#include <unordered_map>
#include <filesystem>
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
    enum class Transaction_Type {
        Async_Write,
        Async_Read,
        Sync_Write,
        Sync_Read,
    };

    //* Variables:
    //************
    int  rank;                // current rank
    bool phase;
    bool a_or_s_flag; // true = async | false = sync         
    bool w_or_r_flag; // true = write | false = read         

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
    std:: vector<collect>   phase_data;
    collect tmp;
    
    //* Methods:
    //************
    IOdata();
    void Mode(int,bool,bool=true); // set if read or write and if actual or required
    //? phase start
    void Phase_Start(bool, double,long long,long long );
    
    //? add I/O tracr or claer all I/O traces
    void Add_IO_Act(long long,double,double);
    void Add_IO_Req(long long,double,double, const std::filesystem::path*);
    void Clear_IO(void);
    
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

#if BW_LIMIT_GRANULARITY > 1
    double get_prev_file_bw(const std::filesystem::path&);
#endif
#if BW_LIMIT_GRANULARITY == 3
    long long get_prev_file_size(const std::filesystem::path&);
#endif
    
    //? calucalte the Bandwidth after the application finishes
    void Bandwidth_In_Phase_Offline(void);
    
    //? Debug
    void Debug_Info_Bandwidth_In_Phase(void);


private: 
    char caller[12] = "\tIOdata "; // name of the class
    char w_or_r[6];   // write or read
    char a_or_s[6];   // async or sync
    long long count_opertaions(long long); //counts operation in a phase
    long long count_opertaions_agg(long long);  //counts all operations bellow input
    long long online_counter;
};

#endif // IO_DATA_H