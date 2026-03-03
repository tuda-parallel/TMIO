#include <mutex>
#include <vector>
#include <utility>
#include <memory>
#include <mpi.h>
#include <map>
#include <ioflags.h>
#include <cstdarg>
#include <iodata.h>
#include <optional>
#include <condition_variable>

class Prefetcher
{
public:
    //Everything a call needs
    struct CallSignature 
    {
        MPI_File fh;
        int count;
        MPI_Datatype type;
        MPI_Offset offset;

        bool operator==(const CallSignature& other) {
            return fh == other.fh && count == other.count && type == other.type && offset == other.offset;
        };
    };
private:
    struct PrefetchInfo
    {
        double last_call_time;
        CallSignature cs;
        bool prefetched;
        std::optional<double> next_prefetch_time;
    };

    struct CallDB
    {
        std::mutex event_mutex;
        std::condition_variable prefetch_event;
        bool event_ready;

        std::mutex call_lock;
        std::map<MPI_File, PrefetchInfo> prefetch_infos;
        double io_frequency;
        int prefetch_bandwidth;
    };

    struct Request {
        MPI_Request request;
        std::vector<std::byte> buffer;
        CallSignature cs;
    };

    struct RequestsInTransit {
        std::mutex request_lock;
        std::vector<Request> requests;
    };

    struct AsyncBuffers {
        std::vector<std::byte> prefetch_buffer;
        void* target_buffer;
    };

    bool inititalized;
    const int max_cache_size_bytes = 100'000;
    const int max_file_size_bytes = 10'000;
    const double prefetch_bandwidth = 10.;

    IOtraceMPI* traces;

    std::shared_ptr<CallDB> callDBs;
    std::shared_ptr<RequestsInTransit> requests_in_transit;
    std::map<MPI_Request*, AsyncBuffers> async_requests;
    std::map<MPI_Request*, CallSignature> request_signature;

public:
    Prefetcher();
    void init(IOtraceMPI*, int*);
    int retrieve_read_snyc(CallSignature&, void*, MPI_Status*);
    int retrieve_read_asnyc(CallSignature&, MPI_Request*, void*);
    void register_transaction(CallSignature &cs, double);
    void fetch_read_async_wait(MPI_Request*, int);
    void fetch_read_async_test(MPI_Request*, int*, int);
    void set_io_frequency(double);
    void set_prefetch_bandwidth(int);
private:
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
    bool take_prefetched_by_call_signature(CallSignature&, MPI_Request*, std::vector<std::byte>&);
    static void prefetching_routine(std::shared_ptr<RequestsInTransit>, std::shared_ptr<CallDB>, IOtraceMPI*);
    static void prefetch_transaction(CallSignature&, std::shared_ptr<RequestsInTransit>&, IOtraceMPI*);
    static std::optional<MPI_File> determine_next_prefetch(std::shared_ptr<CallDB>&, double&);
};