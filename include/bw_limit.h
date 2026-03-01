#ifndef BW_LIMIT_H
#define BW_LIMIT_H

#include "iodata.h"
#include <map>
#include <filesystem>
#include <cstdarg>
#include <mutex>

using TransactionType = IOdata::TransactionType;

template<typename FDType, typename RequestIDType>
class [[maybe_unused]] FileTracker {

	//TODO: make this threadsafe
private:
	std::map<FDType, std::filesystem::path> file_register;
	std::map<RequestIDType, std::filesystem::path> request_register;

public:
	void track_file_opened(const char* path, const FDType fd)
	{
		// Get full unique path
		auto full_path = std::filesystem::absolute(
			std::filesystem::weakly_canonical(std::filesystem::path(path)));
		file_register.insert(std::make_pair(fd, full_path));
	};

	void track_file_closed(const FDType fd) 
	{
		file_register.erase(fd);
	};

	std::filesystem::path* get_fd_path(const FDType fd) 
	{
		auto it = file_register.find(fd);
		if (it != file_register.end()) {
			return &it->second;
		}
		return nullptr;
	};

	bool fd_valid(const FDType fd)
	{
		auto it = file_register.find(fd);
		return it != file_register.end();
	}

	void register_request(const RequestIDType request_id, const FDType fd) 
	{
		auto it = file_register.find(fd);
		if (it != file_register.end()) {
			auto path = it->second;
			request_register.insert(std::make_pair(request_id, path));
		}
	};

	std::filesystem::path* get_request_path(const RequestIDType request_id) 
	{
		auto it = request_register.find(request_id);
		if(it != request_register.end()) {
			return &it->second;
		}
		return nullptr;
	};

	void unregister_request(const RequestIDType request_id) 
	{
		auto it = request_register.find(request_id);
		if(it != request_register.end()) {
			request_register.erase(it);		
		}
	};
};

class Bw_limit
{

private:
	std::mutex bw_lock;
	char caller[12] = "\tBw_limit";
	int rank;
	int processes;

	double counter_read;   // used to indicate when the sync write operations increase
	double counter_write;  // used to indicate when the sync read operations increase
	double counter_iread;  // used to indicate when the  async write operations increase
	double counter_iwrite; // used to indicate when the  async read operations increase

	IOdata *p_aw;
	IOdata *p_ar;
	IOdata *p_sw;
	IOdata *p_sr;

#if BW_LIMIT_FTIO == 1
	double ftio_phase_pred;
#endif

#if defined BW_LIMIT
	double scale_bw_write;	// scales the bandwidth limit of sync write operations
	double scale_bw_read;	// scales the bandwidth limit of sync read operations
	double scale_bw_iwrite; // scales the bandwidth limit of async write operations
	double scale_bw_iread;	// scales the bandwidth limit of async read operations

	double bw_limit_iwrite;
	double bw_limit_iread;
	double Bw;
	double Br;

	double calculate_bw_limit(const double, const double) const;
	
#endif // BW_LIMIT

	double get_phase_info(TransactionType, std::string info) const;
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

#if defined CUSTOM_MPI
	void set_throughput();
#endif

#if BW_LIMIT_GRANULARITY == 1
	void limit_async();
#endif

#if BW_LIMIT_GRANULARITY > 1
	void limit_by_file(bool, [[maybe_unused]] const std::filesystem::path* path, [[maybe_unused]] long long transaction_size);
#endif

#if BW_LIMIT_FTIO == 1
	void receive_dominant_frequency(int, MPI_Comm);
#endif

};
#endif // BW_LIMIT_H