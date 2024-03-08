#include <iostream>
#include <string.h>
#if FILE_FORMAT > 1
#include "msgpack.hpp"
#endif
/**
 * @file iocollect.h
 * @brief 
 * @details 
 * @author Ahmad Tarraf
 * @date 23.06.2022
 */


/**
 * @brief structured used to call all phase information of a rank
 * 
 * @param data sum of data during the phase
 * @param t_start start of the I/O phase
 * @param t_end_act actual end of the I/O phase
 * @param t_end_req required end of the I/O phase
 * @param T_sum phase throughput: sum of all bandwidths during phase
 * @param T_avr phase throughput: average of bandwidths during phase
 * @param B_sum phase bandwidth: sum of all bandwidths during phase
 * @param B_avr phase bandwidth: average of bandwidths during phase
 * @param n_op number of io operations during phase
 */
class collect{
    public:
    long long data   = 0;
    double t_start   = 0;
    double t_end_act = 0;
    double t_end_req = 0;
    double T_sum     = 0;
    double T_avr     = 0;
    double B_sum     = 0;
    double B_avr     = 0;
    int    n_op      = 0;
    
	
    collect(void);
    double get(std::string) const;
    void set(std::string mode, double value);

	#if FILE_FORMAT > 1
		MSGPACK_DEFINE(data, t_start, t_end_act, t_end_req, T_sum, T_avr, B_sum, B_avr, n_op);
	#endif
};