#include "bw_limit.h"

#if BW_LIMIT_FTIO == 1 
#include <zmq.hpp>
#endif


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
 * @param void
 * @return void
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
 * @param void
 * @return void
 */
void Bw_limit::Reset(void)
{
	counter_iread = 0;
	counter_iwrite = 0;
	counter_read = 0;
	counter_write = 0;
}

//************************************************************************************
//*                               1. get_phase_info
//************************************************************************************
/**
 * @brief returns the I/O traces to extern libaries.
 *
 * @param mode either "aw", "sw", "ar" or "sr"
 * @param info info needs to be in the form of iocollect: "t_start", "t_end_act", "t_end_req", "T_sum", "T_avr", "B_sum", "B_avr"
 * @return double
 */
double Bw_limit::get_phase_info(TransactionType mode, std::string info) const
{
	double res;
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
 * @brief assigns the I/O traces by extern libaries.
 *
 * @param mode either "aw", "sw", "ar" or "sr"
 * @param info info needs to be in the form of iocollect: "t_start", "t_end_act", "t_end_req", "T_sum", "T_avr", "B_sum", "B_avr"
 * @param value value assigned to the variable
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
 * @brief Assigns all externs variables
 *
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

#if BW_LIMIT_FTIO == 1
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

	bw_limit_iwrite = EMPI_DESIRED_BW_IWRITE;
	bw_limit_iread = EMPI_DESIRED_BW_IREAD;
	Bw = EMPI_DESIRED_BW_WRITE;
	Br = EMPI_DESIRED_BW_READ;
#endif
}

#if BW_LIMIT_GRANULARITY > 1

void Bw_limit::limit_by_file(bool write, [[maybe_unused]] const std::filesystem::path* path, [[maybe_unused]] long long transaction_size)
{
	std::lock_guard lock(bw_lock);

	if (size_t phase_count = p_aw->get_phase_count(); phase_count > counter_iwrite) {
		set_throughput_impl(TransactionType::Async_Write);
		bw_limit_iwrite = 0.0;
		counter_iwrite = phase_count;
	}
	
	if (size_t phase_count = p_ar->get_phase_count(); phase_count > counter_iread) {
		set_throughput_impl(TransactionType::Async_Read);
		bw_limit_iwrite = 0.0;
		counter_iread = phase_count;
	}

	double file_measured_bw = 0.0;

	TransactionType transaction = write? TransactionType::Async_Write : TransactionType::Async_Read;
	IOdata* p_data = write? p_aw : p_ar;

	
	if(path)
		file_measured_bw = p_data->get_prev_file_bw(*path);

	if (file_measured_bw <= 0.0) {
		Bw_limit::Log<VerbosityLevel::BASIC_LOG>("No previous bandwidth, falling back to phase BW");
		double phase_duration = Bw_limit::get_phase_info(transaction, "t_start") - Bw_limit::get_phase_info(transaction, "t_end_req");
		file_measured_bw = transaction_size / phase_duration;
	}
	
	double file_bw_limit = file_measured_bw * TOL;

	if constexpr (BW_LIMIT_GRANULARITY == 3) {
		double scale_factor = 1.0;

		if (path) {
			long long prev_transaction_size = p_data->get_prev_file_size(*path);

			if(prev_transaction_size > 0) {
				scale_factor = static_cast<double>(transaction_size) / static_cast<double>(prev_transaction_size);

				double scaled_limit = scale_factor * file_bw_limit;

				Bw_limit::Log<VerbosityLevel::BASIC_LOG>("%s > rank %i / %i > %s%s %s> BW scale factor: %.2f   prev: %lld B   cur: %lld B   BW new scaled goal %.2f Mb/s%s\n",
					caller, rank, processes - 1, YELLOW,  write? "async_write" : "async_read", BLUE,
					scale_factor, prev_transaction_size, transaction_size, scaled_limit / 1'000'000, BLACK);

				file_bw_limit = scaled_limit;
			} else {
				Bw_limit::Log<VerbosityLevel::BASIC_LOG>("No previous transaction, no scaling applied");
			}
		}
	}

	if(transaction == TransactionType::Async_Write) {
		bw_limit_iwrite += file_bw_limit;
		EMPI_DESIRED_BW_IWRITE = bw_limit_iwrite;
	} else {
		bw_limit_iread += file_bw_limit;
		EMPI_DESIRED_BW_IREAD = bw_limit_iread;
	}
}

#endif

#if BW_LIMIT_GRANULARITY == 1

//! --------------------------- For BW Limiting only -----------------------------------
//************************************************************************************
//*                               1. limit_async
//************************************************************************************
/**
 * @brief Limits I/O through extern MPI.
 */
void Bw_limit::limit_async() {
	std::lock_guard lock(bw_lock);
	
	if (size_t phase_count = p_aw->get_phase_count(); phase_count > counter_iwrite) {
		
		limit_async_impl(TransactionType::Async_Write);
		counter_iwrite = phase_count;
	} else if (size_t phase_count = p_ar->get_phase_count(); phase_count > counter_iread) {

		limit_async_impl(TransactionType::Async_Read);
		counter_iread = phase_count;
	}
}

void Bw_limit::limit_async_impl(TransactionType transaction) {
	double phase_throughput = set_throughput_impl(transaction);

	double measured_bw = 0.0;

	if constexpr (BW_LIMIT_FTIO == 1) {
		if(ftio_phase_pred < 0.0) {
			measured_bw = Bw_limit::get_phase_info(transaction, "B_sum");
		} else {
			measured_bw = Bw_limit::get_phase_info(transaction, "data") / ftio_phase_pred;
		}
	} else {
		measured_bw = Bw_limit::get_phase_info(transaction, "B_sum");
	}

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
 * @param measured_bw last bw measured for this transaction
 * @param prev_bw_limit last limit applied to this transaction
 */
double Bw_limit::calculate_bw_limit(const double measured_bw, const double prev_bw_limit) const
{
	double pot_limit = TOL * measured_bw;

	if constexpr (BW_LIMIT_STRATEGY == 1) { // increase only
		if (pot_limit <= prev_bw_limit)
			return prev_bw_limit;

	} else if constexpr (BW_LIMIT_STRATEGY == 2) {
		if (pot_limit < prev_bw_limit) // limit the downside
			pot_limit = pot_limit + abs(pot_limit - prev_bw_limit) / 2;

	} else { // just take the new limit
		pot_limit;
	}
	return pot_limit;
}

#endif

//!------------------- modify T and duration in case custom MPI version or BW Limit---------------
#ifdef CUSTOM_MPI
//************************************************************************************
//*                               1. set_throughput
//************************************************************************************
/**
 * @brief assigns throughput through custom MPI version
 *
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
//************************************************************************************
//*                               1. set_throughput
//************************************************************************************
/**
 * @brief assigns throughput through custom MPI version for one transaction type
 * @param tt type of transaction to asign throughput to
 */
double Bw_limit::set_throughput_impl(TransactionType tt)
{
	long& empi_data = (tt == TransactionType::Async_Write)? EMPI_DATA_IWRITE : EMPI_DATA_IREAD;
	long& empi_utime = (tt == TransactionType::Async_Write)? EMPI_UTIME_IWRITE : EMPI_UTIME_IREAD;

	double phase_throughput = (static_cast<double>(empi_data)) / (static_cast<double>(empi_utime) / 1'000'000);

	//?Async write
	Bw_limit::set_phase_info(tt, "T_avr", phase_throughput);
	Bw_limit::set_phase_info(
		tt, "t_end_act",
		Bw_limit::get_phase_info(tt, "t_start") + static_cast<double>(empi_utime) / 1'000'000);

	Bw_limit::Log<VerbosityLevel::BASIC_LOG>("%s > rank %i / %i > %s%s %s> T set to(%.2f) Mb/s %s\n",
			caller, rank, processes - 1, YELLOW, (tt == TransactionType::Async_Write)? "async_write" : "async_read",
			BLUE, phase_throughput / 1'000'000, BLACK);

	empi_data = 0;
	empi_utime = 0;

	return phase_throughput;
}
#endif


#if BW_LIMIT_FTIO == 1
//************************************************************************************
//*                               1. receive_dominant_frequency
//************************************************************************************
/**
 * @brief Receive dominant frequency from FTIO via ZMQ
 *
 */
void Bw_limit::receive_dominant_frequency(int rank, MPI_Comm IO_WORLD) {
	std::lock_guard lock(bw_lock);

	double dominant_frequency = -1.0;

	if (rank == 0) {
		zmq::context_t context(1);
		zmq::socket_t receiver(context, ZMQ_PULL);
		receiver.connect("tcp://127.0.0.1:5556");
		
		zmq::message_t msg;
		receiver.recv(&msg, ZMQ_DONTWAIT);

		if (!msg.empty()) {
			std::memcpy(&dominant_frequency, msg.data(), sizeof(double));
		}
	}

	int root = 0;

	MPI_Bcast(&dominant_frequency, 1, MPI_DOUBLE, 0, IO_WORLD);

	if (dominant_frequency > 0.0) {
		ftio_phase_pred = 1.0 / dominant_frequency;
	}
}
#endif