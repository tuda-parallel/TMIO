#include "iotime.h"

iotime::iotime(void)
{
	// pass
}

iotime::~iotime(void)
{
	// pass
}

/**
 * @brief Construct a new iotime object.
 * Used to store all time information related to the application
 * @details -
 *
 * @param t array contating (1) total application time (aggregated), (2) lib I/O overhead at
 * end of application, and (3) lib I/O overhead time (aggregated) during runtime of application controlled with \e OVERHEAD flag (see ioflags.h)
 * @param t_rank_0 ellapsed time of rank 0
 * @param sr statistics object representing synchnous read
 * @param ara statistics object representing asynchnous read (throughput)
 * @param arr statistics object representing asynchnous read (bandwidth)
 * @param sw statistics object representing synchnous write
 * @param awa statistics object representing asynchnous write (throughput)
 * @param awr statistics object representing asynchnous write (bandwidth)
 */
iotime::iotime(double *t, double *t_rank_0, statistics sr, statistics ar, statistics sw, statistics aw)
{
	//? overhead
	//?---------------
	delta_t_overhead_post_runtime = t[1];
#if OVERHEAD == 1
	delta_t_overhead_peri_runtime = t[2];
#else
	delta_t_overhead_peri_runtime = 0;
#endif
	delta_t_overhead = delta_t_overhead_post_runtime + delta_t_overhead_peri_runtime;
#if DFT == 1
	delta_t_overhead_dft = sr.dft_time + sw.dft_time + ar.dft_time + aw.dft_time; // dft overhead is part of the total overhead
																				  // std::cout << "sr.dft_time: " << sr.dft_time << 	 "\nsw.dft_time: " << sw.dft_time << 	 "\nar.dft_time: " << ar.dft_time << 	 "\naw.dft_time: " << aw.dft_time << std::endl;
#else
	delta_t_overhead_dft = 0; 
#endif

	// application time
	delta_t_agg = t[0] - delta_t_overhead_peri_runtime; // remove overhead runtime from application runtime
	// sync read and write time
	delta_t_sr = sr.Total_Time();
	delta_t_sw = sw.Total_Time();
	// async read time
	delta_t_ara = ar.Total_Time();
	delta_t_arr = ar.Total_Time("t_end_req");
	delta_t_ar_lost = ar.Lost_Time();
	// async write time
	delta_t_awa = aw.Total_Time();
	delta_t_awr = aw.Total_Time("t_end_req");
	delta_t_aw_lost = aw.Lost_Time();
	// total I/O time
	delta_t_agg_io = delta_t_aw_lost + delta_t_ar_lost + delta_t_sr + delta_t_sw;
	deta_t_rank0_vec = t_rank_0;
	// rank 0 time
	delta_t_rank0 = deta_t_rank0_vec[0] + deta_t_rank0_vec[2];
	delta_t_rank0_app = deta_t_rank0_vec[0] - deta_t_rank0_vec[1];
	delta_t_rank0_overhead_post_runtime = deta_t_rank0_vec[2];
	delta_t_rank0_overhead_peri_runtime = deta_t_rank0_vec[1];
}

/**
 * @brief prints the content of iotime object to a file and on the dsiplay
 *
 * @param file [in] file to which to print to
 */
void iotime::print(std::ofstream &file)
{

	int values = 21;
#if OVERHEAD == 1
	values += 2;
#if DFT == 1
	values += 1;
#endif
#endif

	char out[values][150];
	double tmp = 0;
	int counter = 0;

	double total = delta_t_overhead + delta_t_agg;
	sprintf(out[counter++], "\nellapsed time (rank 0)             = %f sec\n", delta_t_rank0);
	sprintf(out[counter++], "%s|->%s application time               = %f sec \t-> from ellapsed time %s%.2f %%%s\n", BLUE, BLACK, delta_t_rank0_app, Color_Percent(100 * (delta_t_rank0_app) / (delta_t_rank0)), 100 * (delta_t_rank0_app) / (delta_t_rank0), BLACK);
	sprintf(out[counter++], "%s|->%s overhead during runtime        = %f sec \t-> from ellapsed time %s%.2f %%%s\n", BLUE, BLACK, delta_t_rank0_overhead_peri_runtime, Color_Percent(100 * delta_t_rank0_overhead_peri_runtime / (delta_t_rank0)), 100 * delta_t_rank0_overhead_peri_runtime / (delta_t_rank0), BLACK);
	sprintf(out[counter++], "%s'->%s overhead post runtime          = %f sec \t-> from ellapsed time %s%.2f %%%s\n\n", BLUE, BLACK, delta_t_rank0_overhead_post_runtime, Color_Percent(100 * delta_t_rank0_overhead_post_runtime / (delta_t_rank0)), 100 * delta_t_rank0_overhead_post_runtime / (delta_t_rank0), BLACK);
	sprintf(out[counter++], "%stotal run time%s                     = %f sec\n", BLUE, BLACK, total);
#if OVERHEAD == 1
	sprintf(out[counter++], "%s|->%s lib overhead time%s              = %f sec \t-> from run time %s%.2f %%%s\n", BLUE, RED, BLACK, delta_t_overhead, Color_Percent(100 * delta_t_overhead / total), 100 * delta_t_overhead / total, BLACK);
	sprintf(out[counter++], "%s|%s     |->%s during runtime           = %f sec \t-> from overhead %s%.2f %%%s\n", BLUE, RED, BLACK, delta_t_overhead_peri_runtime, Color_Percent(100 * delta_t_overhead_peri_runtime / delta_t_overhead), 100 * delta_t_overhead_peri_runtime / delta_t_overhead, BLACK);
	sprintf(out[counter++], "%s|%s     '->%s post runtime             = %f sec \t-> from overhead %s%.2f %%%s\n", BLUE, RED, BLACK, delta_t_overhead_post_runtime, Color_Percent(100 * delta_t_overhead_post_runtime / delta_t_overhead), 100 * delta_t_overhead_post_runtime / delta_t_overhead, BLACK);
#if DFT == 1
	sprintf(out[counter++], "%s|%s         '->%s dft overhead         = %f sec \t-> from overhead %s%.2f %%%s\n", BLUE, RED, BLACK, delta_t_overhead_dft, Color_Percent(100 * delta_t_overhead_dft / delta_t_overhead), 100 * delta_t_overhead_dft / delta_t_overhead, BLACK);
#endif
#else
	sprintf(out[counter++], "%s|->%s lib overhead time              = %f sec \t-> from run time %s%.2f %%%s\n", BLUE, BLACK, delta_t_overhead, Color_Percent(100 * delta_t_overhead / total), 100 * delta_t_overhead / total, BLACK);
#endif
	sprintf(out[counter++], "%s|\n'->%s app time%s                       = %f sec \t-> from run time %s%.2f %%%s\n", BLUE, GREEN, BLACK, delta_t_agg, Color_Percent(100 - 100 * delta_t_agg / total), 100 * delta_t_agg / total, BLACK);
	tmp = delta_t_agg - delta_t_agg_io;
	sprintf(out[counter++], "%s    |->%s total compute/comm. time   = %f sec \t-> from app time %s%.2f %%%s\n", GREEN, BLACK, tmp, Color_Percent(100 - 100 * tmp / delta_t_agg), 100 * tmp / delta_t_agg, BLACK);
	sprintf(out[counter++], "%s    '->%s total I/O time%s             = %f sec \t-> from app time %s%.2f %%%s\n", GREEN, YELLOW, BLACK, delta_t_agg_io, Color_Percent(100 * delta_t_agg_io / delta_t_agg), 100 * delta_t_agg_io / delta_t_agg, BLACK);
	sprintf(out[counter++], "%s        |->%s sync read time         = %f sec \t-> from app time %s%.2f %%%s\n", YELLOW, BLACK, delta_t_sr, Color_Percent(100 * delta_t_sr / delta_t_agg), 100 * delta_t_sr / delta_t_agg, BLACK);
	sprintf(out[counter++], "%s        |->%s sync write time        = %f sec \t-> from app time %s%.2f %%%s\n", YELLOW, BLACK, delta_t_sw, Color_Percent(100 * delta_t_sw / delta_t_agg), 100 * delta_t_sw / delta_t_agg, BLACK);
	sprintf(out[counter++], "%s        |%s\n", YELLOW, BLACK);
	sprintf(out[counter++], "%s        |->%s req. async read time   = %f sec \t-> from app time %s%.2f %%%s\n", YELLOW, BLACK, delta_t_arr, Color_Percent(100 * delta_t_arr / delta_t_agg), 100 * delta_t_arr / delta_t_agg, BLACK);
	sprintf(out[counter++], "%s        |->%s async read time%s        = %f sec \t-> from app time %s%.2f %%%s\n", YELLOW, CYAN, BLACK, delta_t_ara, Color_Percent(100 * delta_t_ara / delta_t_agg), 100 * delta_t_ara / delta_t_agg, BLACK);
	tmp = (delta_t_ara - delta_t_arr) > 0 ? delta_t_ara - delta_t_arr : 0;
	sprintf(out[counter++], "%s        |%s    |->%s approx. wait time = %s%f%s sec \t-> from app time %s%.2f %%%s\n", YELLOW, CYAN, BLACK, (tmp > 0) ? RED : GREEN, tmp, BLACK, Color_Percent(100 * tmp / delta_t_agg), 100 * tmp / delta_t_agg, BLACK);
	tmp = delta_t_ar_lost;
	sprintf(out[counter++], "%s        |%s    '->%s real wait time    = %s%f%s sec \t-> from app time %s%.2f %%%s\n", YELLOW, CYAN, BLACK, (tmp > 0) ? RED : GREEN, tmp, BLACK, Color_Percent(100 * tmp / delta_t_agg), 100 * tmp / delta_t_agg, BLACK);
	sprintf(out[counter++], "%s        |%s\n", YELLOW, BLACK);
	sprintf(out[counter++], "%s        |->%s req. async write time  = %f sec \t-> from app time %s%.2f %%%s\n", YELLOW, BLACK, delta_t_awr, Color_Percent(100 * delta_t_awr / delta_t_agg), 100 * delta_t_awr / delta_t_agg, BLACK);
	sprintf(out[counter++], "%s        '->%s async write time%s       = %f sec \t-> from app time %s%.2f %%%s\n", YELLOW, CYAN, BLACK, delta_t_awa, Color_Percent(100 * delta_t_awa / delta_t_agg), 100 * delta_t_awa / delta_t_agg, BLACK);
	tmp = (delta_t_awa - delta_t_awr) > 0 ? delta_t_awa - delta_t_awr : 0;
	sprintf(out[counter++], "%s             |->%s approx. wait time = %s%f%s sec \t-> from app time %s%.2f %%%s\n", CYAN, BLACK, (tmp > 0) ? RED : GREEN, tmp, BLACK, Color_Percent(100 * tmp / delta_t_agg), 100 * tmp / delta_t_agg, BLACK);
	tmp = delta_t_aw_lost;
	sprintf(out[counter++], "%s             '->%s real wait time    = %s%f%s sec \t-> from app time %s%.2f %%%s\n\n", CYAN, BLACK, (tmp > 0) ? RED : GREEN, tmp, BLACK, Color_Percent(100 * tmp / delta_t_agg), 100 * tmp / delta_t_agg, BLACK);

	// std::cout << "counter: " << counter << "  --  values: " << values << std::endl;
	for (int i = 0; i < values; i++)
	{
		std::cout << out[i];
		file << out[i];
	}
}

std::string iotime::Print_Json(bool jsonl)
{
	int len = 19;
	char buff[len][65];
	std::string out;
	char line_start[3] = {'\0', '\0', '\0'};
	char line_end[2] = {'\0', '\0'};
	if (jsonl == false)
	{
		line_end[0] = '\n';
		line_start[0] = '\t';
		line_start[1] = '\t';
		sprintf(buff[0], "\t\"%s\":{%s", "io_time", line_end);
	}
	else
		sprintf(buff[0], "\t{\"%s\":{%s", "io_time", line_end);
	
	sprintf(buff[1], "%s\"delta_t_agg\": %.2e,%s", line_start, delta_t_agg, line_end);
	sprintf(buff[2], "%s\"delta_t_agg_io\": %.2e,%s", line_start, delta_t_agg_io, line_end);
	sprintf(buff[3], "%s\"delta_t_sr\":  %.2e,%s", line_start, delta_t_sr, line_end);
	sprintf(buff[4], "%s\"delta_t_ara\": %.2e,%s", line_start, delta_t_ara, line_end);
	sprintf(buff[5], "%s\"delta_t_arr\": %.2e,%s", line_start, delta_t_arr, line_end);
	sprintf(buff[6], "%s\"delta_t_ar_lost\":%.2e,%s", line_start, delta_t_ar_lost, line_end);
	sprintf(buff[7], "%s\"delta_t_sw\":  %.2e,%s", line_start, delta_t_sw, line_end);
	sprintf(buff[8], "%s\"delta_t_awa\": %.2e,%s", line_start, delta_t_awa, line_end);
	sprintf(buff[9], "%s\"delta_t_awr\": %.2e,%s", line_start, delta_t_awr, line_end);
	sprintf(buff[10], "%s\"delta_t_aw_lost\":  %.2e,%s", line_start, delta_t_aw_lost, line_end);
	sprintf(buff[11], "%s\"delta_t_overhead\": %.2e,%s", line_start, delta_t_overhead, line_end);
	sprintf(buff[12], "%s\"delta_t_overhead_post_runtime\": %.2e,%s", line_start, delta_t_overhead_post_runtime, line_end);
	sprintf(buff[13], "%s\"delta_t_overhead_peri_runtime\": %.2e,%s", line_start, delta_t_overhead_peri_runtime, line_end);
	sprintf(buff[14], "%s\"delta_t_rank0\": %.2e,%s", line_start, delta_t_rank0, line_end);
	sprintf(buff[15], "%s\"delta_t_rank0_app\": %.2e,%s", line_start, delta_t_rank0_app, line_end);
	sprintf(buff[16], "%s\"delta_t_rank0_overhead_post_runtime\": %.2e,%s", line_start, delta_t_rank0_overhead_post_runtime, line_end);
	sprintf(buff[17], "%s\"delta_t_rank0_overhead_peri_runtime\": %.2e%s", line_start, delta_t_rank0_overhead_peri_runtime, line_end);

	if (jsonl == true)
		sprintf(buff[18], "}}\n");
	else
		sprintf(buff[18], "\t\t}\n");

	for (int i = 0; i < len; i++)
		out.append(buff[i]);

	return out;
}

const char *iotime::Color_Percent(double percentage)
{
	// generates colored output

	if (percentage < 20)
		return GREEN;
	else if (percentage < 40)
		return CYAN;
	else if (percentage < 60)
		return YELLOW;
	else
		return RED;
}
