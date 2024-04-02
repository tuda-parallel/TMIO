#include "ioprint.h"

namespace ioprint
{

	//**********************************************************************
	//*                       1. Summary
	//**********************************************************************
	/**
	 * @brief prints summary as a text file and to stdout
	 *
	 * @param processes
	 * @param read_sync
	 * @param read_async
	 * @param write_sync
	 * @param write_async
	 * @param io_time
	 */
	void Summary(int processes, statistics read_sync, statistics read_async, statistics write_sync, statistics write_async, iotime io_time)
	{

		int values = 142;
#if SHOW_AVR == 1
		values += 2 * 12;
#endif
#if SHOW_SUM == 1
		values += 4 * 12;
#endif

#if DO_CALC == 0
		values -= 6 * 3;
#if SHOW_AVR == 1
		values -= 2 * 3;
#endif
#if SHOW_SUM == 1
		values -= 4 * 3;
#endif
#endif

		char out[values][150];
		std::ofstream myfile;
		myfile.open(std::to_string(processes) + ".txt");
		std::string unit = "B";
		double unit_scale = 1;
		int counter = 0;

		sprintf(out[counter++], "\n\nSummary\n***************************\n");
		sprintf(out[counter++], "Ranks: %i \n", processes);
		sprintf(out[counter++], "\n _________________Read_________________\n");
		sprintf(out[counter++], "|\n");
		sprintf(out[counter++], "| %s_________Async_Read_________%s\n", GREEN, BLACK);
		sprintf(out[counter++], "| %s|%s Max number of ranks        : %i   \n", GREEN, BLACK, read_async.procs_io);
		iohf::Set_Unit(read_async.agg_bytes, unit, unit_scale);
		sprintf(out[counter++], "| %s|%s Total read bytes           : %.2f %s\n", GREEN, BLACK, read_async.agg_bytes * unit_scale, unit.c_str());
		iohf::Set_Unit(read_async.max_bytes, unit, unit_scale);
		sprintf(out[counter++], "| %s|%s Max read bytes per rank    : %.2f %s\n", GREEN, BLACK, read_async.max_bytes * unit_scale, unit.c_str());
		// iohf::Set_Unit(read_async.max_offset, unit, unit_scale);
		// sprintf(out[counter++], "| %s|%s Max offset over ranks      : %.2f %s\n", GREEN, BLACK, read_async.max_offset*unit_scale, unit.c_str());
		iohf::Set_Unit(read_async.max_bytes_phase, unit, unit_scale);
		sprintf(out[counter++], "| %s|%s Max transfersize           : %.2f %s\n", GREEN, BLACK, read_async.max_bytes_phase * unit_scale, unit.c_str());
		sprintf(out[counter++], "| %s|%s\n", GREEN, BLACK);
		sprintf(out[counter++], "| %s| %s ___Throughput___%s\n", GREEN, CYAN, BLACK);
		sprintf(out[counter++], "| %s| %s|%s Max # of I/O read phases per rank       : %i \n", GREEN, CYAN, BLACK, read_async.max_phases);
		sprintf(out[counter++], "| %s| %s|%s Aggregated # of I/O read phases         : %i \n", GREEN, CYAN, BLACK, read_async.agg_phases);
		sprintf(out[counter++], "| %s| %s|%s Max # of I/O read ops in phase          : %lli \n", GREEN, CYAN, BLACK, read_async.max_ops);
		sprintf(out[counter++], "| %s| %s|%s Max # of I/O overlaping phases          : %i \n", GREEN, CYAN, BLACK, read_async.throughput.n_max);
		sprintf(out[counter++], "| %s| %s|%s Max # of I/O read ops per rank          : %lli \n", GREEN, CYAN, BLACK, read_async.max_ops_rank);
		sprintf(out[counter++], "| %s| %s|%s Aggregated # of I/O read ops            : %lli \n", GREEN, CYAN, BLACK, read_async.agg_ops);
		sprintf(out[counter++], "| %s| %s|%s Weighted harmonic mean                  : %.3f MB/s\n", GREEN, CYAN, BLACK, read_async.throughput.rank_metric.avr.whmean / 1'000'000);
		sprintf(out[counter++], "| %s| %s|%s Harmonic mean                           : %.3f MB/s\n", GREEN, CYAN, BLACK, read_async.throughput.rank_metric.avr.hmean / 1'000'000);
		sprintf(out[counter++], "| %s| %s|%s Arithmetic mean                         : %.3f MB/s\n", GREEN, CYAN, BLACK, read_async.throughput.rank_metric.avr.amean / 1'000'000);
		sprintf(out[counter++], "| %s| %s|%s Median                                  : %.3f MB/s\n", GREEN, CYAN, BLACK, read_async.throughput.rank_metric.avr.median / 1'000'000);
		sprintf(out[counter++], "| %s| %s|%s Max                                     : %.3f MB/s\n", GREEN, CYAN, BLACK, read_async.throughput.rank_metric.avr.max / 1'000'000);
		sprintf(out[counter++], "| %s| %s|%s Min                                     : %.3f MB/s\n", GREEN, CYAN, BLACK, read_async.throughput.rank_metric.avr.min / 1'000'000);
		sprintf(out[counter++], "| %s| %s|%s Harmonic mean x ranks                   : %.3f MB/s\n", GREEN, CYAN, BLACK, read_async.procs_io * read_async.throughput.rank_metric.avr.hmean / 1'000'000);
		sprintf(out[counter++], "| %s| %s|%s Aritmetic mean x ranks                  : %.3f MB/s\n", GREEN, CYAN, BLACK, read_async.procs_io * read_async.throughput.rank_metric.avr.amean / 1'000'000);
#if DO_CALC > 0
		sprintf(out[counter++], "| %s| %s|%s Aggregated max over ranks               : %.3f MB/s\n", GREEN, CYAN, BLACK, read_async.throughput.app_metric.avr.agg_max / 1'000'000);
		sprintf(out[counter++], "| %s| %s|%s Max overlapping throughput              : %.3f MB/s\n", GREEN, CYAN, BLACK, read_async.throughput.app_metric.avr.max / 1'000'000);
		sprintf(out[counter++], "| %s| %s|%s Harmonic mean overlapping throughput    : %.3f MB/s\n", GREEN, CYAN, BLACK, read_async.throughput.app_metric.avr.hmean / 1'000'000);
#endif
#if SHOW_SUM == 1
		sprintf(out[counter++], "| %s| %s|  %s___Sum___%s\n", GREEN, CYAN, BLUE, BLACK);
		sprintf(out[counter++], "| %s| %s| %s|%s Weighted harmonic mean                : %.3f MB/s\n", GREEN, CYAN, BLUE, BLACK, read_async.throughput.rank_metric.sum.whmean / 1'000'000);
		sprintf(out[counter++], "| %s| %s| %s|%s Harmonic mean                         : %.3f MB/s\n", GREEN, CYAN, BLUE, BLACK, read_async.throughput.rank_metric.sum.hmean / 1'000'000);
		sprintf(out[counter++], "| %s| %s| %s|%s Arithmetic mean                       : %.3f MB/s\n", GREEN, CYAN, BLUE, BLACK, read_async.throughput.rank_metric.sum.amean / 1'000'000);
		sprintf(out[counter++], "| %s| %s| %s|%s Median                                : %.3f MB/s\n", GREEN, CYAN, BLUE, BLACK, read_async.throughput.rank_metric.sum.median / 1'000'000);
		sprintf(out[counter++], "| %s| %s| %s|%s Max                                   : %.3f MB/s\n", GREEN, CYAN, BLUE, BLACK, read_async.throughput.rank_metric.sum.max / 1'000'000);
		sprintf(out[counter++], "| %s| %s| %s|%s Min                                   : %.3f MB/s\n", GREEN, CYAN, BLUE, BLACK, read_async.throughput.rank_metric.sum.min / 1'000'000);
		sprintf(out[counter++], "| %s| %s| %s|%s Harmonic mean x ranks                 : %.3f MB/s\n", GREEN, CYAN, BLUE, BLACK, read_async.procs_io * read_async.throughput.rank_metric.sum.hmean / 1'000'000);
		sprintf(out[counter++], "| %s| %s| %s|%s Aritmetic mean x ranks                : %.3f MB/s\n", GREEN, CYAN, BLUE, BLACK, read_async.procs_io * read_async.throughput.rank_metric.sum.amean / 1'000'000);
#if DO_CALC > 0
		sprintf(out[counter++], "| %s| %s| %s|%s Aggregated max over ranks             : %.3f MB/s\n", GREEN, CYAN, BLUE, BLACK, read_async.throughput.app_metric.sum.agg_max / 1'000'000);
		sprintf(out[counter++], "| %s| %s| %s|%s Max overlapping throughput            : %.3f MB/s\n", GREEN, CYAN, BLUE, BLACK, read_async.throughput.app_metric.sum.max / 1'000'000);
		sprintf(out[counter++], "| %s| %s| %s|%s Harmonic mean overlapping throughput  : %.3f MB/s\n", GREEN, CYAN, BLUE, BLACK, read_async.throughput.app_metric.sum.hmean / 1'000'000);
#endif
#endif
		sprintf(out[counter++], "| %s|%s\n", GREEN, BLACK);

		sprintf(out[counter++], "| %s| %s ___Required Bandwidth___%s\n", GREEN, RED, BLACK);
		sprintf(out[counter++], "| %s| %s|%s Max # of I/O read phases per rank       : %i \n", GREEN, RED, BLACK, read_async.max_phases);
		sprintf(out[counter++], "| %s| %s|%s Aggregated # of I/O read phases         : %i \n", GREEN, RED, BLACK, read_async.agg_phases);
		sprintf(out[counter++], "| %s| %s|%s Max # of I/O read ops in phase          : %lli \n", GREEN, RED, BLACK, read_async.max_ops);
		sprintf(out[counter++], "| %s| %s|%s Max # of I/O overlaping phases          : %i \n", GREEN, RED, BLACK, read_async.bandwidth.n_max);
		sprintf(out[counter++], "| %s| %s|%s Max # of I/O read ops per rank          : %lli \n", GREEN, RED, BLACK, read_async.max_ops_rank);
		sprintf(out[counter++], "| %s| %s|%s Aggregated # of I/O read ops            : %lli \n", GREEN, RED, BLACK, read_async.agg_ops);
		sprintf(out[counter++], "| %s| %s|%s Weighted harmonic mean                  : %.3f MB/s\n", GREEN, RED, BLACK, read_async.bandwidth.rank_metric.sum.whmean / 1'000'000);
		sprintf(out[counter++], "| %s| %s|%s Harmonic mean                           : %.3f MB/s\n", GREEN, RED, BLACK, read_async.bandwidth.rank_metric.sum.hmean / 1'000'000);
		sprintf(out[counter++], "| %s| %s|%s Arithmetic mean                         : %.3f MB/s\n", GREEN, RED, BLACK, read_async.bandwidth.rank_metric.sum.amean / 1'000'000);
		sprintf(out[counter++], "| %s| %s|%s Median                                  : %.3f MB/s\n", GREEN, RED, BLACK, read_async.bandwidth.rank_metric.sum.median / 1'000'000);
		sprintf(out[counter++], "| %s| %s|%s Max                                     : %.3f MB/s\n", GREEN, RED, BLACK, read_async.bandwidth.rank_metric.sum.max / 1'000'000);
		sprintf(out[counter++], "| %s| %s|%s Min                                     : %.3f MB/s\n", GREEN, RED, BLACK, read_async.bandwidth.rank_metric.sum.min / 1'000'000);
		sprintf(out[counter++], "| %s| %s|%s Harmonic mean x ranks                   : %.3f MB/s\n", GREEN, RED, BLACK, read_async.procs_io * read_async.bandwidth.rank_metric.sum.hmean / 1'000'000);
		sprintf(out[counter++], "| %s| %s|%s Aritmetic mean x ranks                  : %.3f MB/s\n", GREEN, RED, BLACK, read_async.procs_io * read_async.bandwidth.rank_metric.sum.amean / 1'000'000);
#if DO_CALC > 0
		sprintf(out[counter++], "| %s| %s|%s Aggregated max over ranks               : %.3f MB/s\n", GREEN, RED, BLACK, read_async.bandwidth.app_metric.sum.agg_max / 1'000'000);
		sprintf(out[counter++], "| %s| %s|%s Max overlapping bandwidth               : %.3f MB/s\n", GREEN, RED, BLACK, read_async.bandwidth.app_metric.sum.max / 1'000'000);
		sprintf(out[counter++], "| %s| %s|%s Harmonic mean overlapping bandwidth     : %.3f MB/s\n", GREEN, RED, BLACK, read_async.bandwidth.app_metric.sum.hmean / 1'000'000);
#endif
#if SHOW_AVR == 1
		sprintf(out[counter++], "| %s| %s|  %s___Average___%s\n", GREEN, RED, BLUE, BLACK);
		sprintf(out[counter++], "| %s| %s| %s|%s Weighted harmonic mean                : %.3f MB/s\n", GREEN, RED, BLUE, BLACK, read_async.bandwidth.rank_metric.avr.whmean / 1'000'000);
		sprintf(out[counter++], "| %s| %s| %s|%s Harmonic mean                         : %.3f MB/s\n", GREEN, RED, BLUE, BLACK, read_async.bandwidth.rank_metric.avr.hmean / 1'000'000);
		sprintf(out[counter++], "| %s| %s| %s|%s Arithmetic mean                       : %.3f MB/s\n", GREEN, RED, BLUE, BLACK, read_async.bandwidth.rank_metric.avr.amean / 1'000'000);
		sprintf(out[counter++], "| %s| %s| %s|%s Median                                : %.3f MB/s\n", GREEN, RED, BLUE, BLACK, read_async.bandwidth.rank_metric.avr.median / 1'000'000);
		sprintf(out[counter++], "| %s| %s| %s|%s Max                                   : %.3f MB/s\n", GREEN, RED, BLUE, BLACK, read_async.bandwidth.rank_metric.avr.max / 1'000'000);
		sprintf(out[counter++], "| %s| %s| %s|%s Min                                   : %.3f MB/s\n", GREEN, RED, BLUE, BLACK, read_async.bandwidth.rank_metric.avr.min / 1'000'000);
		sprintf(out[counter++], "| %s| %s| %s|%s Harmonic mean x ranks                 : %.3f MB/s\n", GREEN, RED, BLUE, BLACK, read_async.procs_io * read_async.bandwidth.rank_metric.avr.hmean / 1'000'000);
		sprintf(out[counter++], "| %s| %s| %s|%s Aritmetic mean x ranks                : %.3f MB/s\n", GREEN, RED, BLUE, BLACK, read_async.procs_io * read_async.bandwidth.rank_metric.avr.amean / 1'000'000);
#if DO_CALC > 0
		sprintf(out[counter++], "| %s| %s| %s|%s Aggregated max over ranks             : %.3f MB/s\n", GREEN, RED, BLUE, BLACK, read_async.bandwidth.app_metric.avr.agg_max / 1'000'000);
		sprintf(out[counter++], "| %s| %s| %s|%s Max overlapping bandwidth             : %.3f MB/s\n", GREEN, RED, BLUE, BLACK, read_async.bandwidth.app_metric.avr.max / 1'000'000);
		sprintf(out[counter++], "| %s| %s| %s|%s Harmonic mean overlapping bandwidth   : %.3f MB/s\n", GREEN, RED, BLUE, BLACK, read_async.bandwidth.app_metric.avr.hmean / 1'000'000);
#endif
#endif
		sprintf(out[counter++], "|\n");

		sprintf(out[counter++], "|  %s_________Sync_Read_________%s\n", GREEN, BLACK);
		sprintf(out[counter++], "| %s|%s Max number of ranks        : %i   \n", GREEN, BLACK, read_sync.procs_io);
		iohf::Set_Unit(read_sync.agg_bytes, unit, unit_scale);
		sprintf(out[counter++], "| %s|%s Total read bytes           : %.2f %s\n", GREEN, BLACK, read_sync.agg_bytes * unit_scale, unit.c_str());
		iohf::Set_Unit(read_sync.max_bytes, unit, unit_scale);
		sprintf(out[counter++], "| %s|%s Max read bytes per rank    : %.2f %s\n", GREEN, BLACK, read_sync.max_bytes * unit_scale, unit.c_str());
		// iohf::Set_Unit(read_sync.max_offset, unit, unit_scale);
		// sprintf(out[counter++], "| %s|%s Max offset over ranks      : %.2f %s\n", GREEN, BLACK, read_sync.max_offset*unit_scale, unit.c_str());
		iohf::Set_Unit(read_sync.max_bytes_phase, unit, unit_scale);
		sprintf(out[counter++], "| %s|%s Max transfersize           : %.2f %s\n", GREEN, BLACK, read_sync.max_bytes_phase * unit_scale, unit.c_str());
		sprintf(out[counter++], "| %s|%s\n", GREEN, BLACK);
		sprintf(out[counter++], "| %s|%s Max # of I/O read phases per rank         : %i \n", GREEN, BLACK, read_sync.max_phases);
		sprintf(out[counter++], "| %s|%s Aggregated # of I/O read phases           : %i \n", GREEN, BLACK, read_sync.agg_phases);
		sprintf(out[counter++], "| %s|%s Max # of I/O read ops in phase            : %lli \n", GREEN, BLACK, read_sync.max_ops);
		sprintf(out[counter++], "| %s|%s Max # of I/O overlaping phases            : %i \n", GREEN, BLACK, read_sync.throughput.n_max);
		sprintf(out[counter++], "| %s|%s Max # of I/O read ops per rank            : %lli \n", GREEN, BLACK, read_sync.max_ops_rank);
		sprintf(out[counter++], "| %s|%s Aggregated # of I/O read ops              : %lli \n", GREEN, BLACK, read_sync.agg_ops);
		sprintf(out[counter++], "| %s|%s Weighted harmonic mean                    : %.3f MB/s\n", GREEN, BLACK, read_sync.throughput.rank_metric.avr.whmean / 1'000'000);
		sprintf(out[counter++], "| %s|%s Harmonic mean                             : %.3f MB/s\n", GREEN, BLACK, read_sync.throughput.rank_metric.avr.hmean / 1'000'000);
		sprintf(out[counter++], "| %s|%s Arithmetic mean                           : %.3f MB/s\n", GREEN, BLACK, read_sync.throughput.rank_metric.avr.amean / 1'000'000);
		sprintf(out[counter++], "| %s|%s Median                                    : %.3f MB/s\n", GREEN, BLACK, read_sync.throughput.rank_metric.avr.median / 1'000'000);
		sprintf(out[counter++], "| %s|%s Max                                       : %.3f MB/s\n", GREEN, BLACK, read_sync.throughput.rank_metric.avr.max / 1'000'000);
		sprintf(out[counter++], "| %s|%s Min                                       : %.3f MB/s\n", GREEN, BLACK, read_sync.throughput.rank_metric.avr.min / 1'000'000);
		sprintf(out[counter++], "| %s|%s Harmonic mean x ranks                     : %.3f MB/s\n", GREEN, BLACK, read_sync.procs_io * read_sync.throughput.rank_metric.avr.hmean / 1'000'000);
		sprintf(out[counter++], "| %s|%s Aritmetic mean x ranks                    : %.3f MB/s\n", GREEN, BLACK, read_sync.procs_io * read_sync.throughput.rank_metric.avr.amean / 1'000'000);
#if DO_CALC > 0
		sprintf(out[counter++], "| %s|%s Aggregated max over ranks                 : %.3f MB/s\n", GREEN, BLACK, read_sync.throughput.app_metric.avr.agg_max / 1'000'000);
		sprintf(out[counter++], "| %s|%s Max overlapping throughput                : %.3f MB/s\n", GREEN, BLACK, read_sync.throughput.app_metric.avr.max / 1'000'000);
		sprintf(out[counter++], "| %s|%s Harmonic mean throughput                  : %.3f MB/s\n", GREEN, BLACK, read_sync.throughput.app_metric.avr.hmean / 1'000'000);
#endif
#if SHOW_SUM == 1
		sprintf(out[counter++], "| %s|  %s___Sum___%s\n", GREEN, BLUE, BLACK);
		sprintf(out[counter++], "| %s| %s|%s Weighted  harmonic mean                 : %.3f MB/s\n", GREEN, BLUE, BLACK, read_sync.throughput.rank_metric.sum.whmean / 1'000'000);
		sprintf(out[counter++], "| %s| %s|%s Harmonic mean                           : %.3f MB/s\n", GREEN, BLUE, BLACK, read_sync.throughput.rank_metric.sum.hmean / 1'000'000);
		sprintf(out[counter++], "| %s| %s|%s Arithmetic mean                         : %.3f MB/s\n", GREEN, BLUE, BLACK, read_sync.throughput.rank_metric.sum.amean / 1'000'000);
		sprintf(out[counter++], "| %s| %s|%s Median                                  : %.3f MB/s\n", GREEN, BLUE, BLACK, read_sync.throughput.rank_metric.sum.median / 1'000'000);
		sprintf(out[counter++], "| %s| %s|%s Max                                     : %.3f MB/s\n", GREEN, BLUE, BLACK, read_sync.throughput.rank_metric.sum.max / 1'000'000);
		sprintf(out[counter++], "| %s| %s|%s Min                                     : %.3f MB/s\n", GREEN, BLUE, BLACK, read_sync.throughput.rank_metric.sum.min / 1'000'000);
		sprintf(out[counter++], "| %s| %s|%s Harmonic mean x ranks                   : %.3f MB/s\n", GREEN, BLUE, BLACK, read_sync.procs_io * read_sync.throughput.rank_metric.sum.hmean / 1'000'000);
		sprintf(out[counter++], "| %s| %s|%s Aritmetic mean x ranks                  : %.3f MB/s\n", GREEN, BLUE, BLACK, read_sync.procs_io * read_sync.throughput.rank_metric.sum.amean / 1'000'000);
#if DO_CALC > 0
		sprintf(out[counter++], "| %s| %s|%s Aggregated max over ranks               : %.3f MB/s\n", GREEN, BLUE, BLACK, read_sync.throughput.app_metric.sum.agg_max / 1'000'000);
		sprintf(out[counter++], "| %s| %s|%s Max overlapping throughput              : %.3f MB/s\n", GREEN, BLUE, BLACK, read_sync.throughput.app_metric.sum.max / 1'000'000);
		sprintf(out[counter++], "| %s| %s|%s Harmonic mean overlapping throughput    : %.3f MB/s\n", GREEN, BLUE, BLACK, read_sync.throughput.app_metric.sum.hmean / 1'000'000);
#endif
#endif
		sprintf(out[counter++], "\n\n");

		sprintf(out[counter++], "\n _________________Write_________________\n");
		sprintf(out[counter++], "|\n");
		sprintf(out[counter++], "| %s_________Async_Write_________%s\n", GREEN, BLACK);
		sprintf(out[counter++], "| %s|%s Max number of ranks        : %i   \n", GREEN, BLACK, write_async.procs_io);
		iohf::Set_Unit(write_async.agg_bytes, unit, unit_scale);
		sprintf(out[counter++], "| %s|%s Total written bytes        : %.2f %s\n", GREEN, BLACK, write_async.agg_bytes * unit_scale, unit.c_str());
		iohf::Set_Unit(write_async.max_bytes, unit, unit_scale);
		sprintf(out[counter++], "| %s|%s Max written bytes per rank : %.2f %s\n", GREEN, BLACK, write_async.max_bytes * unit_scale, unit.c_str());
		// iohf::Set_Unit(write_async.max_offset, unit, unit_scale);
		// sprintf(out[counter++], "| %s|%s Max offset over ranks      : %.2f %s\n", GREEN, BLACK, write_async.max_offset*unit_scale, unit.c_str());
		iohf::Set_Unit(write_async.max_bytes_phase, unit, unit_scale);
		sprintf(out[counter++], "| %s|%s Max transfersize           : %.2f %s\n", GREEN, BLACK, write_async.max_bytes_phase * unit_scale, unit.c_str());
		sprintf(out[counter++], "| %s|%s\n", GREEN, BLACK);
		sprintf(out[counter++], "| %s| %s ___Throughput___%s\n", GREEN, CYAN, BLACK);
		sprintf(out[counter++], "| %s| %s|%s Max # of I/O write phases per rank      : %i \n", GREEN, CYAN, BLACK, write_async.max_phases);
		sprintf(out[counter++], "| %s| %s|%s Aggregated # of I/O write phases        : %i \n", GREEN, CYAN, BLACK, write_async.agg_phases);
		sprintf(out[counter++], "| %s| %s|%s Max # of I/O write ops in phase         : %lli \n", GREEN, CYAN, BLACK, write_async.max_ops);
		sprintf(out[counter++], "| %s| %s|%s Max # of I/O overlaping phases          : %i \n", GREEN, CYAN, BLACK, write_async.throughput.n_max);
		sprintf(out[counter++], "| %s| %s|%s Max # of I/O write ops per rank         : %lli \n", GREEN, CYAN, BLACK, write_async.max_ops_rank);
		sprintf(out[counter++], "| %s| %s|%s Aggregated # of I/O write ops           : %lli \n", GREEN, CYAN, BLACK, write_async.agg_ops);
		sprintf(out[counter++], "| %s| %s|%s Weighted harmonic mean                  : %.3f MB/s\n", GREEN, CYAN, BLACK, write_async.throughput.rank_metric.avr.whmean / 1'000'000);
		sprintf(out[counter++], "| %s| %s|%s Harmonic mean                           : %.3f MB/s\n", GREEN, CYAN, BLACK, write_async.throughput.rank_metric.avr.hmean / 1'000'000);
		sprintf(out[counter++], "| %s| %s|%s Arithmetic mean                         : %.3f MB/s\n", GREEN, CYAN, BLACK, write_async.throughput.rank_metric.avr.amean / 1'000'000);
		sprintf(out[counter++], "| %s| %s|%s Median                                  : %.3f MB/s\n", GREEN, CYAN, BLACK, write_async.throughput.rank_metric.avr.median / 1'000'000);
		sprintf(out[counter++], "| %s| %s|%s Max                                     : %.3f MB/s\n", GREEN, CYAN, BLACK, write_async.throughput.rank_metric.avr.max / 1'000'000);
		sprintf(out[counter++], "| %s| %s|%s Min                                     : %.3f MB/s\n", GREEN, CYAN, BLACK, write_async.throughput.rank_metric.avr.min / 1'000'000);
		sprintf(out[counter++], "| %s| %s|%s Harmonic mean x ranks                   : %.3f MB/s\n", GREEN, CYAN, BLACK, write_async.procs_io * write_async.throughput.rank_metric.avr.hmean / 1'000'000);
		sprintf(out[counter++], "| %s| %s|%s Aritmetic mean x ranks                  : %.3f MB/s\n", GREEN, CYAN, BLACK, write_async.procs_io * write_async.throughput.rank_metric.avr.amean / 1'000'000);
#if DO_CALC > 0
		sprintf(out[counter++], "| %s| %s|%s Aggregated max over ranks               : %.3f MB/s\n", GREEN, CYAN, BLACK, write_async.throughput.app_metric.avr.agg_max / 1'000'000);
		sprintf(out[counter++], "| %s| %s|%s Max overlapping throughput              : %.3f MB/s\n", GREEN, CYAN, BLACK, write_async.throughput.app_metric.avr.max / 1'000'000);
		sprintf(out[counter++], "| %s| %s|%s Harmonic mean overlapping throughput    : %.3f MB/s\n", GREEN, CYAN, BLACK, write_async.throughput.app_metric.avr.hmean / 1'000'000);
#endif
#if SHOW_SUM == 1
		sprintf(out[counter++], "| %s| %s|  %s___Sum___%s\n", GREEN, CYAN, BLUE, BLACK);
		sprintf(out[counter++], "| %s| %s| %s|%s Weighted harmonic mean                : %.3f MB/s\n", GREEN, CYAN, BLUE, BLACK, write_async.throughput.rank_metric.sum.whmean / 1'000'000);
		sprintf(out[counter++], "| %s| %s| %s|%s Harmonic mean                         : %.3f MB/s\n", GREEN, CYAN, BLUE, BLACK, write_async.throughput.rank_metric.sum.hmean / 1'000'000);
		sprintf(out[counter++], "| %s| %s| %s|%s Arithmetic mean                       : %.3f MB/s\n", GREEN, CYAN, BLUE, BLACK, write_async.throughput.rank_metric.sum.amean / 1'000'000);
		sprintf(out[counter++], "| %s| %s| %s|%s Median                                : %.3f MB/s\n", GREEN, CYAN, BLUE, BLACK, write_async.throughput.rank_metric.sum.median / 1'000'000);
		sprintf(out[counter++], "| %s| %s| %s|%s Max                                   : %.3f MB/s\n", GREEN, CYAN, BLUE, BLACK, write_async.throughput.rank_metric.sum.max / 1'000'000);
		sprintf(out[counter++], "| %s| %s| %s|%s Min                                   : %.3f MB/s\n", GREEN, CYAN, BLUE, BLACK, write_async.throughput.rank_metric.sum.min / 1'000'000);
		sprintf(out[counter++], "| %s| %s| %s|%s Harmonic mean x ranks                 : %.3f MB/s\n", GREEN, CYAN, BLUE, BLACK, write_async.procs_io * write_async.throughput.rank_metric.sum.hmean / 1'000'000);
		sprintf(out[counter++], "| %s| %s| %s|%s Aritmetic mean x ranks                : %.3f MB/s\n", GREEN, CYAN, BLUE, BLACK, write_async.procs_io * write_async.throughput.rank_metric.sum.amean / 1'000'000);
#if DO_CALC > 0
		sprintf(out[counter++], "| %s| %s| %s|%s Aggregated max over ranks             : %.3f MB/s\n", GREEN, CYAN, BLUE, BLACK, write_async.throughput.app_metric.sum.agg_max / 1'000'000);
		sprintf(out[counter++], "| %s| %s| %s|%s Max overlapping throughput            : %.3f MB/s\n", GREEN, CYAN, BLUE, BLACK, write_async.throughput.app_metric.sum.max / 1'000'000);
		sprintf(out[counter++], "| %s| %s| %s|%s Harmonic mean overlapping throughput  : %.3f MB/s\n", GREEN, CYAN, BLUE, BLACK, write_async.throughput.app_metric.sum.hmean / 1'000'000);
#endif
#endif
		sprintf(out[counter++], "| %s|%s\n", GREEN, BLACK);

		sprintf(out[counter++], "| %s| %s ___Required Bandwidth___%s\n", GREEN, RED, BLACK);
		sprintf(out[counter++], "| %s| %s|%s Max # of I/O write phases per rank      : %i \n", GREEN, RED, BLACK, write_async.max_phases);
		sprintf(out[counter++], "| %s| %s|%s Aggregated # of I/O write phases        : %i \n", GREEN, RED, BLACK, write_async.agg_phases);
		sprintf(out[counter++], "| %s| %s|%s Max # of I/O write ops in phase         : %lli \n", GREEN, RED, BLACK, write_async.max_ops);
		sprintf(out[counter++], "| %s| %s|%s Max # of I/O overlaping phases          : %i \n", GREEN, RED, BLACK, write_async.bandwidth.n_max);
		sprintf(out[counter++], "| %s| %s|%s Max # of I/O write ops per rank         : %lli \n", GREEN, RED, BLACK, write_async.max_ops_rank);
		sprintf(out[counter++], "| %s| %s|%s Aggregated # of I/O write ops           : %lli \n", GREEN, RED, BLACK, write_async.agg_ops);
		sprintf(out[counter++], "| %s| %s|%s Weighted harmonic mean                  : %.3f MB/s\n", GREEN, RED, BLACK, write_async.bandwidth.rank_metric.sum.whmean / 1'000'000);
		sprintf(out[counter++], "| %s| %s|%s Harmonic mean                           : %.3f MB/s\n", GREEN, RED, BLACK, write_async.bandwidth.rank_metric.sum.hmean / 1'000'000);
		sprintf(out[counter++], "| %s| %s|%s Arithmetic mean                         : %.3f MB/s\n", GREEN, RED, BLACK, write_async.bandwidth.rank_metric.sum.amean / 1'000'000);
		sprintf(out[counter++], "| %s| %s|%s Median                                  : %.3f MB/s\n", GREEN, RED, BLACK, write_async.bandwidth.rank_metric.sum.median / 1'000'000);
		sprintf(out[counter++], "| %s| %s|%s Max                                     : %.3f MB/s\n", GREEN, RED, BLACK, write_async.bandwidth.rank_metric.sum.max / 1'000'000);
		sprintf(out[counter++], "| %s| %s|%s Min                                     : %.3f MB/s\n", GREEN, RED, BLACK, write_async.bandwidth.rank_metric.sum.min / 1'000'000);
		sprintf(out[counter++], "| %s| %s|%s Harmonic mean x ranks                   : %.3f MB/s\n", GREEN, RED, BLACK, write_async.procs_io * write_async.bandwidth.rank_metric.sum.hmean / 1'000'000);
		sprintf(out[counter++], "| %s| %s|%s Aritmetic mean x ranks                  : %.3f MB/s\n", GREEN, RED, BLACK, write_async.procs_io * write_async.bandwidth.rank_metric.sum.amean / 1'000'000);
#if DO_CALC > 0
		sprintf(out[counter++], "| %s| %s|%s Aggregated max over ranks               : %.3f MB/s\n", GREEN, RED, BLACK, write_async.bandwidth.app_metric.sum.agg_max / 1'000'000);
		sprintf(out[counter++], "| %s| %s|%s Max overlapping bandwidth               : %.3f MB/s\n", GREEN, RED, BLACK, write_async.bandwidth.app_metric.sum.max / 1'000'000);
		sprintf(out[counter++], "| %s| %s|%s Harmonic mean overlapping bandwidth     : %.3f MB/s\n", GREEN, RED, BLACK, write_async.bandwidth.app_metric.sum.hmean / 1'000'000);
#endif
#if SHOW_AVR == 1
		sprintf(out[counter++], "| %s| %s|  %s___Average___%s\n", GREEN, RED, BLUE, BLACK);
		sprintf(out[counter++], "| %s| %s| %s|%s Weighted harmonic mean                : %.3f MB/s\n", GREEN, RED, BLUE, BLACK, write_async.bandwidth.rank_metric.avr.whmean / 1'000'000);
		sprintf(out[counter++], "| %s| %s| %s|%s Harmonic mean                         : %.3f MB/s\n", GREEN, RED, BLUE, BLACK, write_async.bandwidth.rank_metric.avr.hmean / 1'000'000);
		sprintf(out[counter++], "| %s| %s| %s|%s Arithmetic mean                       : %.3f MB/s\n", GREEN, RED, BLUE, BLACK, write_async.bandwidth.rank_metric.avr.amean / 1'000'000);
		sprintf(out[counter++], "| %s| %s| %s|%s Median                                : %.3f MB/s\n", GREEN, RED, BLUE, BLACK, write_async.bandwidth.rank_metric.avr.median / 1'000'000);
		sprintf(out[counter++], "| %s| %s| %s|%s Max                                   : %.3f MB/s\n", GREEN, RED, BLUE, BLACK, write_async.bandwidth.rank_metric.avr.max / 1'000'000);
		sprintf(out[counter++], "| %s| %s| %s|%s Min                                   : %.3f MB/s\n", GREEN, RED, BLUE, BLACK, write_async.bandwidth.rank_metric.avr.min / 1'000'000);
		sprintf(out[counter++], "| %s| %s| %s|%s Harmonic mean x ranks                 : %.3f MB/s\n", GREEN, RED, BLUE, BLACK, write_async.procs_io * write_async.bandwidth.rank_metric.avr.hmean / 1'000'000);
		sprintf(out[counter++], "| %s| %s| %s|%s Aritmetic mean x ranks                : %.3f MB/s\n", GREEN, RED, BLUE, BLACK, write_async.procs_io * write_async.bandwidth.rank_metric.avr.amean / 1'000'000);
#if DO_CALC > 0
		sprintf(out[counter++], "| %s| %s| %s|%s Aggregated max over ranks             : %.3f MB/s\n", GREEN, RED, BLUE, BLACK, write_async.bandwidth.app_metric.avr.agg_max / 1'000'000);
		sprintf(out[counter++], "| %s| %s| %s|%s Max overlapping bandwidth             : %.3f MB/s\n", GREEN, RED, BLUE, BLACK, write_async.bandwidth.app_metric.avr.max / 1'000'000);
		sprintf(out[counter++], "| %s| %s| %s|%s Harmonic mean overlapping bandwidth   : %.3f MB/s\n", GREEN, RED, BLUE, BLACK, write_async.bandwidth.app_metric.avr.hmean / 1'000'000);
#endif
#endif
		sprintf(out[counter++], "|\n");

		sprintf(out[counter++], "|  %s_________Sync_Write_________%s\n", GREEN, BLACK);
		sprintf(out[counter++], "| %s|%s Max number of ranks        : %i   \n", GREEN, BLACK, write_sync.procs_io);
		iohf::Set_Unit(write_sync.agg_bytes, unit, unit_scale);
		sprintf(out[counter++], "| %s|%s Total written bytes        : %.2f %s\n", GREEN, BLACK, write_sync.agg_bytes * unit_scale, unit.c_str());
		iohf::Set_Unit(write_sync.max_bytes, unit, unit_scale);
		sprintf(out[counter++], "| %s|%s Max written bytes per rank : %.2f %s\n", GREEN, BLACK, write_sync.max_bytes * unit_scale, unit.c_str());
		// iohf::Set_Unit(write_sync.max_offset, unit, unit_scale);
		// sprintf(out[counter++], "| %s|%s Max offset over ranks      : %.2f %s\n", GREEN, BLACK, write_sync.max_offset*unit_scale, unit.c_str());
		iohf::Set_Unit(write_sync.max_bytes_phase, unit, unit_scale);
		sprintf(out[counter++], "| %s|%s Max transfersize           : %.2f %s\n", GREEN, BLACK, write_sync.max_bytes_phase * unit_scale, unit.c_str());
		sprintf(out[counter++], "| %s|%s\n", GREEN, BLACK);
		sprintf(out[counter++], "| %s|%s Max # of I/O write phases per rank        : %i \n", GREEN, BLACK, write_sync.max_phases);
		sprintf(out[counter++], "| %s|%s Aggregated # of I/O write phases          : %i \n", GREEN, BLACK, write_sync.agg_phases);
		sprintf(out[counter++], "| %s|%s Max # of I/O write ops in phase           : %lli \n", GREEN, BLACK, write_sync.max_ops);
		sprintf(out[counter++], "| %s|%s Max # of I/O overlaping phases            : %i \n", GREEN, BLACK, write_sync.throughput.n_max);
		sprintf(out[counter++], "| %s|%s Max # of I/O write ops per rank           : %lli \n", GREEN, BLACK, write_sync.max_ops_rank);
		sprintf(out[counter++], "| %s|%s Aggregated # of I/O write ops             : %lli \n", GREEN, BLACK, write_sync.agg_ops);
		sprintf(out[counter++], "| %s|%s Weighted harmonic mean                    : %.3f MB/s\n", GREEN, BLACK, write_sync.throughput.rank_metric.avr.whmean / 1'000'000);
		sprintf(out[counter++], "| %s|%s Harmonic mean                             : %.3f MB/s\n", GREEN, BLACK, write_sync.throughput.rank_metric.avr.hmean / 1'000'000);
		sprintf(out[counter++], "| %s|%s Arithmetic mean                           : %.3f MB/s\n", GREEN, BLACK, write_sync.throughput.rank_metric.avr.amean / 1'000'000);
		sprintf(out[counter++], "| %s|%s Median                                    : %.3f MB/s\n", GREEN, BLACK, write_sync.throughput.rank_metric.avr.median / 1'000'000);
		sprintf(out[counter++], "| %s|%s Max                                       : %.3f MB/s\n", GREEN, BLACK, write_sync.throughput.rank_metric.avr.max / 1'000'000);
		sprintf(out[counter++], "| %s|%s Min                                       : %.3f MB/s\n", GREEN, BLACK, write_sync.throughput.rank_metric.avr.min / 1'000'000);
		sprintf(out[counter++], "| %s|%s Harmonic mean x ranks                     : %.3f MB/s\n", GREEN, BLACK, write_sync.procs_io * write_sync.throughput.rank_metric.avr.hmean / 1'000'000);
		sprintf(out[counter++], "| %s|%s Aritmetic mean x ranks                    : %.3f MB/s\n", GREEN, BLACK, write_sync.procs_io * write_sync.throughput.rank_metric.avr.amean / 1'000'000);
#if DO_CALC > 0
		sprintf(out[counter++], "| %s|%s Aggregated max over ranks                 : %.3f MB/s\n", GREEN, BLACK, write_sync.throughput.app_metric.avr.agg_max / 1'000'000);
		sprintf(out[counter++], "| %s|%s Max overlapping throughput                : %.3f MB/s\n", GREEN, BLACK, write_sync.throughput.app_metric.avr.max / 1'000'000);
		sprintf(out[counter++], "| %s|%s Harmonic mean overlapping throughput      : %.3f MB/s\n", GREEN, BLACK, write_sync.throughput.app_metric.avr.hmean / 1'000'000);
#endif
#if SHOW_SUM == 1
		sprintf(out[counter++], "| %s|  %s___Sum___%s\n", GREEN, BLUE, BLACK);
		sprintf(out[counter++], "| %s| %s|%s Weighted harmonic mean                  : %.3f MB/s\n", GREEN, BLUE, BLACK, write_sync.throughput.rank_metric.sum.whmean / 1'000'000);
		sprintf(out[counter++], "| %s| %s|%s Harmonic mean                           : %.3f MB/s\n", GREEN, BLUE, BLACK, write_sync.throughput.rank_metric.sum.hmean / 1'000'000);
		sprintf(out[counter++], "| %s| %s|%s Arithmetic mean                         : %.3f MB/s\n", GREEN, BLUE, BLACK, write_sync.throughput.rank_metric.sum.amean / 1'000'000);
		sprintf(out[counter++], "| %s| %s|%s Median                                  : %.3f MB/s\n", GREEN, BLUE, BLACK, write_sync.throughput.rank_metric.sum.median / 1'000'000);
		sprintf(out[counter++], "| %s| %s|%s Max                                     : %.3f MB/s\n", GREEN, BLUE, BLACK, write_sync.throughput.rank_metric.sum.max / 1'000'000);
		sprintf(out[counter++], "| %s| %s|%s Min                                     : %.3f MB/s\n", GREEN, BLUE, BLACK, write_sync.throughput.rank_metric.sum.min / 1'000'000);
		sprintf(out[counter++], "| %s| %s|%s Harmonic mean x ranks                   : %.3f MB/s\n", GREEN, BLUE, BLACK, write_sync.procs_io * write_sync.throughput.rank_metric.sum.hmean / 1'000'000);
		sprintf(out[counter++], "| %s| %s|%s Aritmetic mean x ranks                  : %.3f MB/s\n", GREEN, BLUE, BLACK, write_sync.procs_io * write_sync.throughput.rank_metric.sum.amean / 1'000'000);
#if DO_CALC > 0
		sprintf(out[counter++], "| %s| %s|%s Aggregated max over ranks               : %.3f MB/s\n", GREEN, BLUE, BLACK, write_sync.throughput.app_metric.sum.agg_max / 1'000'000);
		sprintf(out[counter++], "| %s| %s|%s Max overlapping throughput              : %.3f MB/s\n", GREEN, BLUE, BLACK, write_sync.throughput.app_metric.sum.max / 1'000'000);
		sprintf(out[counter++], "| %s| %s|%s Harmonic mean overlapping throughput    : %.3f MB/s\n", GREEN, BLUE, BLACK, write_sync.throughput.app_metric.sum.hmean / 1'000'000);
#endif
#endif
		sprintf(out[counter++], "\n");

		// std::cout << "counter:" << counter <<"  values:"<< values << std::endl;

		for (int i = 0; i < values; i++)
		{
			std::cout << out[i];
			myfile << out[i];
		}
		io_time.print(myfile);

		myfile.close();
		// iohf::Disp(buff_actual_async_write, n, "tmp[i] = ");
	}

	//**********************************************************************
	//*                       2. Jsonlines
	//**********************************************************************
	/**
	 * @brief Create json file
	 *
	 * @param processes
	 * @param read_sync
	 * @param read_async
	 * @param write_sync
	 * @param write_async
	 * @param io_time
	 */
	void Jsonl(int processes, statistics read_sync, statistics read_async, statistics write_sync, statistics write_async, iotime io_time)
	{
		std::ofstream file;
		static bool first_time = true;
		if (first_time)
		{
			file.open(std::to_string(processes) + ".jsonl");
			first_time = false;
		}
		else
		{
			file.open(std::to_string(processes) + ".jsonl", std::ios_base::app);
		}

		std::string print = Format_Json(read_sync, "read_sync", false, true);
		print.append(Format_Json(read_async, "read_async_t", false, true));
		print.append(Format_Json(read_async, "read_async_b", true, true));
		print.append(Format_Json(write_async, "write_async_t", false, true));
		print.append(Format_Json(write_async, "write_async_b", true, true));
		print.append(Format_Json(write_sync, "write_sync", false, true));
		print.append(io_time.Print_Json(true));
		file << print << std::flush;
		file.flush();
		file.close();
	}

	//**********************************************************************
	//*                       2. Json
	//**********************************************************************
	/**
	 * @brief Create json file
	 *
	 * @param processes
	 * @param read_sync
	 * @param read_async
	 * @param write_sync
	 * @param write_async
	 * @param io_time
	 */
	void Json(int processes, statistics read_sync, statistics read_async, statistics write_sync, statistics write_async, iotime io_time)
	{
		std::ofstream file;
		file.open(std::to_string(processes) + ".json");
		file << "{\n";
		std::string print = Format_Json(read_sync, "read_sync");
		print.append(Format_Json(read_async, "read_async_t"));
		print.append(Format_Json(read_async, "read_async_b", true));
		print.append(Format_Json(write_async, "write_async_t"));
		print.append(Format_Json(write_async, "write_async_b", true));
		// print.append(Format_Json(write_sync, "write_sync", ""));
		print.append(Format_Json(write_sync, "write_sync"));
		print.append(io_time.Print_Json());
		print.append("}\n");
		file << print;
		file.close();
	}

	std::string Format_Json(statistics data, std::string mode, bool req, bool jsonl)
	{
		char buff[27][50];
		std::string out;
		int counter = 0;
		char line_start[2] = {'\0', '\0'};
		char line_end[2] = {'\0', '\0'};
		double unit_scale = 1; // in Bytes or Bytes/s ;
		if (jsonl == false)
		{
			line_end[0] = '\n';
			line_start[0] = '\t';
		}
		if (jsonl == true)
			sprintf(buff[counter++], "\t{\"%s\":{%s", mode.c_str(), line_end);
		else
			sprintf(buff[counter++], "\t\"%s\":{%s", mode.c_str(), line_end);
		sprintf(buff[counter++], "%s\"total_bytes\": %.2e,%s", line_start, data.agg_bytes * unit_scale, line_end);
		sprintf(buff[counter++], "%s\"max_bytes_per_rank\": %.2e,%s", line_start, data.max_bytes * unit_scale, line_end);
		// sprintf(buff[counter++], "%s\"max_offset_over_ranks\": %.2e,%s",line_start ,data.max_offset*unit_scale, line_end);
		sprintf(buff[counter++], "%s\"max_bytes_per_phase\": %.2e,%s", line_start, data.max_bytes_phase * unit_scale, line_end);
		sprintf(buff[counter++], "%s\"max_io_phases_per_rank\": %i,%s", line_start, data.max_phases, line_end);
		sprintf(buff[counter++], "%s\"total_io_phases\": %i,%s", line_start, data.agg_phases, line_end);
		sprintf(buff[counter++], "%s\"max_io_ops_in_phase\": %lli,%s", line_start, data.max_ops, line_end);
		sprintf(buff[counter++], "%s\"max_io_ops_per_rank\": %lli,%s", line_start, data.max_ops_rank, line_end);
		sprintf(buff[counter++], "%s\"total_io_ops\": %lli,%s", line_start, data.agg_ops, line_end);
		sprintf(buff[counter++], "%s\"number_of_ranks\": %i,%s", line_start, data.procs_io, line_end);
		sprintf(buff[counter++], "%s\"bandwidth\": {%s", line_start, line_end);
		// FIXME show these only for exact
		sprintf(buff[counter++], "%s%s\"weighted_harmonic_mean\": %.2e,%s", line_start, line_start, (req) ? data.bandwidth.rank_metric.sum.whmean * unit_scale : data.throughput.rank_metric.avr.whmean * unit_scale, line_end);
		sprintf(buff[counter++], "%s%s\"harmonic_mean\": %.2e,%s", line_start, line_start, (req) ? data.bandwidth.rank_metric.sum.hmean * unit_scale : data.throughput.rank_metric.avr.hmean * unit_scale, line_end);
		sprintf(buff[counter++], "%s%s\"arithmetic_mean\": %.2e,%s", line_start, line_start, (req) ? data.bandwidth.rank_metric.sum.amean * unit_scale : data.throughput.rank_metric.avr.amean * unit_scale, line_end);
		sprintf(buff[counter++], "%s%s\"median\": %.2e,%s", line_start, line_start, (req) ? data.bandwidth.rank_metric.sum.median * unit_scale : data.throughput.rank_metric.avr.median * unit_scale, line_end);
		sprintf(buff[counter++], "%s%s\"max\": %.2e,%s", line_start, line_start, (req) ? data.bandwidth.rank_metric.sum.max * unit_scale : data.throughput.rank_metric.avr.max * unit_scale, line_end);
		sprintf(buff[counter++], "%s%s\"min\": %.2e", line_start, line_start, (req) ? data.bandwidth.rank_metric.sum.min * unit_scale : data.throughput.rank_metric.avr.min * unit_scale);
#if DO_CALC > 0
		sprintf(buff[counter++], ",%s%s%s\"app\": %.2e", line_end, line_start, line_start, (req) ? data.bandwidth.app_metric.sum.max * unit_scale : data.throughput.app_metric.avr.max * unit_scale);
		sprintf(buff[counter++], ",%s%s%s\"appH\": %.2e", line_end, line_start, line_start, (req) ? data.bandwidth.app_metric.sum.hmean * unit_scale : data.throughput.app_metric.avr.hmean * unit_scale);
// #else
// values -=2;
#endif
		if (req)
		{
#if SHOW_AVR == 1
			sprintf(buff[counter++], ",%s%s%s\"weighted_avr_harmonic_mean\": %.2e", line_end, line_start, line_start, data.throughput.rank_metric.avr.whmean * unit_scale);
			sprintf(buff[counter++], ",%s%s%s\"harmonic_avr_mean\": %.2e", line_end, line_start, line_start, data.throughput.rank_metric.avr.hmean * unit_scale);
			sprintf(buff[counter++], ",%s%s%s\"arithmetic_avr_mean\": %.2e", line_end, line_start, line_start, data.throughput.rank_metric.avr.amean * unit_scale);
			sprintf(buff[counter++], ",%s%s%s\"median_avr\": %.2e", line_end, line_start, line_start, data.throughput.rank_metric.avr.median * unit_scale);
			sprintf(buff[counter++], ",%s%s%s\"max_avr\": %.2e", line_end, line_start, line_start, data.throughput.rank_metric.avr.max * unit_scale);
			sprintf(buff[counter++], ",%s%s%s\"min_avr\": %.2e", line_end, line_start, line_start, data.throughput.rank_metric.avr.min * unit_scale);
#if DO_CALC > 0
			sprintf(buff[counter++], ",%s%s%s\"app_avr\": %.2e", line_end, line_start, line_start, data.throughput.app_metric.avr.max * unit_scale);
			sprintf(buff[counter++], ",%s%s%s\"appH_avr\": %.2e", line_end, line_start, line_start, data.throughput.app_metric.avr.hmean * unit_scale);
// #else
// values -=2;
#endif
// #else
//                         values -= 8;
#endif
		}
		else
		{
#if SHOW_SUM == 1
			sprintf(buff[counter++], ",%s%s%S\"weighted_sum_harmonic_mean\": %.2e", line_end, line_start, line_start data.bandwidth.rank_metric.sum.whmean * unit_scale);
			sprintf(buff[counter++], ",%s%s%S\"harmonic_sum_mean\": %.2e", line_end, line_start, line_start data.bandwidth.rank_metric.sum.hmean * unit_scale);
			sprintf(buff[counter++], ",%s%s%S\"arithmetic_sum_mean\": %.2e", line_end, line_start, line_start data.bandwidth.rank_metric.sum.amean * unit_scale);
			sprintf(buff[counter++], ",%s%s%S\"median_sum\": %.2e", line_end, line_start, line_start data.bandwidth.rank_metric.sum.median * unit_scale);
			sprintf(buff[counter++], ",%s%s%S\"max_sum\": %.2e", line_end, line_start, line_start data.bandwidth.rank_metric.sum.max * unit_scale);
			sprintf(buff[counter++], ",%s%s%S\"min_sum\": %.2e", line_end, line_start, line_start data.bandwidth.rank_metric.sum.min * unit_scale);
#if DO_CALC > 0
			sprintf(buff[counter++], ",%s%s%S\"app_sum\": %.2e", line_end, line_start, line_start data.bandwidth.app_metric.sum.max * unit_scale);
			sprintf(buff[counter++], ",%s%s%S\"appH_sum\": %.2e", line_end, line_start, line_start data.bandwidth.app_metric.sum.hmean * unit_scale);
// #else
// values -=2;
#endif
// #else
//                         values -= 8;
#endif
		}

		for (int i = 0; i < counter; i++)
			out.append(buff[i]);

		// print b to output string
		int n = (data.max_ops > 10) ? 5 : data.max_ops;
		n = (data.max_ops == 1) ? data.procs_io : data.max_ops;

#if ALL_SAMPLES > 0 && DO_CALC > 0
		std::string tmp_0;
		std::string tmp_1;
		std::string tmp_2;
		if (req)
		{

			tmp_0 = Print_Series(data.bandwidth_sum_phase, data.phase_overlap_req.size(), unit_scale, n, "\"b_overlap_sum\": [", "]", jsonl);
			tmp_1 = Print_Series(data.bandwidth_avr_phase, data.phase_overlap_req.size(), unit_scale, n, "\"b_overlap_avr\": [", "]", jsonl);
			tmp_2 = Print_Series(data.n_overlap_req, data.phase_overlap_req.size(), 1, n, "\"n_overlap\": [", "]", jsonl);
		}
		else
		{
			tmp_0 = Print_Series(data.throughput_sum_phase, data.phase_overlap_act.size(), unit_scale, n, "\"b_overlap_sum\": [", "]", jsonl);
			tmp_1 = Print_Series(data.throughput_avr_phase, data.phase_overlap_act.size(), unit_scale, n, "\"b_overlap_avr\": [", "]", jsonl);
			tmp_2 = Print_Series(data.n_overlap_act, data.phase_overlap_act.size(), 1, n, "\"n_overlap\": [", "]", jsonl);
		}
		out.append(tmp_0);
		out.append(tmp_1);
		out.append(tmp_2);
#endif

#if ALL_SAMPLES > 1 && DO_CALC > 0
		std::string tmp_4;
		if (req)
			tmp_4 = Print_Series(&(data.phase_time_req[0]), data.phase_time_req.size(), 1, n, "\"t_overlap\": [", "]", jsonl);
		else
			tmp_4 = Print_Series(&(data.phase_time_act[0]), data.phase_time_act.size(), 1, n, "\"t_overlap\": [", "]", jsonl);
		out.append(tmp_4);
#endif

#if ALL_SAMPLES > 2
		std::string tmp_5;
		if (req)
		{
			tmp_5 = Print_Series(data.all_data, "B_sum", data.agg_phases, unit_scale, n, "\"b_rank_sum\": [", "]", jsonl);
			// #if SHOW_AVR == 1
			tmp_5 = tmp_5 + Print_Series(data.all_data, "B_avr", data.agg_phases, unit_scale, n, "\"b_rank_avr\": [", "]", jsonl);
			// #endif
		}
		else
		{
			tmp_5 = Print_Series(data.all_data, "T_avr", data.agg_phases, unit_scale, n, "\"b_rank_avr\": [", "]", jsonl);
			// #if SHOW_SUM == 1
			tmp_5 = tmp_5 + Print_Series(data.all_data, "T_sum", data.agg_phases, unit_scale, n, "\"b_rank_sum\": [", "]", jsonl);
			// #endif
		}
		out.append(tmp_5);
// #else
//                 out.append("\t\t\"b\": []");
#endif

#if ALL_SAMPLES > 3
		std::string tmp_6;
		std::string tmp_7;
		if (req)
		{
			tmp_6 = Print_Series(data.all_data, "t_start", data.agg_phases, 1, n, "\"t_rank_s\": [", "]", jsonl);
			tmp_7 = Print_Series(data.all_data, "t_end_req", data.agg_phases, 1, n, "\"t_rank_e\": [", "]", jsonl);
		}
		else
		{
			tmp_6 = Print_Series(data.all_data, "t_start", data.agg_phases, 1, n, "\"t_rank_s\": [", "]", jsonl);
			tmp_7 = Print_Series(data.all_data, "t_end_act", data.agg_phases, 1, n, "\"t_rank_e\": [", "]", jsonl);
		}

		out.append(tmp_6);
		out.append(tmp_7);
#endif

#if ALL_SAMPLES > 4
		std::string tmp_8;
		std::string tmp_9;
		std::string tmp_10;
		if (req)
		{
			tmp_8 = Print_Series(data.all_b, data.agg_ops, unit_scale, n, "\"b_ind\": [", "]", jsonl);
			tmp_9 = Print_Series(data.all_t_req_s, data.agg_ops, 1, n, "\"t_ind_s\": [", "]", jsonl);
			tmp_10 = Print_Series(data.all_t_req_e, data.agg_ops, 1, n, "\"t_ind_e\": [", "]", jsonl);
		}
		else
		{
			tmp_8 = Print_Series(data.all_t, data.agg_ops, unit_scale, n, "\"b_ind\": [", "]", jsonl);
			tmp_9 = Print_Series(data.all_t_act_s, data.agg_ops, 1, n, "\"t_ind_s\": [", "]", jsonl);
			tmp_10 = Print_Series(data.all_t_act_e, data.agg_ops, 1, n, "\"t_ind_e\": [", "]", jsonl);
		}

		out.append(tmp_8);
		out.append(tmp_9);
		out.append(tmp_10);
#endif

		if (jsonl == true)
			out.append("}}}\n");
		else
			out.append("\n\t\t}\n\t\t},\n\n");
		return out;
	}

	//! ----------------------- Phase Calculation ------------------------------

	//**********************************************************************
	//*                       1. Print_Series
	//**********************************************************************

	template <class T>
	std::string Print_Series(T ptr, int loops, double unit_scale, int n, std::string start, std::string end, bool jsonl)
	{
		std::string s;
		std::string out;
		out.append(",");
		if (jsonl == false)
			out.append("\n\t\t");
		out.append(start);
		for (int i = 0; i < loops; i++)
		{
			if (i % n == 0 && jsonl == false)
				out.append("\n\t\t\t");

			s = isnan(*(ptr + i)) ? "NaN" : std::to_string(*(ptr + i) * unit_scale);

			if (i == loops - 1)
				out.append(" " + s);
			else
				out.append(" " + s + ",");
		}
		out.append(end);
		return out;
	}
	template std::string Print_Series<int *>(int *, int, double, int, std::string, std::string, bool);
	template std::string Print_Series<double *>(double *, int, double, int, std::string, std::string, bool);

	//**********************************************************************
	//*                       2. Print_Series (overload)
	//**********************************************************************

	std::string Print_Series(collect *all_data, std::string field, int loops, double unit_scale, int n, std::string start, std::string end, bool jsonl)
	{

		std::string s;
		std::string out;
		out.append(",");
		if (jsonl == false)
			out.append("\n\t\t");
		out.append(start);
		for (int i = 0; i < loops; i++)
		{
			if (i % n == 0 && jsonl == false)
				out.append("\n\t\t\t");

			s = isnan(all_data[i].get(field)) ? "NaN" : std::to_string(all_data[i].get(field) * unit_scale);

			if (i == loops - 1)
				out.append(" " + s);
			else
				out.append(" " + s + ",");
		}
		out.append(end);
		return out;
	}

	void Binary(int processes, statistics read_sync, statistics read_async, statistics write_sync, statistics write_async, iotime io_time)
	{

		static int chunk = 0;
#if FILE_FORMAT == 1
		// Generate a large string
		std::string print = Format_Json(read_sync, "read_sync", false, true);
		print.append(Format_Json(read_async, "read_async_t", false, true));
		print.append(Format_Json(read_async, "read_async_b", true, true));
		print.append(Format_Json(write_async, "write_async_t", false, true));
		print.append(Format_Json(write_async, "write_async_b", true, true));
		print.append(Format_Json(write_sync, "write_sync", false, true));
		print.append(io_time.Print_Json(true));

		std::string name = std::to_string(processes) + ".bin" + "_chunk_" + std::to_string(chunk);
		auto flag = std::ios_base::binary;
		std::ofstream file(name, flag);
		if (file.is_open())
		{
			file.write(print.c_str(), print.size());
			file.close();
		}

#elif FILE_FORMAT == 2 || FILE_FORMAT == 3 // MSGPack & ZMQ
		msgpack::sbuffer buffer;
		// Pack
		msgpack::pack(buffer, read_async);
		msgpack::pack(buffer, read_sync);
		msgpack::pack(buffer, write_async);
		msgpack::pack(buffer, write_sync);
		msgpack::pack(buffer, io_time);

#if FILE_FORMAT == 2   // MSGPACK

		//? 1) One file:
		std::string name = std::to_string(processes) + ".msgpack";
		static bool first_time = true;
		auto flag = std::ios_base::binary | std::ios_base::app;
		if (first_time)
		{
			flag = std::ios_base::binary;
			first_time = false;
		}
		//? 2) Several files
		// std::string name = std::to_string(processes) + ".msgpack" + "_chunk_" + std::to_string(chunk) ;
		// auto flag = std::ios_base::binary;
		std::ofstream file(name, flag);

		// Write the serialized data to the file
		file.write(buffer.data(), buffer.size());
		file.close();
#elif FILE_FORMAT == 3 // ZMQ
		zmq::context_t context(1);
		zmq::socket_t socket(context, ZMQ_PUSH);

		static bool first_time = true;
		static std::string port = "tcp://127.0.0.1:5555";
		if (first_time)
		{
			std::ifstream myfile("ftio_port");
			if (myfile.good() && myfile.is_open())
			{
				while (getline(myfile, port))
				{
					std::cout << port << '\n';
				}
				myfile.close();
			}
			first_time = false;
		}
		socket.bind(port);
		zmq::message_t message(buffer.size());
		memcpy(message.data(), buffer.data(), buffer.size());
		socket.send(message, zmq::send_flags::none);
#endif
#endif
		chunk++;
	}
}
