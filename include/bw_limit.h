#ifndef BW_LIMIT_H
#define BW_LIMIT_H

#include "convergence.h"
#include "iodata.h"
#include "ioflags.h"
#include <cstdarg>
#include <mutex>
#include <optional>
#include <string>

class Bw_limit
{

private:
	using TransactionType = IOdata::TransactionType;

	std::mutex bw_lock;
	char caller[12] = "\tBw_limit";
	int rank;
	int processes;
	
	double total_file_limit_read;
	double total_file_limit_write;
	double last_phase_read_bw;
	double last_phase_write_bw;

	double counter_read;   // used to indicate when the sync write operations increase
	double counter_write;  // used to indicate when the sync read operations increase
	double counter_iread;  // used to indicate when the  async write operations increase
	double counter_iwrite; // used to indicate when the  async read operations increase

	IOdata *p_aw;
	IOdata *p_ar;
	IOdata *p_sw;
	IOdata *p_sr;

#if BW_LIMIT_FREQ == 1
	double ftio_phase_pred;
#endif

#ifdef BW_LIMIT
	double scale_bw_write;	// scales the bandwidth limit of sync write operations
	double scale_bw_read;	// scales the bandwidth limit of sync read operations
	double scale_bw_iwrite; // scales the bandwidth limit of async write operations
	double scale_bw_iread;	// scales the bandwidth limit of async read operations

	double bw_limit_iwrite;
	double bw_limit_iread;

	dc_context_t context_read;	 // structure used by the bandwidth limitation approach in the custom mpich
	dc_context_t context_write;	 // structure used by the bandwidth limitation approach in the custom mpich
	dc_context_t context_iread;	 // structure used by the bandwidth limitation approach in the custom mpich
	dc_context_t context_iwrite; // structure used by the bandwidth limitation approach in the custom mpich

	int first_time_read;   // used to indicate the first time assignment of the bandwidth for sync write operations
	int first_time_write;  // used to indicate the first time assignment of the bandwidth for sync read operations
	int first_time_iread;  // used to indicate the first time assignment of the bandwidth for  async write operations
	int first_time_iwrite; // used to indicate the first time assignment of the bandwidth for  async read operations

	double Biw;
	double Bir;
	double Bw;
	double Br;

	double calculate_bw_limit(const double, const double) const;
	
#endif // BW_LIMIT

	std::optional<double> get_last_phase_info(TransactionType, std::string info) const;
	void set_phase_info(TransactionType, std::string info, double value);

	template <VerbosityLevel Level>
	inline void Log(const char *format, ...) const
	{
		if constexpr (static_cast<int>(BW_LIMIT_VERBOSITY) >= static_cast<int>(Level))
		{
			va_list args;
			va_start(args, format);
			vprintf(format, args);
			va_end(args);
		}
	};

#if (defined BW_LIMIT) || (defined CUSTOM_MPI)
	double set_throughput_impl(TransactionType);
#endif

#if BW_LIMIT_GRANULARITY == 1
	void limit_async_impl(TransactionType);
#endif

public:
	Bw_limit();
	~Bw_limit();
	std::string Info(void) const;
	void Reset(void);
	void Init(int, int, IOdata *, IOdata *, IOdata *, IOdata *);

#ifdef CUSTOM_MPI
	void set_throughput();
#endif

#if BW_LIMIT_GRANULARITY == 1
	void limit_async();
#endif

#if BW_LIMIT_GRANULARITY > 1
	void limit_by_file(bool, [[maybe_unused]] const std::optional<PathID> path, [[maybe_unused]] long long transaction_size);
#endif
#ifdef BW_LIMIT
	void limit_checkpoint(long long transaction_size, double end_time);
	void limit_prefetch(long long transaction_size, double end_time);
#endif
#if BW_LIMIT_FREQ == 1
	void set_io_frequency(double);
#endif

};
#endif // BW_LIMIT_H