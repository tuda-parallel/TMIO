#ifndef BW_LIMIT_H
#define BW_LIMIT_H

#include "iodata.h"
#include <map>
#include <filesystem>
#include <cstdarg>

using Transaction_Type = IOdata::Transaction_Type;

#if BW_FILE_SPECIFIC == 1

template<typename FDType, typename RequestIDType>
class FileTracker {
private:
	std::map<FDType, std::filesystem::path> file_register;
	std::map<RequestIDType, std::filesystem::path> request_register;

public:
	void track_file_opened(const char*, const FDType);
	void track_file_closed(const FDType);
	std::filesystem::path get_fd_path(const FDType);
	void register_request(const RequestIDType, const FDType);
	std::filesystem::path get_request_path(const RequestIDType);
	void unregister_request(const RequestIDType);
};

#endif // BW_FILE_SPECIFIC

class Bw_limit
{

private:
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

#if defined BW_LIMIT
	double scale_bw_write;	// scales the bandwidth limit of sync write operations
	double scale_bw_read;	// scales the bandwidth limit of sync read operations
	double scale_bw_iwrite; // scales the bandwidth limit of async write operations
	double scale_bw_iread;	// scales the bandwidth limit of async read operations

	double bw_limit_iwrite;
	double bw_limit_iread;
	double Bw;
	double Br;

	double Get_BW_Limit(const double, const double) const;

#endif // BW_LIMIT

	double Get(Transaction_Type, std::string info) const;
	void Set(Transaction_Type, std::string info, double value);

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

public:
	Bw_limit();
	~Bw_limit();
	std::string Info(void) const;
	void Reset(void);
	void Init(int, int, IOdata *, IOdata *, IOdata *, IOdata *);

#if defined BW_LIMIT
	void Limit_Async(std::filesystem::path, bool);
#endif
#if defined CUSTOM_MPI || BW_LIMIT
	void Set_Throughput(void);
#endif

};
#endif // BW_LIMIT_H