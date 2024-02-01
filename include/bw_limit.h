#include "convergence.h"
#include "ioanalysis.h"

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

	dc_context_t context_read;	 // strucutre used by the bandwidth limitation approach in the custom mpich
	dc_context_t context_write;	 // strucutre used by the bandwidth limitation approach in the custom mpich
	dc_context_t context_iread;	 // strucutre used by the bandwidth limitation approach in the custom mpich
	dc_context_t context_iwrite; // strucutre used by the bandwidth limitation approach in the custom mpich

	int first_time_read;   // used to indicate the first time assignment of the bandwidth for sync write operations
	int first_time_write;  // used to indicate the first time assignment of the bandwidth for sync read operations
	int first_time_iread;  // used to indicate the first time assignment of the bandwidth for  async write operations
	int first_time_iwrite; // used to indicate the first time assignment of the bandwidth for  async read operations

	double Biw;
	double Bir;
	double Bw;
	double Br;

	// bool Bw_goal(double, double, dc_context_t *);
#endif
	double Get(std::string mode, std::string info);
	void Set(std::string mode, std::string info, double value);

public:
	Bw_limit();
	~Bw_limit();
	std::string Info(void);
	void Reset(void);
	void Init(int, int, IOdata *, IOdata *, IOdata *, IOdata *);

#if defined BW_LIMIT
	void Limit_Async(void);
#elif defined CUSTOM_MPI
	void Set_Throughput(void);
#endif

};
