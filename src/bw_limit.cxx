#include "bw_limit.h"


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
//*                               1. Get
//************************************************************************************
/**
 * @brief returns the I/O traces to extern libaries.
 *
 * @param mode either "aw", "sw", "ar" or "sr"
 * @param info info needs to be in the form of iocollect: "t_start", "t_end_act", "t_end_req", "T_sum", "T_avr", "B_sum", "B_avr"
 * @return double
 */
double Bw_limit::Get(Transaction_Type mode, std::string info) const
{
	double res;
	switch (mode)
	{
		case Transaction_Type::Async_Write:
			res = (p_aw->phase_data.empty()) ? -1.0 : p_aw->phase_data.back().get(info);
			break;
		case Transaction_Type::Async_Read:
			res = (p_ar->phase_data.empty()) ? -1.0 : p_ar->phase_data.back().get(info);
			break;
		case Transaction_Type::Sync_Read:
			res = (p_sr->phase_data.empty()) ? -1.0 : p_sr->phase_data.back().get(info);
			break;
		case Transaction_Type::Sync_Write:
			res = (p_sw->phase_data.empty()) ? -1.0 : p_sw->phase_data.back().get(info);
			break;
	}
	return res;
}

//************************************************************************************
//*                               2. Set
//************************************************************************************
/**
 * @brief assigns the I/O traces by extern libaries.
 *
 * @param mode either "aw", "sw", "ar" or "sr"
 * @param info info needs to be in the form of iocollect: "t_start", "t_end_act", "t_end_req", "T_sum", "T_avr", "B_sum", "B_avr"
 * @param value value assigned to the variable
 */
void Bw_limit::Set(Transaction_Type mode, std::string info, double value)
{
	switch (mode)
	{
		case Transaction_Type::Async_Write:
			if (!p_aw->phase_data.empty())
				p_aw->phase_data.back().set(info, value);
			break;
		case Transaction_Type::Async_Read:
			if (!p_ar->phase_data.empty())
				p_ar->phase_data.back().set(info, value);
			break;
		case Transaction_Type::Sync_Read:
			if (!p_sr->phase_data.empty())
				p_sr->phase_data.back().set(info, value);
			break;
		case Transaction_Type::Sync_Write:
			if (!p_sw->phase_data.empty())
				p_sw->phase_data.back().set(info, value);
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

#ifdef BW_LIMIT

//! --------------------------- For BW Limiting only -----------------------------------
//************************************************************************************
//*                               1. Limit_Async
//************************************************************************************
/**
 * @brief Limits I/O through extern MPI.
 * @param path path to the accessed file
 * @param write true = write, false = read
 */
void Bw_limit::Limit_Async(bool write, [[maybe_unused]] const std::filesystem::path* path, [[maybe_unused]] long long transaction_size)
{
	// Async write
	if (write)
	{
		const double T = (static_cast<double>(EMPI_DATA_IWRITE)) / (static_cast<double>(EMPI_UTIME_IWRITE) / 1'000'000);

		double measured_bw; 

		if constexpr (BW_FILE_SPECIFIC == 1) {
			if(path)
				measured_bw = p_aw->Get_Prev_File_BW(*path);
		} else {
			measured_bw = Bw_limit::Get(Transaction_Type::Async_Write, "B_sum");
		}

		if (measured_bw <= 0.0) {
			Bw_limit::Log<VerbosityLevel::BASIC_LOG>("No previous bandwidth");
			return;
		}
		
		const double bw = Get_BW_Limit(measured_bw, bw_limit_iwrite);

		if (bw != bw_limit_iwrite) {
			Bw_limit::Log<VerbosityLevel::BASIC_LOG>("%s > rank %i / %i > %sasync write %s> BW old goal: %.2f Mb/s   BW: %.2f (%.2f) Mb/s   BW new goal: %.2f Mb/s%s\n", caller, rank, processes - 1, GREEN, BLUE, bw_limit_iwrite / 1'000'000, T / 1'000'000, T / 1'000'000, bw / 1'000'000, BLACK);
			bw_limit_iwrite = bw;
		}
		else {
			Bw_limit::Log<VerbosityLevel::BASIC_LOG>("%s > rank %i / %i > %sasync write %s> BW old goal: %.2f Mb/s   BW: %.2f (%.2f) Mb/s %s\n", caller, rank, processes - 1, GREEN, BLUE, bw_limit_iwrite / 1'000'000, T / 1'000'000, T / 1'000'000, BLACK);
		}

		EMPI_DESIRED_BW_IWRITE = bw_limit_iwrite;

		EMPI_DATA_IWRITE = 0;
		EMPI_UTIME_IWRITE = 0;
		
		double scale_factor = 1.0;
		if constexpr (BW_FILE_SCALING == 1) {
			if (path) {
				long long prev_transaction_size = p_aw->Get_Prev_File_Size(*path);
				scale_factor = static_cast<double>(transaction_size) / static_cast<double>(prev_transaction_size);

				double scaled_limit = scale_factor * bw;

				Bw_limit::Log<VerbosityLevel::BASIC_LOG>("%s > rank %i / %i > %sasync write %s> BW scale factor: %.2f   prev: %lld B   cur: %lld B   BW new scaled goal %.2f Mb/s%s\n", caller, rank, processes - 1, YELLOW, BLUE, scale_factor, prev_transaction_size, transaction_size, scaled_limit / 1'000'000, BLACK);
			}
		}

		EMPI_SCALE_BW_IWRITE = scale_factor;
	} 
	else // Async read
	{
		const double T = (static_cast<double>(EMPI_DATA_IREAD)) / (static_cast<double>(EMPI_UTIME_IREAD) / 1'000'000);

		double measured_bw;

		if constexpr (BW_FILE_SPECIFIC == 1) {
			if(path)
				measured_bw = p_ar->Get_Prev_File_BW(*path);
		} else {
			measured_bw = Bw_limit::Get(Transaction_Type::Async_Read, "B_sum");
		}

		if (measured_bw <= 0.0) {
			Bw_limit::Log<VerbosityLevel::BASIC_LOG>("No previous bandwidth");
			return;
		}

		auto bw = Get_BW_Limit(measured_bw, bw_limit_iread);
				
		if (bw != bw_limit_iread) {
			Bw_limit::Log<VerbosityLevel::BASIC_LOG>("%s > rank %i / %i > %sasync read %s> BW old goal: %.2f Mb/s   BW: %.2f (%.2f) Mb/s   BW new goal: %.2f Mb/s%s\n", caller, rank, processes - 1, YELLOW, BLUE, bw_limit_iread / 1'000'000, T / 1'000'000, T / 1'000'000, bw / 1'000'000, BLACK);
			bw_limit_iread = bw;
		} else {
			Bw_limit::Log<VerbosityLevel::BASIC_LOG>("%s > rank %i / %i > %sasync read %s> BW old goal: %.2f Mb/s   BW: %.2f (%.2f) Mb/s %s\n", caller, rank, processes - 1, YELLOW, BLUE, bw_limit_iread / 1'000'000, T / 1'000'000, T / 1'000'000, BLACK);
		}
		
		EMPI_DESIRED_BW_IREAD = bw_limit_iread;

		EMPI_UTIME_IREAD = 0;
		EMPI_DATA_IREAD = 0;

		double scale_factor = 1.0;
		if constexpr (BW_FILE_SCALING == 1) {
			if (path) {
				long long prev_transaction_size = p_ar->Get_Prev_File_Size(*path);
				scale_factor = static_cast<double>(transaction_size) / static_cast<double>(prev_transaction_size);

				double scaled_limit = scale_factor * bw;

				Bw_limit::Log<VerbosityLevel::BASIC_LOG>("%s > rank %i / %i > %sasync read %s> BW scale factor: %.2f   prev: %lld B   cur: %lld B   BW new scaled goal %.2f Mb/s%s\n", caller, rank, processes - 1, YELLOW, BLUE, scale_factor, prev_transaction_size, transaction_size, scaled_limit / 1'000'000, BLACK);
			}
		} 

		EMPI_SCALE_BW_IREAD = scale_factor;
	}
}

/**
 * @brief Calculates new bandwidth limit based on current strategy
 * @param measured_bw last bw measured for this transaction
 * @param prev_bw_limit last limit applied to this transaction
 */
double Bw_limit::Get_BW_Limit(const double measured_bw, const double prev_bw_limit) const
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
#if defined CUSTOM_MPI || defined BW_LIMIT
//************************************************************************************
//*                               1. Set_Throughput
//************************************************************************************
/**
 * @brief assigns Throughput through custom MPI version
 *
 */
void Bw_limit::Set_Throughput(void)
{
	//?Async write
	if (p_aw->phase_data.size() > counter_iwrite)
	{
		double T = (static_cast<double>(EMPI_DATA_IWRITE)) / (static_cast<double>(EMPI_UTIME_IWRITE) / 1'000'000);
		Bw_limit::Set(Transaction_Type::Async_Write, "T_avr", T);
		Bw_limit::Set(Transaction_Type::Async_Write, "t_end_act", Bw_limit::Get(Transaction_Type::Async_Write, "t_start") + static_cast<double>(EMPI_UTIME_IWRITE) / 1'000'000);

		Bw_limit::Log<VerbosityLevel::BASIC_LOG>("%s > rank %i / %i > %sasync write %s> T set to(%.2f) Mb/s %s\n", caller, rank, processes - 1, YELLOW, BLUE, T / 1'000'000, BLACK);

		counter_iwrite = p_aw->phase_data.size();
		EMPI_UTIME_IWRITE = 0;
		EMPI_DATA_IWRITE = 0;
	}

	if (p_ar->phase_data.size() > counter_iread)
	{
		double T = (static_cast<double>(EMPI_DATA_IREAD)) / (static_cast<double>(EMPI_UTIME_IREAD) / 1'000'000);
		Bw_limit::Set(Transaction_Type::Async_Read, "T_avr", T);
		Bw_limit::Set(Transaction_Type::Async_Read, "t_end_act", Bw_limit::Get(Transaction_Type::Async_Read, "t_start") + static_cast<double>(EMPI_UTIME_IREAD) / 1'000'000);

		Bw_limit::Log<VerbosityLevel::BASIC_LOG>("%s > rank %i / %i > %sasync read %s> T set to(%.2f) Mb/s %s\n", caller, rank, processes - 1, YELLOW, BLUE, T / 1'000'000, BLACK);

		counter_iread = p_ar->phase_data.size();
		EMPI_UTIME_IREAD = 0;
		EMPI_DATA_IREAD = 0;
	}
}
#endif