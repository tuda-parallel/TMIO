#include "statistics.h"

statistics::statistics(void)
{

	procs_io = 0;
	procs = 0;
	phases_of_ranks = NULL;
	max_phases = 0;
	agg_phases = 0;
	max_bytes = 0;
	agg_bytes = 0;
	max_transfersize = 0;
}

statistics::statistics(collect *c, int *n, int rank, int procs, bool w_or_r, bool flag_req)
{

	this->flag_req = flag_req;
	if (rank == 0)
	{
		// assign collect object
		all_data = c;
		this->w_or_r = w_or_r;

		this->procs = procs;	  // all ranks
		procs_io = 0; // ranks that did I/O

#if DFT >= 1
		dft_time = 0;
#endif

		phases_of_ranks = n;
		max_phases = 0; // maximum phases over ranks
		agg_phases = 0; // total number of phases

		max_ops = 0;	  // maximum number of I/O operations in  all  phases
		max_ops_rank = 0; // maximum number of I/O operations for all ranks
		long long tmp_max_ops_rank = 0;
		agg_ops = 0; // aggregated number of I/O operations

		max_bytes = 0; // maximum bytes transfered by a ranks
		long long tmp_max_bytes = 0;
		max_transfersize = 0; // maximum bytes transfered during phase
		agg_bytes = 0;		  // aggregated bytes for entire application

		int counter = 0;
		for (int i = 0; i < procs; i++)
		{

			// find ranks that did I/O
			if (phases_of_ranks[i] > 0)
				procs_io++;

			// maximum phases over ranks
			if (phases_of_ranks[i] > max_phases)
				max_phases = phases_of_ranks[i];

			// aggregated over all ranks
			agg_phases += phases_of_ranks[i];

			tmp_max_ops_rank = 0;
			tmp_max_bytes = 0;
			for (int j = 0; j < phases_of_ranks[i]; j++)
			{
				tmp_max_ops_rank += all_data[counter].n_op;
				if (tmp_max_ops_rank > max_ops_rank)
					max_ops_rank = tmp_max_ops_rank;

				tmp_max_bytes += all_data[counter].data;
				if (tmp_max_bytes > max_bytes)
					max_bytes = tmp_max_bytes;

				if (all_data[counter].n_op > max_ops)
					max_ops = all_data[counter].n_op;

				// maximum bytes over all phases
				if (all_data[counter].data > max_transfersize)
					max_transfersize = all_data[counter].data;

				agg_ops += all_data[counter].n_op;
				agg_bytes += all_data[counter].data;

				counter++;
			}
		}
	}
}

statistics::~statistics()
{
	// keep it clean
}

void statistics::Clean(void)
{

// free(all_data);
#if DO_CALC > 0
	free(throughput_avr_phase);
	free(throughput_sum_phase);
	free(n_overlap_act);
	if (flag_req)
	{
		free(bandwidth_avr_phase);
		free(bandwidth_sum_phase);
		free(n_overlap_req);
	}
#endif
	free(all_t);
	free(all_t_act_s);
	free(all_t_act_e);
	if (flag_req)
	{
		free(all_b);
		free(all_t_req_s);
		free(all_t_req_e);
	}
}

//! ----------------------- Statistics Core ------------------------------

//**********************************************************************
//*                       1. Compute
//**********************************************************************
/**
 * @brief computes the statistics
 *
 */
void statistics::Compute(void)
{

	//? (2) Remove selected phases
	//?----------------------------------------------
//* removes from the collected bandwidthes n last writes and m first writes (n=SKIP_LAST_WRITE m=SKIP_FIRST_WRITE)
#if SKIP_LAST_WRITE > 0 || SKIP_FIRST_WRITE > 0
	if (w_or_r)
		Remove_Phase(SKIP_FIRST_WRITE, SKIP_LAST_WRITE);
#endif
		//* removes from the collected bandwidthes n last read and m first read (n=SKIP_LAST_READ m=SKIP_FIRST_READ)
#if SKIP_LAST_READ > 0 || SKIP_FIRST_READ > 0
	if (!w_or_r)
		Remove_Phase(SKIP_FIRST_READ, SKIP_LAST_READ);
#endif

	//? (2) collective anaylsis of captured bandwidth
	//?----------------------------------------------
	//* find overlapping phases of different ranks
	// iohf::Overlap(phase_overlap, phase_time, t_start, t_end, phases_of_ranks, agg_phases, procs);
	Overlap(phase_overlap_act, phase_time_act, "t_end_act");
	if (flag_req)
		Overlap(phase_overlap_req, phase_time_req, "t_end_req");

	//* find vector of overlaps (n_vec_overlap) ;
	n_overlap_act = iohf::N_Phase(phase_overlap_act);
	if (flag_req)
		n_overlap_req = iohf::N_Phase(phase_overlap_req);

	//* project bandwidthes (average and sum) on the overlap result
	throughput_avr_phase = Phase_Bandwidth(phase_overlap_act, "T_avr");
	throughput_sum_phase = Phase_Bandwidth(phase_overlap_act, "T_sum");
	if (flag_req)
	{
		bandwidth_avr_phase = Phase_Bandwidth(phase_overlap_req, "B_avr");
		bandwidth_sum_phase = Phase_Bandwidth(phase_overlap_req, "B_sum");
	}

	//? (3) Compute metrics (app and rank metrics)
	//?------------------------------------
	Compute_Metrics();

#if DFT >= 1
	std::complex<double> *X = freq_analysis::Dft(throughput_avr_phase, &phase_time_act[0], phase_overlap_act.size(), flag_req, w_or_r, procs, dft_time, FREQ); // 200
	free(X);
#endif
}

//! ----------------------- Phase Calculation ------------------------------

//**********************************************************************
//*                       1. Remove_Phase
//**********************************************************************
/**
 * @brief Removes selected number of phases from the strart|end of each rank. If rank has only
 * one phase, nothing is removed
 *
 * @param s number of phases to remove from the start for each rank
 * @param e number of phases to remove from the end of each rank
 *
 *
 */
void statistics::Remove_Phase(int s, int e)
{

	iohf::Function_Debug(__PRETTY_FUNCTION__);
	int counter = 0;
	// std::cout << "agg_phases is " << agg_phases << ", procs_io is " << procs_io << std::endl;
	if (agg_phases > 1 && procs_io > 0 && ceil(agg_phases / procs_io) > 1 && (e > 0 || s > 0))
	{

		for (int i = 0; i < procs_io; i++)
		{
			for (int j = e; j > 0; j--)
			{
				all_data[counter + phases_of_ranks[i] - j].T_avr = std::numeric_limits<double>::quiet_NaN();
				all_data[counter + phases_of_ranks[i] - j].T_sum = std::numeric_limits<double>::quiet_NaN();
				if (flag_req)
				{
					all_data[counter + phases_of_ranks[i] - j].B_avr = std::numeric_limits<double>::quiet_NaN();
					all_data[counter + phases_of_ranks[i] - j].B_sum = std::numeric_limits<double>::quiet_NaN();
				}
			}
			for (int j = 0; j < s; j++)
			{
				all_data[counter + j].T_avr = std::numeric_limits<double>::quiet_NaN();
				all_data[counter + j].T_sum = std::numeric_limits<double>::quiet_NaN();
				if (flag_req)
				{
					all_data[counter + j].B_avr = std::numeric_limits<double>::quiet_NaN();
					all_data[counter + j].B_sum = std::numeric_limits<double>::quiet_NaN();
				}
			}
			counter += phases_of_ranks[i];
		}
		std::cout << "removed: " << s << " from start & " << e << " from the end\n";
	}
}

//**********************************************************************
//*                       2. Overlap
//**********************************************************************
/**
 * @brief   function for finding phases for the entire job
 *
 * @param phase_overlap [out] vector of vectors storing overlaping phases of all ranks
 * @param phase_time [out] vector storing start and end of the overlapping regions
 * @param mode [in] indicates if req or act is used.
 * Supported modes are: "t_end_req" and "t_end_act"
 * default mode is "t_end_act"
 */
void statistics::Overlap(std::vector<std::vector<int>> &phase_overlap, std::vector<double> &phase_time, std::string mode)
{
	iohf::Function_Debug(__PRETTY_FUNCTION__);
	std::vector<int> stack;
	stack.reserve(agg_phases);
	// #ifdef OPENMP
	// std::cout << "max number of OpenMP threads: ";
	// std::cout << omp_get_max_threads() << std::endl;
	// #endif

	int k_s = 0;
	int k_e = 0;

	// sort start and end times of the phases
	int *id_s = iohf::Sort_With_Index(all_data, agg_phases);
	int *id_e = iohf::Sort_With_Index(all_data, agg_phases, mode);

	int n = 0;
	// Disp(t_start, agg_phases, "t_start = ");
	// Disp(t_end, agg_phases, "t_end = ");

	while (k_s < agg_phases || k_e < agg_phases)
	{

#if HDEBUG >= 2
		std::cout << "k_e = " << k_e << ", id_e[k_e] = " << id_e[k_e] << ",    t_end[id_e[k_e]] = " << all_data[id_e[k_e]].get(mode) << std::endl;
		if (k_s != agg_phases)
			std::cout << "k_s = " << k_s << ", id_s[k_s] = " << id_s[k_s] << ", t_start[id_s[k_s]] = " << all_data[id_s[k_s]].t_start << std::endl;
#endif

		//? case 1: end happend before start -> remove the phase from the stack
		// if it is the last phase, application I/O phase ended
		if (k_s == agg_phases || (all_data[id_e[k_e]].get(mode) < all_data[id_s[k_s]].t_start))
		{
			if (stack.size() == 1)
				stack.pop_back();
			else
			{
				for (unsigned int j = 0; j < stack.size(); j++)
				{
#if HDEBUG >= 2
					std::cout << "\t( " << stack[j] << " == " << id_e[k_e] << " )?";
#endif
					if (stack[j] == id_e[k_e])
					{
						// old
						// stack.erase(stack.begin() + j);
						// std::cout << "old method: " << stack[j] << std:: endl;
						std::swap(stack[j], stack.back());
						stack.pop_back();
#if HDEBUG >= 2
						std::cout << "-> removed " << id_e[k_e] << std::endl;
#endif
						break;
					}
				}
			}
//* debug: show current phase
#if HDEBUG >= 1
			std::cout << "phase " << id_e[k_e] << " -> end" << std::endl;
			std::cout << "stack: ";
			for (int i = 0; i < stack.size(); i++)
				std::cout << " " << stack[i] << " ";
			std::cout << std::endl;
#endif
// track phase_time if ALL_SAMPLE is set
#if ALL_SAMPLES > 2
			phase_time.push_back(all_data[id_e[k_e]].get(mode));
			// std::cout << "all_data[id_e[k_e]]."<< mode<<" " << all_data[id_e[k_e]].get(mode)<<  std::endl;
#endif

			k_e++;
		}
		//? case 2: start is before end -> overlap. Add the overlaping phase to stack
		else
		{
			stack.push_back(id_s[k_s]);
#if HDEBUG >= 1
			std::cout << "phase " << id_s[k_s] << " -> start" << std::endl;
			std::cout << "stack: ";
			for (int i = 0; i < stack.size(); i++)
				std::cout << " " << stack[i] << " ";
			std::cout << std::endl;
#endif
#if ALL_SAMPLES > 2
			phase_time.push_back(all_data[id_s[k_s]].t_start);
			// std::cout << "all_data[id_e[k_e]].t_start "<< all_data[id_s[k_s]].t_start<<  std::endl;
#endif
			k_s++;
		}

		// snapshot current stack to phase_overlapult
		phase_overlap.push_back(std::vector<int>());
		phase_overlap[n++] = stack;
	}

// plot graph if flag is set
#if OVERLAP_GRAPH > 0
	iohf::Overlap_Graph(phase_overlap, agg_phases);
#endif

#if HDEBUG > 0
	if (agg_phases > 0)
	{
		// print sorted and unsorted start and end phase_time
		for (int i = 0; i < agg_phases; i++)
			std::cout << "all_data[i" << i << "].t_start = " << all_data[i].t_start << "\t"
					  << "sorted all_data[" << i << "].t_start = " << all_data[id_s[i]].t_start << std::endl;
		for (int i = 0; i < agg_phases; i++)
			std::cout << "all_data[i" << i << "].t_end  = " << all_data[i].get(mode) << "\t"
					  << "sorted all_data[" << i << "].t_end = " << all_data[id_e[i]].get(mode) << std::endl;

		// print all phases

		for (int rowD = 0; rowD < phase_overlap.size(); rowD++)
		{
			std::cout << std::endl
					  << "Phase " << rowD << ": ";
			for (int colD = 0; colD < phase_overlap[rowD].size(); colD++)
				std::cout << phase_overlap[rowD][colD] << " ";
		}

		std::cout << "\n\n";
	}
#endif

	free(id_s);
	free(id_e);
}

//**********************************************************************
//*                       3. Phase_Bandwidth
//**********************************************************************
/**
 * @brief maps the bandwidths to the overlapping regions. The bandwidths of the selected \e mode
 * are aggregated according to the overlapping of the regions
 *
 * @param res
 * @param mode
 * @return double*
 */
double *statistics::Phase_Bandwidth(std::vector<std::vector<int>> &res, std::string mode)
{
	// ALL_SAMPLES > 1 print also zeroes bandwidth values, else skip
	iohf::Function_Debug(__PRETTY_FUNCTION__);

#if ALL_SAMPLES > 1 // also print zero bandiwdth
	double *out = (double *)malloc(res.size() * sizeof(double));
	// add all bandidth in phases
	for (unsigned int i = 0; i < res.size(); i++)
	{
		out[i] = 0;
		for (unsigned int j = 0; j < res[i].size(); j++)
		{

			out[i] += all_data[res[i][j]].get(mode);
		}

		// if (res[i].size() == 0)
		//   std::cout << "Application phase "<< ++app_phase << " over" << std::endl;
	}
#else
	// find number of non-empty overlaps (n_overla)
	int n = iohf::Non_Empty(res);
	double *out = (double *)malloc(n * sizeof(double));
	n = 0;
	// add all bandidth in phases
	for (int i = 0; i < res.size(); i++)
	{
		if (!res[i].empty())
		{
			out[n] = 0;
			for (int j = 0; j < res[i].size(); j++)
			{
				if (!isnan(all_data[res[i][j]].get(mode)))
				{
					if (j == 0)
						out[n] = all_data[res[i][j]].get(mode);
					else
						out[n] += all_data[res[i][j]].get(mode);
				}
			}

			n++;
		}
	}
#endif

	return out;
}

//**********************************************************************
//*                       4. Phase_Detection
//**********************************************************************
/**
 * @brief Detect phases at application level.
 * todo: for example peak occurence of dominant frequency (at amplitude)
 */
void statistics::Phase_Detection(void)
{
	// int n = 0;
}

//**********************************************************************
//*                       5. Gather_Ind_Bandwidth
//**********************************************************************
/**
 * @brief Gather bandwidth of individual operations of each rank
 *
 */
void statistics::Gather_Ind_Bandwidth(int rank, int procs, std::vector<double> t, std::vector<double> b, std::vector<double> t_act_s, std::vector<double> t_act_e, std::vector<double> t_req_s, std::vector<double> t_req_e, MPI_Comm IO_WORLD)
{
	int *n_ind = NULL;
	int counter = 0;
	if (rank == 0)
	{
		n_ind = (int *)malloc(sizeof(int) * procs);
		for (int i = 0; i < procs; i++)
		{
			n_ind[i] = 0;
			for (int j = 0; j < phases_of_ranks[i]; j++)
			{
				n_ind[i] += all_data[counter].n_op;
				counter++;
			}
		}
	}

	if (rank == 0)
	{
		all_t = (double *)malloc(sizeof(double) * agg_ops);
		all_t_act_s = (double *)malloc(sizeof(double) * agg_ops);
		all_t_act_e = (double *)malloc(sizeof(double) * agg_ops);
	}

	iohf::Gather_Summary(t.size(), procs, rank, all_t, t, n_ind, IO_WORLD);
	iohf::Gather_Summary(t_act_s.size(), procs, rank, all_t_act_s, t_act_s, n_ind, IO_WORLD);
	iohf::Gather_Summary(t_act_e.size(), procs, rank, all_t_act_e, t_act_e, n_ind, IO_WORLD);
	if (flag_req)
	{
		if (rank == 0)
		{
			all_b = (double *)malloc(sizeof(double) * agg_ops);
			all_t_req_s = (double *)malloc(sizeof(double) * agg_ops);
			all_t_req_e = (double *)malloc(sizeof(double) * agg_ops);
		}

		iohf::Gather_Summary(b.size(), procs, rank, all_b, b, n_ind, IO_WORLD);
		iohf::Gather_Summary(t_req_s.size(), procs, rank, all_t_req_s, t_req_s, n_ind, IO_WORLD);
		iohf::Gather_Summary(t_req_e.size(), procs, rank, all_t_req_e, t_req_e, n_ind, IO_WORLD);
	}

	free(n_ind);
}

//! ----------------------- Metric Calculation ------------------------------
//**********************************************************************
//*                       1. Compute_Metrics
//**********************************************************************
/**
 * @brief computes Metrics and application as well as on rank level.
 *
 */
void statistics::Compute_Metrics(void)
{

	//? (1) Metrics for entire application
	//?------------------------------------
	Compute_App_Metrics();

	//? (2) Metrics over ranks (except agg_max)
	//?------------------------------------
	Compute_Rank_Metrics();
}

//**********************************************************************
//*                       2. Compute_App_Metrics
//**********************************************************************
/**
 * @brief computes the metrics from the overlaping phases at application level
 */
void statistics::Compute_App_Metrics(void)
{
	//! comput max/hmean of overlapping bandwidth (max for required bandwidth and hmean for bandwidth)
	//* 1) if all samples are printed, the overlap algo in iohf::Phase_Bandwidth introduces zero must be removed for calculating the harmonic mean.
	//* 2) if not all samples are printed, reduce the amount of throughput_sum_phase/throughput_avr_phase  by removing zero regions (n_tmp = iohf::Non_Empty)
#if ALL_SAMPLES > 1
	throughput.app_metric.avr.max = iohf::Max(throughput_avr_phase, phase_overlap_act.size());
	throughput.app_metric.avr.hmean = iohf::Harmonic_Mean_Non_Zero(throughput_avr_phase, phase_overlap_act.size());
#if SHOW_SUM == 1
	throughput.app_metric.sum.max = iohf::Max(throughput_sum_phase, phase_overlap_act.size());
	throughput.app_metric.sum.hmean = iohf::Harmonic_Mean_Non_Zero(throughput_sum_phase, phase_overlap_act.size());
#endif

	if (flag_req)
	{
		bandwidth.app_metric.sum.max = iohf::Max(bandwidth_sum_phase, phase_overlap_req.size());
		bandwidth.app_metric.sum.hmean = iohf::Harmonic_Mean_Non_Zero(bandwidth_sum_phase, phase_overlap_req.size());
#if SHOW_AVR == 1
		bandwidth.app_metric.avr.max = iohf::Max(bandwidth_avr_phase, phase_overlap_req.size());
		bandwidth.app_metric.avr.hmean = iohf::Harmonic_Mean_Non_Zero(bandwidth_avr_phase, phase_overlap_req.size());
#endif
	}

#else
	int n_tmp = iohf::Non_Empty(phase_overlap_act);
	throughput.app_metric.avr.max = iohf::Max(throughput_avr_phase, n_tmp);
	throughput.app_metric.avr.hmean = iohf::Harmonic_Mean(throughput_avr_phase, n_tmp);
#if SHOW_SUM == 1
	throughput.app_metric.sum.max = iohf::Max(throughput_sum_phase, n_tmp);
	throughput.app_metric.sum.hmean = iohf::Harmonic_Mean(throughput_sum_phase, n_tmp);
#endif
	if (flag_req)
	{
		n_tmp = iohf::Non_Empty(phase_overlap_req);
		bandwidth.app_metric.sum.max = iohf::Max(bandwidth_sum_phase, n_tmp);
		bandwidth.app_metric.sum.hmean = iohf::Harmonic_Mean(bandwidth_sum_phase, n_tmp);
#if SHOW_AVR == 1
		bandwidth.app_metric.avr.max = iohf::Max(bandwidth_avr_phase, n_tmp);
		bandwidth.app_metric.avr.hmean = iohf::Harmonic_Mean(bandwidth_avr_phase, n_tmp);
#endif
	}
#endif

	// maximum number of overlapping phases of the ranks inside an application phase
	throughput.n_max = (n_overlap_act == NULL) ? 0 : iohf::Max(n_overlap_act, phase_overlap_act.size());
	if (flag_req)
		bandwidth.n_max = (n_overlap_req == NULL) ? 0 : iohf::Max(n_overlap_req, phase_overlap_req.size());
}

//**********************************************************************
//*                       3. Compute_Rank_Metrics
//**********************************************************************
/**
 * @brief computes the metrics from the overlaping phases as well as from the individual phases of the ranks
 *
 */
void statistics::Compute_Rank_Metrics(void)
{
	Compute_Rank_Metrics_Core(true, true);
#if SHOW_SUM == 1
	Compute_Rank_Metrics_Core(true, false);
#endif

	if (flag_req)
	{
		Compute_Rank_Metrics_Core(false, false);
#if SHOW_AVR == 1
		Compute_Rank_Metrics_Core(false, true);
#endif
	}
}

//**********************************************************************
//*                       4. Compute_Rank_Metrics
//**********************************************************************
/**
 * @brief computes the metrics from the overlaping phases as well as from the individual phases of the ranks
 *
 * @param t_or_b true: use throughout/namdwidth for calculation (throughout | false: bandwidth).
 * @param avr_or_sum   use sum/avr during for the calculation (true: avr | false: sum).
 */
void statistics::Compute_Rank_Metrics_Core(bool t_or_b, bool avr_or_sum)
{

	//* rank metrics: ittertate over all phases of all ranks
	std::string field;
	if (t_or_b)
		field = "T";
	else
		field = "B";

	if (avr_or_sum)
		field = field + "_avr";
	else
		field = field + "_sum";

	double hmean = 0;
	double amean = 0;
	double whmean = 0;
	double median = 0;
	double max = 0;
	double min = 0;
	double agg_max = 0;
	long long bytes = 0;
	double tmp = 0;

	if (agg_phases != 0)
	{
		int N = 0;
		min = std::numeric_limits<double>::max();

		int i = 0;
		for (int p = 0; p < procs; p++)
		{
			for (int j = 0; j < phases_of_ranks[p]; j++)
			{
				// for(int i = 0; i < agg_phases; i++){
				tmp = all_data[i].get(field);
				if (!isnan(tmp))
				{
					N++;
					if (tmp != 0)
					{
						//? harmonic mean
						hmean += 1 / tmp;
						//? wighted harmonic mean
						whmean += all_data[i].data / tmp;
					}
					//? arithmetic mean
					amean += tmp;
					//? bytes transferred
					bytes += all_data[i].data;
					//? min
					min = (min > tmp) ? tmp : min;
					//? max
					max = (tmp > max) ? tmp : max;
				}
				i++;
				agg_max += max;
			}
		}

		hmean = (hmean == 0) ? 0 : N / hmean;
		amean = (N == 0) ? 0 : amean / N;
		whmean = (whmean == 0) ? 0 : bytes / whmean; // don't use agg_bytes as phases can be removed (see ioflags.h)

//? median
#if SKIP_LAST_WRITE > 0 || SKIP_FIRST_READ > 0 || SKIP_LAST_READ > 0 || SKIP_FIRST_WRITE > 0
		double arr[N];
		int counter = 0;
		for (int i = 0; i < N; i++)
			if (!isnan(all_data[i].get(field)))
				arr[counter++] = all_data[i].get(field);
		std::sort(arr, arr + counter);
		median = (counter % 2 == 0) ? (arr[counter / 2 - 1] + arr[counter / 2]) / 2 : arr[counter / 2];
#else
		int *id = iohf::Sort_With_Index(all_data, N, field);
		median = (N % 2 == 0) ? (all_data[id[N / 2 - 1]].get(field) + all_data[id[N / 2]].get(field)) / 2 : all_data[id[N / 2]].get(field);
		free(id);
#endif

		iometrics *ptr0;
		core_rank_metrics *ptr_rank_metric;
		core_app_metrics *ptr_app_metric;

		//*assign:
		if (t_or_b)
			ptr0 = &throughput;
		else
			ptr0 = &bandwidth;

		if (avr_or_sum)
		{
			ptr_rank_metric = &ptr0->rank_metric.avr;
			ptr_app_metric = &ptr0->app_metric.avr;
		}
		else
		{
			ptr_rank_metric = &ptr0->rank_metric.sum;
			ptr_app_metric = &ptr0->app_metric.sum;
		}

		//? assign
		ptr_rank_metric->hmean = hmean;
		ptr_rank_metric->amean = amean;
		ptr_rank_metric->whmean = whmean;
		ptr_rank_metric->median = median;
		ptr_rank_metric->max = max;
		ptr_rank_metric->min = min;
		ptr_app_metric->agg_max = agg_max;
	}
}

//! ----------------------- Time Information ------------------------------

//**********************************************************************
//*                       1. Lost_Time
//**********************************************************************
/**
 * @brief Calculates the real lost time by (1) finding for each I/O operation the
 * waiting time (throughtput_time - Bandwidth_time) (2) followed by aggregating the results.
 *
 * @return lost time (type: double)
 */
double statistics::Lost_Time()
{
	double t = 0;
	double tmp = 0;

	for (int i = 0; i < agg_phases; i++)
	{
		tmp = (all_data[i].t_end_act - all_data[i].t_start) - (all_data[i].t_end_req - all_data[i].t_start);
		tmp = tmp > 0 ? tmp : 0;
		t += tmp;
	}

	return t;
}

//**********************************************************************
//*                       2. Total_Time
//**********************************************************************
/**
 * @brief Computes the aggregated time over all ranks. No input needed
 *
 * @return double
 */
double statistics::Total_Time(std::string mode)
{
	double t = 0;

	for (int i = 0; i < agg_phases; i++)
		t += all_data[i].get(mode) - all_data[i].t_start;

	return t;
}


//! ----------------------- Print binary ------------------------------
//**********************************************************************
//*                       1. msgpack_pack
//**********************************************************************
/**
 * @brief Pack data in msgpack format
 *
 * @return double
 */
#ifdef MSGPACK
// Serialize the class manually
void statistics::msgpack_pack(msgpack::packer<msgpack::sbuffer> &pk) const
{
	std::string s = "";
	if (w_or_r)
		s += "write_";
	else
		s += "read_";
	if (flag_req)
		s += "async";
	else
		s += "sync";

	// pack the next following together in an array
	int n = 14;
	pk.pack_array(n);
	pk.pack(s);
	pk.pack(flag_req);
	pk.pack(w_or_r);
	pk.pack(agg_bytes);
	pk.pack(max_bytes);
	pk.pack(max_transfersize);
	pk.pack(max_phases);
	pk.pack(agg_phases);
	pk.pack(max_ops);
	pk.pack(max_ops_rank);
	pk.pack(agg_ops);
	pk.pack(procs_io);
	pk.pack(procs);
	
	pk.pack_array(agg_phases);
	for (int i = 0; i < agg_phases; i++)
	{
		all_data[i].msgpack_pack(pk);
    }

// #if ALL_SAMPLES > 4
// 	if (flag_req){
// 		pk.pack_array(6);
// 		pk.pack_array(agg_ops);
// 		for (int i = 0; i < agg_ops; i++)
// 			pk.pack(all_b[i]);
// 		pk.pack_array(agg_ops);
// 		for (int i = 0; i < agg_ops; i++)
// 			pk.pack(all_t_req_s[i]);
// 		pk.pack_array(agg_ops);
// 		for (int i = 0; i < agg_ops; i++)
// 			pk.pack(all_t_req_e[i]);
// 	}
// 	else
// 		pk.pack_array(3);
	
// 	pk.pack_array(agg_ops);
// 	for (int i = 0; i < agg_ops; i++)
// 		pk.pack(all_t[i]);
// 	pk.pack_array(agg_ops);
// 	for (int i = 0; i < agg_ops; i++)
// 		pk.pack(all_t_act_s[i]);
// 	pk.pack_array(agg_ops);
// 	for (int i = 0; i < agg_ops; i++)
// 		pk.pack(all_t_act_e[i]);
// #endif
		
	

}
#endif