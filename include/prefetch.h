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
#include <iotrace.h>


struct CallSignature 
{
    enum class CallType{
        Read,
        ReadAt,
    };

    MPI_File fh;
    int count;
    MPI_Datatype type;
    MPI_Offset offset;
    CallType ct;

    bool operator==(const CallSignature& other) {
        return fh == other.fh && count == other.count && type == other.type && offset == other.offset;
    };

    CallSignature(MPI_File fh,
        int count,
        MPI_Datatype type,
        MPI_Offset offset,
        CallType ct) :
            fh(fh), count(count), type(type), offset(offset), ct(ct) {};
};

class RequestCache {
public:
    struct Request {
        MPI_Request request;
        std::vector<std::byte> buffer;
        CallSignature cs;
        double last_access;

        Request(MPI_Request&& request, std::vector<std::byte>&& buffer, CallSignature& cs, double last_access)
         : request(request), buffer(buffer), cs(cs), last_access(last_access) {};

        int fill_buffer_with_request(void* target_buffer, MPI_Offset total_offset, int count);
    };
private:
    std::mutex request_lock;
    std::map<MPI_File, std::vector<Request>> requests;
    int total_cached_bytes;

    bool evict_request(int);
public:
    const int max_cache_size_bytes = 100'000;
    const int max_file_size_bytes = 10'000;

    void remove_file(MPI_File&);
    std::optional<Request> take_prefetched_by_call_signature(CallSignature&, MPI_Offset);
    void insert_request(MPI_File, Request);
};

class Prefetcher
{
    //Everything a call needs
private:
    struct PrefetchInfo
    {
        double last_call_time;
        std::vector<MPI_Offset> total_offset;
        std::vector<int> count;
        MPI_Datatype datatype;
        std::optional<double> next_prefetch_time;

        PrefetchInfo(double last_call_time, 
            MPI_Offset total_offset, 
            int count, 
            MPI_Datatype datatype) : 
                last_call_time(last_call_time),
                total_offset{total_offset},
                count{count},
                datatype(datatype),
                next_prefetch_time({}) {};
    };

    struct CallDB
    {
        std::mutex event_mutex;
        std::condition_variable prefetch_event;
        bool event_ready;

        std::mutex call_lock;
        std::unordered_map<MPI_File, PrefetchInfo> prefetch_infos;
        double io_frequency;
        int prefetch_bandwidth;
    };

    struct AsyncBuffers {
        RequestCache::Request prefetch_request;
        void* target_buffer;
        MPI_Offset total_offset;
        int count;

        AsyncBuffers(RequestCache::Request&& pr, void* tb, int c, MPI_Offset to) : prefetch_request(pr), target_buffer(tb), total_offset(to), count(c) {};
    };

    bool inititalized;
    
    const double prefetch_bandwidth = 10.;
    std::thread prefetching_thread;

    IOtraceMPI* traces;
    std::shared_ptr<std::atomic<bool>> stop_token;

    std::shared_ptr<CallDB> callDBs;
    std::shared_ptr<RequestCache> requests_in_transit;
    std::map<MPI_Request*, AsyncBuffers> async_requests;
    std::map<MPI_Request*, CallSignature> request_signature;

public:
    Prefetcher();
    void init(IOtraceMPI*, int*);
    void finalize();

    void close_file(MPI_File file);
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
    static void prefetching_routine(std::shared_ptr<RequestCache>, std::shared_ptr<CallDB>, IOtraceMPI*, std::shared_ptr<std::atomic<bool>>);
    static void prefetch_transaction(CallSignature&, std::shared_ptr<RequestCache>&, IOtraceMPI*);
    static std::optional<MPI_File> determine_next_prefetch(std::shared_ptr<CallDB>&, double&);
    static CallSignature determine_prefetch_signature(PrefetchInfo&, MPI_File);
    static std::optional<double> calculate_next_prefetch(PrefetchInfo&, double, double);
};