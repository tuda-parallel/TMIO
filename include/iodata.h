#include <stdio.h>
#include <string.h>
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
    std:: vector<int>       phases;   // phase the current I/O operation belongs to
    
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
    void Add_Io(bool,long long,double,double);
    void Clear_IO(void);
    
    //? for Async tracing 
    void Phase_End_Req(long long,double,double);
    void Phase_End_Act(long long,double,double,bool);
    
    //? for Sync tracing 
    void Phase_End_Sync(double);

    //? statistics
    template <class T>
    T Sum(std::string);
    template <class T>
    T Max(std::vector<T>);
    
    //? calculate the Bandwidth after the application finishes
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
