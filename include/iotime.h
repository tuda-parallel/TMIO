#include "statistics.h"

/**
 * @class iotime
 * @brief trace all time information releated to I/O calls.
 * @note: All variables are aggreagated values accross all ranks except \e deta_t_rank0_vec
 */
class iotime
{

public:
	iotime(double*, double*, statistics, statistics, statistics, statistics);
	iotime(void);
	~iotime();
	void print(std::ofstream &);
	std::string Print_Json(bool jsonl = false);
	const char *Color_Percent(double);

#if FILE_FORMAT > 1
MSGPACK_DEFINE(
	name,
	delta_t_agg,
	delta_t_sr,
	delta_t_sw,
	delta_t_ara,
	delta_t_arr,
	delta_t_ar_lost,
	delta_t_awa,
	delta_t_awr,
	delta_t_aw_lost,
	delta_t_agg_io,
	delta_t_rank0,
	delta_t_rank0_app,
	delta_t_rank0_overhead_post_runtime,
	delta_t_rank0_overhead_peri_runtime,
	delta_t_overhead,
	delta_t_overhead_post_runtime,
	delta_t_overhead_peri_runtime,
	delta_t_overhead_dft);



#endif

private:
	double delta_t_sw;  // aggregated sync write time 
	double delta_t_sr;  // aggregated sync read time 
	double delta_t_awr; // aggregated required async write time 
	double delta_t_awa; // aggregated actual async write time 
	double delta_t_arr; // aggregated required async read time 
	double delta_t_ara; // aggregated actual async read time 

	// lost time
	double delta_t_ar_lost; // aggregated lost read time
	double delta_t_aw_lost; // aggregated lost write time

	double delta_t_agg;	   // aggregated time accross all processes
	double delta_t_agg_io; // aggregated I/O time
	double* deta_t_rank0_vec; // ellapsed time for rank 0 (runtime, overhead during and overhead after run)

	double delta_t_overhead;    // aggregated overhead of tracing library
	double delta_t_overhead_post_runtime; // aggregated overhead after application ends
	double delta_t_overhead_peri_runtime;      // aggregated overhead during the runtime of the application
	double delta_t_overhead_dft;

	// rank 0 times
	double delta_t_rank0; // deta_t_rank0_vec[0]+deta_t_rank0_vec[2] 
	double delta_t_rank0_app; // deta_t_rank0_vec[0]-deta_t_rank0_vec[1]
	double delta_t_rank0_overhead_post_runtime; //deta_t_rank0_vec[2]
	double delta_t_rank0_overhead_peri_runtime; // line_start,deta_t_rank0_vec[1]

	std::string name = "io_time";
};
