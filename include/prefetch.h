#ifndef PREFETCH_H
#define PREFETCH_H

#include <mutex>
#include <vector>
#include <utility>
#include <memory>
#include <mpi.h>
#include <map>
#include <array>
#include <ioflags.h>
#include <cstdarg>
#include <iodata.h>
#include <optional>
#include <condition_variable>
#include <iotrace.h>
#include <unordered_map>


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
        std::unique_ptr<MPI_Request> mpi_request;
        std::vector<char> buffer;
        CallSignature cs;
        double last_access;

        Request(size_t buffer_size, const CallSignature& cs, double last_access)
         : cs(cs), last_access(last_access) {
            buffer.resize(buffer_size);
            mpi_request = std::make_unique<MPI_Request>();
         };

        int fill_buffer_with_request(void* target_buffer, MPI_Offset total_offset, int count, MPI_Status *status, size_t& missed_bytes);
    };
private:
    std::mutex request_lock;
    std::map<MPI_File, std::vector<Request>> requests;
    size_t total_cached_bytes;
    const size_t max_cache_size_bytes;
    const size_t max_file_size_bytes;

    bool evict_request(size_t);
public:

    RequestCache(size_t max_cache_size, size_t max_file_size) : 
        max_cache_size_bytes(max_cache_size), max_file_size_bytes(max_file_size), total_cached_bytes(0) {};
    size_t max_cache_bytes() const {return max_cache_size_bytes;};
    void remove_file(MPI_File&);
    std::optional<Request> take_prefetched_by_call_signature(CallSignature&);
    void insert_request(MPI_File, Request);
    bool reserve_cache_space(size_t);
};



class Prefetcher
{
    //Everything a call needs
private:
    struct CallInfo {
            double last_call_time;
            std::array<MPI_Offset, 2> total_offset;
            std::array<int, 2> count;
            MPI_Datatype datatype;
            std::optional<double> next_prefetch_time;
            bool prefetched;

            CallInfo(double last_call_time, 
            MPI_Offset total_offset, 
            int count, 
            MPI_Datatype datatype) : 
                last_call_time(last_call_time),
                total_offset{total_offset, total_offset},
                count{count, count},
                datatype(datatype),
                next_prefetch_time(std::nullopt),
                prefetched(false) {};

            void add_offset(MPI_Offset o) {
                total_offset[1] = total_offset[0];
                total_offset[0] = o;
            };
            void add_count(int c) {
                count[1] = count[0];
                count[0] = c;
            };
    };

    struct PrefetchInfo
    {
        std::vector<CallInfo> callInfos;
        size_t current_call_idx;
        size_t prefetch_idx;
        bool valid_prefetch;
        bool phase_added;

        PrefetchInfo() : current_call_idx(0), prefetch_idx(0), valid_prefetch(false), phase_added(true) {};
        void add_callInfo(double last_call_time, 
            MPI_Offset total_offset, 
            int count, 
            MPI_Datatype datatype) {
            if(current_call_idx >= callInfos.size()) {
                callInfos.emplace_back(CallInfo(last_call_time, total_offset, count, datatype));  
                valid_prefetch = false;
                phase_added = true;
            } else {
                auto& ci = callInfos[current_call_idx];
                ci.add_offset(total_offset);
                ci.last_call_time = last_call_time;
                ci.add_count(count);
                ci.prefetched = false;
            }
            ++current_call_idx;
        }
        
        void phase_end() {
            if(!phase_added && current_call_idx == callInfos.size())
                valid_prefetch = true;
            current_call_idx = 0;
            phase_added = false;
        }

        CallInfo& get_next_prefetch() {
            return callInfos.at(prefetch_idx);
        }

        void advance_prefetch() {
            callInfos.at(prefetch_idx).prefetched = true;
            callInfos.at(prefetch_idx).next_prefetch_time = std::nullopt;
            prefetch_idx = (prefetch_idx + 1) % callInfos.size();
        }
    };

    struct CallDB
    {
        std::mutex event_mutex; // deadlock prevention: lock call_lock first!
        std::condition_variable prefetch_event;
        bool event_ready = false;

        std::mutex call_lock;
        std::unordered_map<MPI_File, PrefetchInfo> prefetch_infos;
        double io_interval;
        std::optional<double> prefetch_bandwidth;
        double prefetch_ratio;

        CallDB(double frequency) : io_interval(1/frequency), prefetch_ratio(0.8) {};
    };

    struct AsyncBuffers {
        RequestCache::Request prefetch_request;
        void* target_buffer;
        MPI_Offset total_offset;
        int count;

        AsyncBuffers(RequestCache::Request&& pr, void* tb, int c, MPI_Offset to) : prefetch_request(std::move(pr)), target_buffer(tb), total_offset(to), count(c) {};
    };

    bool inititalized = false;
    std::thread prefetching_thread;

    std::atomic<bool> stop_token;

    std::shared_ptr<CallDB> callDBs;
    std::shared_ptr<RequestCache> requests_in_transit;

    std::mutex ar_lock;
    std::map<MPI_Request*, AsyncBuffers> async_requests;

    std::mutex rs_lock;
    std::map<MPI_Request*, CallSignature> request_signature;

    double phase_start;
#if OVERHEAD == 1
    std::atomic<size_t> prefetch_missed_bytes;
    std::atomic<size_t> total_missed_bytes;
    double init_time;
    inline static thread_local double local_overhead;
    std::atomic<double> total_overhead;
#endif

    void start_overhead() {
#if OVERHEAD == 1
        local_overhead = MPI_Wtime();
#endif
    };
    void end_overhead() {
#if OVERHEAD == 1
        double overhead = total_overhead.load();
        while (!total_overhead.compare_exchange_weak(overhead, overhead + MPI_Wtime() - local_overhead));
#endif
    }

    int fetch_read_async_impl(MPI_Request* original_request, MPI_Status* status);
    void register_transaction(CallSignature &cs, double);

public:
    Prefetcher();
    ~Prefetcher() {
        if(inititalized) finalize();
    }
    void init(double, size_t = 1'000'000'000, size_t = 100'000'000);
    void finalize();
    void close_file(MPI_File file);
    int retrieve_read_sync(CallSignature&, void*, MPI_Status*);
    int retrieve_read_async(CallSignature&, MPI_Request*, void*);
    int fetch_read_async_wait(MPI_Request*, MPI_Status*);
    int fetch_read_async_wait_all(int, MPI_Request*, MPI_Status*);
    int fetch_read_async_test(MPI_Request*, MPI_Status*, int*);
    int fetch_read_async_test_all(int, MPI_Request*, MPI_Status*, int*);
    void set_io_frequency(double);
    void set_prefetch_bandwidth(double);
    
    template <VerbosityLevel Level>
    inline static void Log(const char *format, ...) {
		if constexpr (static_cast<int>(PREFETCH_VERBOSITY) >= static_cast<int>(Level))
		{
			va_list args;
			va_start(args, format);
			vprintf(format, args);
			va_end(args);
		}
	};
private:
    void check_phase();
    MPI_Request* determine_valid_request(MPI_Request* request);
    static bool is_contiguous_type(MPI_Datatype);
    static void prefetching_routine(std::shared_ptr<RequestCache>, std::shared_ptr<CallDB>, std::atomic<bool>&);
    static void prefetch_transaction(CallSignature&, std::shared_ptr<RequestCache>&);
    static std::optional<MPI_File> determine_next_prefetch(std::shared_ptr<CallDB>&, double&);
    static CallSignature determine_prefetch_signature(CallInfo&, MPI_File);
    static std::optional<double> calculate_next_prefetch(CallInfo&, double, double, std::optional<double>);
};

#endif