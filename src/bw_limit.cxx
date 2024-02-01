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
std::string Bw_limit::Info(void)
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
double Bw_limit::Get(std::string mode, std::string info)
{

	if (mode == "aw")
		return (p_aw->phase_data.size() == 0) ? 0 : p_aw->phase_data.back().get(info);
	else if (mode == "ar")
		return (p_ar->phase_data.size() == 0) ? 0 : p_ar->phase_data.back().get(info);
	else if (mode == "sw")
		return (p_sw->phase_data.size() == 0) ? 0 : p_sw->phase_data.back().get(info);
	else if (mode == "sr")
		return (p_sr->phase_data.size() == 0) ? 0 : p_sr->phase_data.back().get(info);
	else
		return 0;
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
void Bw_limit::Set(std::string mode, std::string info, double value)
{

	if (mode == "aw")
		p_aw->phase_data.back().set(info, value);
	else if (mode == "ar")
		p_ar->phase_data.back().set(info, value);
	else if (mode == "sw")
		p_sw->phase_data.back().set(info, value);
	else if (mode == "sr")
		p_sr->phase_data.back().set(info, value);
	else
		printf("not supported assignment");
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

	first_time_read = 1;
	first_time_write = 1;
	first_time_iread = 1;
	first_time_iwrite = 1;

	Biw = EMPI_DESIRED_BW_IWRITE;
	Bir = EMPI_DESIRED_BW_IREAD;
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
 *
 */
void Bw_limit::Limit_Async(void)
{
	//?Async write
	if (p_aw->phase_data.size() > counter_iwrite)
	{
		// static int counter = 0;
		// counter++;
		// Set("aw", "B_sum",Get("aw", "B_sum")*1/counter);
		double B = TOL * Get("aw", "B_sum");
		double T = ((double)EMPI_DATA_IWRITE) / ((double)EMPI_UTIME_IWRITE / 1'000'000);
		// printf("asyn write: EMPI_UTIME_IWRITE: %ld, EMPI_DATA_IWRITE: %ld, \n", EMPI_UTIME_IWRITE, EMPI_DATA_IWRITE);
		Set("aw", "T_avr", T);
		Set("aw", "t_end_act", Get("aw", "t_start") + (double)EMPI_UTIME_IWRITE / 1'000'000);

		bool change = false;

		// strategy for changing
#if BW_LIMIT_STRATEGY == 1 // increase only
		if (B > Biw)
			change = true;
		else
			// dont apply change. take the old value
			B = Biw;
#elif BW_LIMIT_STRATEGY == 2 // limit like PID whenever B decreases
		if (B < Biw)
		{
			// printf("Rank %i: B is %.2f, Biw is %.f, new B is %.2f \n",rank,B,Biw,B + abs(B-Biw)/2);
			B = B + abs(B - Biw) / 2;
		}
		change = true;
#else
		change = true;
#endif

		Set("aw", "B_avr", B);
		EMPI_DESIRED_BW_IWRITE = B;
#if BW_LIMIT_VERBOSE >= 1
		double Tempi = ((double)EMPI_DATA_IWRITE) / ((double)EMPI_UTIME_IWRITE / 1'000'000);
#endif
		// T = Tempi;
		// if ((first_time_iwrite == 1) && (scale_bw_iwrite == 1.0))
		// {
		//     dc_init_context(B, &context_iwrite);
		//     dc_change_limits(1.0, 2.0, 2.0, &context_iwrite);
		//     double init_scale;
		//     dc_first_value(scale_bw_iwrite, T, EMPI_DATA_IWRITE, &init_scale, &context_iwrite);
		//     scale_bw_iwrite   = init_scale;
		//     first_time_iwrite = 0;
		// }
		// else
		// {
		//     change = Bw_goal(B,Biw,&context_iwrite);
		//     scale_bw_iwrite = dc_next_value(scale_bw_iwrite, T, EMPI_DATA_IWRITE, &context_iwrite);

		// }
		counter_iwrite = p_aw->phase_data.size();
		EMPI_DATA_IWRITE = 0;
		EMPI_UTIME_IWRITE = 0;
		EMPI_SCALE_BW_IWRITE = 1.0;
		// EMPI_SCALE_BW_IWRITE = scale_bw_iwrite;

		if (change)
		{
#if BW_LIMIT_VERBOSE >= 1
			printf("%s > rank %i / %i > %sasync write %s> BW old goal: %.2f Mb/s   BW: %.2f (%.2f) Mb/s   BW new goal: %.2f Mb/s%s\n", caller, rank, processes - 1, GREEN, BLUE, Biw / 1'000'000, T / 1'000'000, Tempi / 1'000'000, B / 1'000'000, BLACK);
#endif
			Biw = B;
		}
		else
		{
#if BW_LIMIT_VERBOSE >= 1
			printf("%s > rank %i / %i > %sasync write %s> BW old goal: %.2f Mb/s   BW: %.2f (%.2f) Mb/s %s\n", caller, rank, processes - 1, GREEN, BLUE, Biw / 1'000'000, T / 1'000'000, Tempi / 1'000'000, BLACK);
#endif
		}
#if BW_LIMIT_VERBOSE >= 2
		printf("%s > rank %i / %i > %sasync write %s> scale write is: %f %s\n", caller, rank, processes - 1, GREEN, RED, (double)EMPI_SCALE_BW_IWRITE, BLACK);
		printf("%s > rank %i / %i > %sasync write %s> scale is: %.2Lf %s\n", caller, rank, processes - 1, GREEN, BLUE, EMPI_SCALE_BW_IWRITE, BLACK);
#endif
	}
	//?Async read
	if (p_ar->phase_data.size() > counter_iread)
	{
		// if (p_ar->phase_data.size() == 1)
		double B = TOL * Get("ar", "B_sum");
		// double T = Get("ar", "T_sum");
		double T = ((double)EMPI_DATA_IREAD) / ((double)EMPI_UTIME_IREAD / 1'000'000);
		// Set("ar", "T_sum", T);
		Set("ar", "T_avr", T);
		Set("ar", "t_end_act", Get("ar", "t_start") + (double)EMPI_UTIME_IREAD / 1'000'000);

		bool change = false;

		// strategy for changing
#if BW_LIMIT_STRATEGY == 1 // increase only
		if (B > Bir)
			change = true;
		else
			// dont apply change. take old valie
			B = Bir;
#elif BW_LIMIT_STRATEGY == 2 // limit the downside
		if (B < Bir)
			B = B + abs(B - Bir) / 2;
		change = true;
#else
		change = true;
#endif

		Set("ar", "B_avr", B);
		EMPI_DESIRED_BW_IREAD = B;
#if BW_LIMIT_VERBOSE >= 1
		double Tempi = ((double)EMPI_DATA_IREAD) / ((double)EMPI_UTIME_IREAD / 1000000);
#endif
		// T = Tempi;

		// if ((first_time_iread == 1)  && (scale_bw_iread == 1.0))
		// {
		//     dc_init_context(B, &context_iread);
		//     dc_change_limits(1.0, 2.0, 2.0, &context_iread);
		//     double init_scale; //= 1;//B / T;
		//     dc_first_value(scale_bw_iread, T, EMPI_DATA_IREAD, &init_scale, &context_iread);
		//     scale_bw_iread = init_scale;
		//     first_time_iread = 0;
		// }
		// //? all other read
		// else //if (p_ar->phase_data.size() >= 1)
		// {
		//     change = Bw_goal(B,Bir,&context_iread);
		//     scale_bw_iread = dc_next_value(scale_bw_iread, T, EMPI_DATA_IREAD, &context_iread);

		// }

		counter_iread = p_ar->phase_data.size();
		EMPI_UTIME_IREAD = 0;
		EMPI_DATA_IREAD = 0;
		EMPI_SCALE_BW_IREAD = 1;
		// EMPI_SCALE_BW_IREAD   = scale_bw_iread;
		if (change)
		{
#if BW_LIMIT_VERBOSE >= 1
			printf("%s > rank %i / %i > %sasync read %s> BW old goal: %.2f Mb/s   BW: %.2f (%.2f) Mb/s   BW new goal: %.2f Mb/s%s\n", caller, rank, processes - 1, YELLOW, BLUE, Bir / 1'000'000, T / 1'000'000, Tempi / 1'000'000, B / 1'000'000, BLACK);
#endif
			Bir = B;
		}
		else
		{
#if BW_LIMIT_VERBOSE >= 1
			printf("%s > rank %i / %i > %sasync read %s> BW old goal: %.2f Mb/s   BW: %.2f (%.2f) Mb/s %s\n", caller, rank, processes - 1, YELLOW, BLUE, Bir / 1'000'000, T / 1'000'000, Tempi / 1'000'000, BLACK);
#endif
		}
	}

	// sync parts moved to sync start
}

//************************************************************************************
//*                               2. Bw_goal
//************************************************************************************
// bool Bw_limit::Bw_goal(double B_new, double B_old, dc_context_t *context)
// {
//     bool change = false;
//     if (B_new > 0 && abs(B_old - B_new) / B_new > 0.2)
//     {
//         dc_change_goal(B_new, context); // change goal
//         change = true;
//     }
//     return change;
// }
#endif

//!------------------- modify T and duration in case custom MPI version ---------------
#ifdef CUSTOM_MPI
//************************************************************************************
//*                               1. Set_Throughput
//************************************************************************************
/**
 * @brief assigns Throughput through Custome MPI version
 *
 */
void Bw_limit::Set_Throughput(void)
{
	//?Async write
	if (p_aw->phase_data.size() > counter_iwrite)
	{
		double T = ((double)EMPI_DATA_IWRITE) / ((double)EMPI_UTIME_IWRITE / 1'000'000);
		Set("aw", "T_avr", T);
		Set("aw", "t_end_act", Get("aw", "t_start") + (double)EMPI_UTIME_IWRITE / 1'000'000);

#if BW_LIMIT_VERBOSE >= 1
		printf("%s > rank %i / %i > %sasync write %s> T set to(%.2f) Mb/s %s\n", caller, rank, processes - 1, YELLOW, BLUE, T / 1'000'000, BLACK);
#endif

		counter_iwrite = p_aw->phase_data.size();
		EMPI_UTIME_IWRITE = 0;
		EMPI_DATA_IWRITE = 0;
	}

	if (p_ar->phase_data.size() > counter_iread)
	{
		double T = ((double)EMPI_DATA_IREAD) / ((double)EMPI_UTIME_IREAD / 1'000'000);
		Set("ar", "T_avr", T);
		Set("ar", "t_end_act", Get("ar", "t_start") + (double)EMPI_UTIME_IREAD / 1'000'000);

#if BW_LIMIT_VERBOSE >= 1
		printf("%s > rank %i / %i > %sasync read %s> T set to(%.2f) Mb/s %s\n", caller, rank, processes - 1, YELLOW, BLUE, T / 1'000'000, BLACK);
#endif

		counter_iread = p_ar->phase_data.size();
		EMPI_UTIME_IREAD = 0;
		EMPI_DATA_IREAD = 0;
	}
}
#endif