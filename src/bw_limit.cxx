#include "bw_limit.h"

#include <algorithm>


#if defined CUSTOM_MPI || defined BW_LIMIT
extern long int EMPI_DATA_READ;
extern long int EMPI_DATA_IREAD;
extern long int EMPI_DATA_WRITE;
extern long int EMPI_DATA_IWRITE;
extern long int EMPI_UTIME_READ;
extern long int EMPI_UTIME_IREAD;
extern long int EMPI_UTIME_WRITE;
extern long int EMPI_UTIME_IWRITE;
#endif 

#if defined BW_LIMIT
extern long int EMPI_IOBLOCK;
extern long double EMPI_DESIRED_BW_READ;
extern long double EMPI_DESIRED_BW_IREAD;
extern long double EMPI_DESIRED_BW_WRITE;
extern long double EMPI_DESIRED_BW_IWRITE;
extern long double EMPI_SCALE_BW_READ;
extern long double EMPI_SCALE_BW_IREAD;
extern long double EMPI_SCALE_BW_WRITE;
extern long double EMPI_SCALE_BW_IWRITE;
extern int EMPI_WORLD_RANK;
extern int EMPI_WORLD_SIZE;
#endif

Bw_limit::Bw_limit()
{
	counter_read = 0;
	counter_write = 0;
	counter_iread = 0;
	counter_iwrite = 0;
}

Bw_limit::~Bw_limit()
{
}

/**
 * @brief Prints info about the current bandwdith limiting strategy if set (in this case the throughput is always
 * captured through the custome_MPI implementation). If the throughout
 * is captured through the custome_MPI implementation, but the Bandwidth limiting strategy is off,
 * this info is displayed.
 * @return Info string
 */
std::string Bw_limit::Info(void) const
{

	char info[150];
#if defined BW_LIMIT
#if BW_LIMIT_STRATEGY == 1 || BW_LIMIT_STRATEGY == 2 || BW_LIMIT_STRATEGY == 0
	sprintf(info, "BW Limit : 1 \n"
	"TOL      : %.2f\n"
	"Strategy : %i\n", TOL, BW_LIMIT_STRATEGY);
#else
	sprintf(info, "BW Limit : 1 \n"
	"TOL      : %.2f)\n"
	"Strategy : 0 \n", TOL);
#endif

#elif defined CUSTOM_MPI
	sprintf(info, "BW Limit : 0 \n"
	"Custom MPI : 1 \n");
#endif
	return info;
}

/**
 * @brief Resets the variables that count how many operations of a specific type (async/sync read/write) have
 * occured so far. This information is avialble for every rank.
 */
void Bw_limit::Reset(void)
{
	counter_iread = 0;
	counter_iwrite = 0;
	counter_read = 0;
	counter_write = 0;
}

//************************************************************************************
//*                               1. get_last_phase_info
//************************************************************************************
/**
 * @brief Get phase info from current or most recent phase
 * @param mode [in] Type of traces to fetch
 * @param info [in] info needs to be in the form of iocollect: "t_start", "t_end_act", "t_end_req", "T_sum", "T_avr", "B_sum", "B_avr"
 * @return Requested phase info
 */
std::optional<double> Bw_limit::get_last_phase_info(TransactionType mode, std::string info) const
{
	std::optional<double> res;
	switch (mode)
	{
		case TransactionType::Async_Write:
			res = p_aw->get_last_phase_info(info);
			break;
		case TransactionType::Async_Read:
			res = p_ar->get_last_phase_info(info);
			break;
		case TransactionType::Sync_Read:
			res = p_sr->get_last_phase_info(info);
			break;
		case TransactionType::Sync_Write:
			res = p_sw->get_last_phase_info(info);
			break;
	}
	return res;
}

//************************************************************************************
//*                               2. set_phase_info
//************************************************************************************
/**
 * @brief Assign phase info for current or most recent phase
 *
 * @param mode [in] Type of traces to modify
 * @param info [in] Info needs to be in the form of iocollect: "t_start", "t_end_act", "t_end_req", "T_sum", "T_avr", "B_sum", "B_avr"
 * @param value [in] Value assigned to the variable
 */
void Bw_limit::set_phase_info(TransactionType mode, std::string info, double value)
{
	switch (mode)
	{
		case TransactionType::Async_Write:
			p_aw->set_last_phase_info(info, value);
			break;
		case TransactionType::Async_Read:
			p_ar->set_last_phase_info(info, value);
			break;
		case TransactionType::Sync_Read:
			p_sr->set_last_phase_info(info, value);
			break;
		case TransactionType::Sync_Write:
			p_sw->set_last_phase_info(info, value);
			break;
	}
}

//************************************************************************************
//*                               1. Init
//************************************************************************************
/**
 * @brief Setup bw limit
 * @param rank [in] Current MPI rank
 * @param processes [in] Total mpi ranks
 * @param p_aw [in] Pointer to async write trace data
 * @param p_ar [in] Pointer to async read trace data
 * @param p_sw [in] Pointer to sync write trace data
 * @param p_sr [in] Pointer to sync read trace data
 */
void Bw_limit::Init(int rank, int processes, IOdata *p_aw, IOdata *p_ar, IOdata *p_sw, IOdata *p_sr)
{
	std::lock_guard lock(bw_lock);

	this->p_aw = p_aw;
	this->p_ar = p_ar;
	this->p_sw = p_sw;
	this->p_sr = p_sr;
	this->rank = rank;
	this->processes = processes;

#if defined CUSTOM_MPI || defined BW_LIMIT
	EMPI_DATA_WRITE = 0;
	EMPI_DATA_IWRITE = 0;
	EMPI_DATA_READ = 0;
	EMPI_DATA_IREAD = 0;
#endif 

#if BW_LIMIT_FREQ == 1
	ftio_phase_pred = -1.0;
#endif

#ifdef BW_LIMIT
	EMPI_IOBLOCK = 200000;

	EMPI_DESIRED_BW_READ = 0;  // BW_LIMIT_SYNC_READ;
	EMPI_DESIRED_BW_WRITE = 0; // BW_LIMIT_SYNC_WRITE;
	EMPI_DESIRED_BW_IREAD = 0;
	EMPI_DESIRED_BW_IWRITE = 0;

	EMPI_SCALE_BW_IREAD = 1;
	EMPI_SCALE_BW_IWRITE = 1;
	EMPI_SCALE_BW_READ = 1;
	EMPI_SCALE_BW_WRITE = 1;
	EMPI_WORLD_RANK = rank;
	EMPI_WORLD_SIZE = processes;

	scale_bw_write = 1.0;
	scale_bw_read = 1.0;
	scale_bw_iwrite = 1.0;
	scale_bw_iread = 1.0;

	total_file_limit_read = 0.0;
	total_file_limit_write = 0.0;
	last_phase_read_bw = 0.0;
	last_phase_write_bw = 0.0;

	bw_limit_iwrite = EMPI_DESIRED_BW_IWRITE;
	bw_limit_iread = EMPI_DESIRED_BW_IREAD;
	Bw = EMPI_DESIRED_BW_WRITE;
	Br = EMPI_DESIRED_BW_READ;
#endif
}

#if BW_LIMIT_GRANULARITY > 1
/**
 * @brief Update bandwidth limit on file basis
 * @param write [in] Is file access a write access
 * @param path [in, optional] Path associated with file
 * @param transaction_size [in, optional] Number of transmitted bytes
 */
void Bw_limit::limit_by_file(bool write, [[maybe_unused]] const std::optional<PathID> path, [[maybe_unused]] long long transaction_size)
{
	std::lock_guard lock(bw_lock);

	if (write && !p_aw->phase_active()) {
		set_throughput_impl(TransactionType::Async_Write);
		Bw_limit::set_phase_info(TransactionType::Async_Write, "B_avr", bw_limit_iwrite);
		last_phase_write_bw = bw_limit_iwrite;
		bw_limit_iwrite = 0.0;
		total_file_limit_write = 0.0;
		EMPI_DESIRED_BW_IWRITE = 0.0;
	}
	
	if (!write && !p_ar->phase_active()) {
		set_throughput_impl(TransactionType::Async_Read);
		Bw_limit::set_phase_info(TransactionType::Async_Read, "B_avr", bw_limit_iread);
		last_phase_read_bw = bw_limit_iread;
		bw_limit_iread = 0.0;
		total_file_limit_read = 0.0;
		EMPI_DESIRED_BW_IREAD = 0.0;
	}

	double file_measured_bw = 0.0;

	TransactionType transaction = write? TransactionType::Async_Write : TransactionType::Async_Read;
	IOdata* p_data = write? p_aw : p_ar;

	#if BW_LIMIT_GRANULARITY == 2
		auto phase_duration = p_data->get_last_phase_duration();
	#if BW_LIMIT_FREQ == 1
		if(ftio_phase_pred < 0.0) {
			if(!phase_duration.has_value()) return;
			file_measured_bw = transaction_size / phase_duration.value();
		} else {
			Bw_limit::Log<VerbosityLevel::DETAILED_LOG>("Utilizing phase infromation for bandwidth\n");
			file_measured_bw = transaction_size / ftio_phase_pred; 
		}
	#else
		if(!phase_duration.has_value()) return;
			file_measured_bw = transaction_size / phase_duration.value();
	#endif
	#endif

	#if BW_LIMIT_GRANULARITY > 2
	long long prev_transaction_size = 0;

	if(path) {
		p_data->get_prev_phase_file_stats(*path, prev_transaction_size, file_measured_bw);
	}

	if (file_measured_bw <= 0.0) {
		Bw_limit::Log<VerbosityLevel::BASIC_LOG>("No previous bandwidth, falling back to phase BW\n");
		auto phase_duration = p_data->get_last_phase_duration();
		#if BW_LIMIT_FREQ == 1
		if(ftio_phase_pred < 0.0) {
			if(!phase_duration.has_value()) return;
			file_measured_bw = transaction_size / phase_duration.value();
		} else {
			Bw_limit::Log<VerbosityLevel::DETAILED_LOG>("Utilizing phase infromation for bandwidth\n");
			file_measured_bw = transaction_size / ftio_phase_pred; 
		}
		#else
		if(!phase_duration.has_value()) return;
			file_measured_bw = transaction_size / phase_duration.value();
		#endif
	}
	#endif

	Bw_limit::Log<VerbosityLevel::BASIC_LOG>("%s > rank %i / %i > %s%s %s> file BW %.2f Mb/s %s\n",
					caller, rank, processes - 1, YELLOW,  write? "async_write" : "async_read", BLUE, file_measured_bw / 1'000'000, BLACK);

	#if BW_LIMIT_GRANULARITY == 4
		double scale_factor = 1.0;

		if(prev_transaction_size > 0) {
			scale_factor = static_cast<double>(transaction_size) / static_cast<double>(prev_transaction_size);

			double scaled_limit = scale_factor * file_measured_bw;

			Bw_limit::Log<VerbosityLevel::BASIC_LOG>("%s > rank %i / %i > %s%s %s> BW scale factor: %.2f   prev: %lld Mb   cur: %lld Mb   BW new scaled goal %.2f Mb/s%s\n",
				caller, rank, processes - 1, YELLOW,  write? "async_write" : "async_read", BLUE,
				scale_factor, prev_transaction_size / 1'000'000, transaction_size / 1'000'000, scaled_limit / 1'000'000, BLACK);

			file_measured_bw = scaled_limit;
		} else {
			Bw_limit::Log<VerbosityLevel::BASIC_LOG>("No previous transaction, no scaling applied");
		}
		
	#endif

	if(transaction == TransactionType::Async_Write) {
		total_file_limit_write += file_measured_bw;
		bw_limit_iwrite = Bw_limit::calculate_bw_limit(total_file_limit_write, last_phase_write_bw);
		EMPI_DESIRED_BW_IWRITE = bw_limit_iwrite;
	} else {
		total_file_limit_read += file_measured_bw;
		bw_limit_iread = Bw_limit::calculate_bw_limit(total_file_limit_read, last_phase_read_bw);
		EMPI_DESIRED_BW_IREAD = bw_limit_iread;
	}

	Bw_limit::Log<VerbosityLevel::BASIC_LOG>("%s > rank %i / %i > %s%s %s> BW overall: %.2f Mb/s %s\n",
					caller, rank, processes - 1, YELLOW,  write? "async_write" : "async_read", BLUE, (write? bw_limit_iwrite : bw_limit_iread) / 1'000'000, BLACK);
}

#endif
#ifdef BW_LIMIT
/**
 * @brief Limit async write bandwidth based on known duration
 * @param transaction_size [in] Number of transmitted bytes
 * @param duration_sec [in] Maximum transaction duration in seconds
 * @note Limits maximum duration to previous async write phase lenght
 */
void Bw_limit::limit_checkpoint(long long transaction_size, double duration_sec) {
	std::lock_guard lock(bw_lock);

	if (!p_aw->phase_active()) {
		set_throughput_impl(TransactionType::Async_Write);
		Bw_limit::set_phase_info(TransactionType::Async_Write, "B_avr", bw_limit_iwrite);
		last_phase_write_bw = bw_limit_iwrite;
		bw_limit_iwrite = 0.0;
		total_file_limit_write = 0.0;
		EMPI_DESIRED_BW_IWRITE = 0.0;
	}

	auto phase_duration = p_aw->get_last_phase_duration();

	double transaction_time = phase_duration? 
		std::min(phase_duration.value(), duration_sec) : duration_sec;
	
	if (transaction_time <= 0.0) return;

	double bw = static_cast<double>(transaction_size) / transaction_time;
	double chkpt_bw_limit = bw * TOL;

	Bw_limit::Log<VerbosityLevel::BASIC_LOG>("%s > rank %i / %i > %s%s %s> BW %.2f Mb/s %s\n",
					caller, rank, processes - 1, YELLOW, "checkpoint write", BLUE, chkpt_bw_limit / 1'000'000, BLACK);

	total_file_limit_write += chkpt_bw_limit;
	bw_limit_iwrite = Bw_limit::calculate_bw_limit(total_file_limit_write, last_phase_write_bw);
	EMPI_DESIRED_BW_IWRITE = bw_limit_iwrite;

	Bw_limit::Log<VerbosityLevel::BASIC_LOG>("%s > rank %i / %i > %s%s %s> BW overall: %.2f Mb/s %s\n",
					caller, rank, processes - 1, YELLOW, "checkpoint write", BLUE, bw_limit_iwrite / 1'000'000, BLACK);

}
#endif

#ifdef BW_LIMIT
/**
 * @brief Limit async read bandwidth based on known duration
 * @param transaction_size [in] Number of transmitted bytes
 * @param duration_sec [in] Maximum transaction duration in seconds
 * @note Limits maximum duration to previous async read phase lenght
 */
void Bw_limit::limit_prefetch(long long transaction_size, double duration_sec) {
	std::lock_guard lock(bw_lock);

	if (!p_ar->phase_active()) {
		set_throughput_impl(TransactionType::Async_Read);
		Bw_limit::set_phase_info(TransactionType::Async_Read, "B_avr", bw_limit_iread);
		last_phase_read_bw = bw_limit_iread;
		bw_limit_iread = 0.0;
		total_file_limit_read = 0.0;
		EMPI_DESIRED_BW_IREAD = 0.0;
	}

	auto phase_duration = p_ar->get_last_phase_duration();

	double transaction_time = phase_duration? 
		std::min(phase_duration.value(), duration_sec) : duration_sec;
	
	if (transaction_time <= 0.0) return;

	double bw = static_cast<double>(transaction_size) / transaction_time;
	double prefetch_bw_limit = bw * TOL;

	Bw_limit::Log<VerbosityLevel::BASIC_LOG>("%s > rank %i / %i > %s%s %s> BW %.2f Mb/s %s\n",
					caller, rank, processes - 1, YELLOW, "prefetch read", BLUE, prefetch_bw_limit / 1'000'000, BLACK);

	total_file_limit_read += prefetch_bw_limit;
	bw_limit_iread = Bw_limit::calculate_bw_limit(total_file_limit_read, last_phase_read_bw);
	EMPI_DESIRED_BW_IREAD = bw_limit_iread;

	Bw_limit::Log<VerbosityLevel::BASIC_LOG>("%s > rank %i / %i > %s%s %s> BW overall: %.2f Mb/s %s\n",
					caller, rank, processes - 1, YELLOW, "prefetch read", BLUE, bw_limit_iread / 1'000'000, BLACK);

}
#endif
#if BW_LIMIT_GRANULARITY == 1

/**
 * @brief Limits I/O through extern MPI based on previous phase behavior.
 */
void Bw_limit::limit_async() {
	std::lock_guard lock(bw_lock);
	
	if (size_t phase_count = p_aw->get_phase_count(); phase_count > counter_iwrite) {
		
		limit_async_impl(TransactionType::Async_Write);
		counter_iwrite = phase_count;
	} 
	
	if (size_t phase_count = p_ar->get_phase_count(); phase_count > counter_iread) {

		limit_async_impl(TransactionType::Async_Read);
		counter_iread = phase_count;
	}
}

/**
 * @brief Limits I/O through extern MPI based on previous phase behavior.
 * @param transaction [in] Type of the transaction
 */
void Bw_limit::limit_async_impl(TransactionType transaction) {
	double phase_throughput = set_throughput_impl(transaction);

	double measured_bw = 0.0;

	#if BW_LIMIT_FREQ == 1
		if(ftio_phase_pred < 0.0) {
			measured_bw = Bw_limit::get_last_phase_info(transaction, "B_sum").value();
		} else {
			Bw_limit::Log<VerbosityLevel::DETAILED_LOG>("Utilizing phase infromation for bandwidth\n");
			measured_bw = Bw_limit::get_last_phase_info(transaction, "data").value() / ftio_phase_pred; 
		}
	#else
		measured_bw = Bw_limit::get_last_phase_info(transaction, "B_sum").value();
	#endif

	double& prev_bw_limit = (transaction == TransactionType::Async_Write)? bw_limit_iwrite : bw_limit_iread;

	const double desired_bw = calculate_bw_limit(measured_bw, prev_bw_limit);

	if (desired_bw != prev_bw_limit) {
		Bw_limit::Log<VerbosityLevel::BASIC_LOG>("%s > rank %i / %i > %s%s %s> BW old goal: %.2f Mb/s   BW: %.2f Mb/s   BW new goal: %.2f Mb/s%s\n",
			caller, rank, processes - 1, GREEN, (transaction == TransactionType::Async_Write)? "async_write" : "async_read", BLUE,
			prev_bw_limit / 1'000'000, phase_throughput / 1'000'000, desired_bw / 1'000'000, BLACK);

		prev_bw_limit = desired_bw;
	}
	else {
		Bw_limit::Log<VerbosityLevel::BASIC_LOG>("%s > rank %i / %i > %s%s %s> BW old goal: %.2f Mb/s   BW: %.2f Mb/s %s\n",
			caller, rank, processes - 1, GREEN, (transaction == TransactionType::Async_Write)? "async_write" : "async_read", BLUE,
			desired_bw / 1'000'000, phase_throughput / 1'000'000, BLACK);
	}

	Bw_limit::set_phase_info(transaction, "B_avr",desired_bw);

	if(transaction == TransactionType::Async_Write) {
		EMPI_DESIRED_BW_IWRITE = desired_bw;
	} else {
		EMPI_DESIRED_BW_IREAD = desired_bw;
	}
}

#endif

#ifdef BW_LIMIT
/**
 * @brief Calculates new bandwidth limit based on current strategy
 * @param measured_bw [in] Last bw measured for this transaction
 * @param prev_bw_limit [in] Last limit applied to this transaction
 */
double Bw_limit::calculate_bw_limit(const double measured_bw, const double prev_bw_limit) const
{
	double pot_limit = TOL * measured_bw;

	if constexpr (BW_LIMIT_STRATEGY == 1) { // increase only
		if (pot_limit <= prev_bw_limit)
			return prev_bw_limit;

	} else if constexpr (BW_LIMIT_STRATEGY == 2) {
		if (pot_limit < prev_bw_limit) // limit the downside
			return pot_limit + (abs(pot_limit - prev_bw_limit) / 2);
	}
	// else just take pot_limit as is
	return pot_limit;
}

#endif

#ifdef CUSTOM_MPI
/**
 * @brief Assigns throughput through custom MPI version
 */
void Bw_limit::set_throughput(void)
{
	std::lock_guard lock(bw_lock);

	if (size_t phase_count = p_aw->get_phase_count(); phase_count > counter_iwrite) {
		set_throughput_impl(TransactionType::Async_Write);
		counter_iwrite = phase_count;
	} 
	
	if (size_t phase_count = p_ar->get_phase_count(); phase_count > counter_iread) {
		set_throughput_impl(TransactionType::Async_Read);
		counter_iread = phase_count;
	}
}
#endif

#if (defined BW_LIMIT) || (defined CUSTOM_MPI)
//!------------------- modify T and duration in case custom MPI version or BW Limit---------------
/**
 * @brief Assigns throughput through custom MPI version for one transaction type
 * @param tt [in] Type of transaction to asign throughput to
 * @return Throughput during phase
 */
double Bw_limit::set_throughput_impl(TransactionType tt)
{
	if((tt == TransactionType::Async_Write && p_aw->get_phase_count() < 1) ||
	 	(tt == TransactionType::Async_Read && p_ar->get_phase_count() < 1)) return 0.0;

	
	long& empi_data = (tt == TransactionType::Async_Write)? EMPI_DATA_IWRITE : EMPI_DATA_IREAD;
	long& empi_utime = (tt == TransactionType::Async_Write)? EMPI_UTIME_IWRITE : EMPI_UTIME_IREAD;
	if(empi_data == 0 || empi_utime == 0) return 0;

	double phase_throughput = (static_cast<double>(empi_data)) / (static_cast<double>(empi_utime) / 1'000'000);

	//?Async write
	Bw_limit::set_phase_info(tt, "T_avr", phase_throughput);
	Bw_limit::set_phase_info(
		tt, "t_end_act",
		Bw_limit::get_last_phase_info(tt, "t_start").value() + static_cast<double>(empi_utime) / 1'000'000);

	Bw_limit::Log<VerbosityLevel::BASIC_LOG>("%s > rank %i / %i > %s%s %s> T set to %.2f Mb/s %s\n",
			caller, rank, processes - 1, YELLOW, (tt == TransactionType::Async_Write)? "async_write" : "async_read",
			BLUE, phase_throughput / 1'000'000, BLACK);

	empi_data = 0;
	empi_utime = 0;

	return phase_throughput;
}
#endif


#if BW_LIMIT_FREQ == 1
//************************************************************************************
//*                               1. receive_dominant_frequency
//************************************************************************************
/**
 * @brief Set dominant io frequency for bandwdith limit
 * @param dominant_frequeny [in] Applied frequency
 */
void Bw_limit::set_io_frequency(double dominant_frequency) {
	std::lock_guard lock(bw_lock);

	if (dominant_frequency > 0.0) {
		ftio_phase_pred = 1.0 / dominant_frequency;
	}
}
#endif