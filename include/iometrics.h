#include "iocollect.h"

/**
 *  IO trace class
 * @file   iotrace.h
 * @author Ahmad Tarraf
 * @date   30.06.2022
 */

struct core_rank_metrics
{
    double amean = 0;
    double hmean = 0;
    double whmean = 0;
    double median = 0;
    double max = 0;
    double min = 0;
};

struct rank_metrics
{
    core_rank_metrics sum; // sum over opearations
	core_rank_metrics avr; // average over operations 
};

struct core_app_metrics
{
    double max = 0; // app: maximum of the overlaping bandwidth
    double hmean = 0; //appH: harmonic mean of the overlapping bandwidth
    double agg_max = 0; // aggreagated maximum over all ranks
};

struct app_metrics
{
    core_app_metrics sum;
	core_app_metrics avr;
};

/**
 * @brief all metrics releated to throughput|bandwidth are stored in this structure.
 * app_metric: contains the metrics at application level (max, heamn and agg_max). @see app_metrics
 * rank_metric: contains the metrics at rank level (amean, hmean, wmean, median, max and min). @see app_metrics
 */
struct iometrics{
    app_metrics app_metric;
    rank_metrics rank_metric;
    int n_max = 0;     // max overlap accros different phases
};
