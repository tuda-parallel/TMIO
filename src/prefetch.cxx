#include "prefetch.h"
#include <thread>
#include <stdio.h>
#include <algorithm>
#include <cstring>
#include <chrono>

extern IOtraceMPI mpi_iotrace;

using CallType = CallSignature::CallType;
using Request = RequestCache::Request;

int Request::fill_buffer_with_request(void* target_buffer, MPI_Offset requested_offset, int requested_count, MPI_Status* status, size_t& missed_bytes) {
    Prefetcher::Log<VerbosityLevel::BASIC_LOG>(
            "Prefetcher > Retrieving prefetch %ld b with offset %ld\n", requested_count, requested_offset);
    
    int err = MPI_SUCCESS;
    int type_size;
    int total_read_elements = 0;
    MPI_Type_size(cs.type, &type_size);

    MPI_Status cache_miss_status;
    MPI_Offset copy_start = 0;

    char* target_buffer_ptr = static_cast<char*>(target_buffer);
    if (requested_offset < cs.offset) {
        int pre_count_bytes = static_cast<int>(cs.offset - requested_offset);
        
        missed_bytes += pre_count_bytes;
        int pre_count = pre_count_bytes / type_size;

        err = PMPI_File_read_at(cs.fh, requested_offset, target_buffer, pre_count, cs.type, &cache_miss_status);

        PMPI_Get_count(&cache_miss_status, cs.type, &total_read_elements);

        target_buffer_ptr = target_buffer_ptr + (pre_count * type_size);
    } else {
        copy_start = requested_offset - cs.offset;
    }

    if (err != MPI_SUCCESS) return err;

    MPI_Offset fetched_end = cs.offset + (cs.count * type_size);
    MPI_Offset requested_end = requested_offset + (requested_count * type_size);

    MPI_Offset begin = std::max(cs.offset, requested_offset);
    MPI_Offset end = std::min(fetched_end, requested_end);

    MPI_Offset copy_count = end - begin;

    std::memcpy(target_buffer_ptr, buffer.data() + copy_start, copy_count);
    total_read_elements += copy_count;

    target_buffer_ptr = target_buffer_ptr + (copy_count * type_size);


    if (fetched_end < requested_end) {
        int post_count_bytes = static_cast<int>(requested_end - fetched_end);
        missed_bytes += post_count_bytes;
        int post_count = post_count_bytes / type_size;

        err = PMPI_File_read_at(cs.fh, fetched_end, target_buffer_ptr, post_count, cs.type, &cache_miss_status);
    
        int post_read;
        PMPI_Get_count(&cache_miss_status, cs.type, &post_read);
        total_read_elements += post_read;
    }

    Prefetcher::Log<VerbosityLevel::BASIC_LOG>(
            "Prefetcher > Missed %ld b of %ld b\n", missed_bytes, requested_count * type_size);
    
    if(status != MPI_STATUS_IGNORE && status != MPI_STATUSES_IGNORE) {
        PMPI_Status_set_elements(status, cs.type, total_read_elements);
        status->MPI_ERROR = err;
    }

    return err;
}

/**
 * @brief Find prefetch in cache by call signature
 * @param cs Signature of the retrieved call
 * @param mpi_request [out] MPI_Request associated with prefetch call
 * @param prefetch_buffer [out] Returns buffer associated with prefetch
 * @return true if prefetch with same call_signature was found
 */
std::optional<Request> RequestCache::take_prefetched_by_call_signature(CallSignature& requested_cs) {

    //TODO better cs finding
    const std::lock_guard<std::mutex> lock(request_lock);

    auto it = requests.find(requested_cs.fh);
    bool found = it != requests.end();

    if(found) {
        MPI_Offset max_overlap = 0;

        int type_size;
        MPI_Type_size(requested_cs.type, &type_size);

        MPI_Offset requested_begin = requested_cs.offset;
        MPI_Offset requested_end = requested_cs.offset + (type_size * requested_cs.count);

        std::optional<decltype(it->second.begin())> max_iterator = std::nullopt;
        
        for(auto fetch_request = it->second.begin(); fetch_request != it->second.end(); fetch_request++) {
            if (fetch_request->cs.type != requested_cs.type) continue;

            MPI_Offset fetched_begin = std::max(fetch_request->cs.offset, requested_begin);
            MPI_Offset fetched_end = std::min(fetch_request->cs.offset + (type_size * fetch_request->cs.count), requested_end);

            if (fetched_end <= fetched_begin) continue;

            MPI_Offset overlap = fetched_end - fetched_begin;

            if (overlap > max_overlap) {
                max_overlap = overlap;
                max_iterator = fetch_request;
            }
        }
        if(max_overlap == 0) return std::nullopt; 
        Request request = std::move(*(max_iterator.value()));
        total_cached_bytes -= request.buffer.size();
        it->second.erase(max_iterator.value());
        if (it->second.empty()) {
            requests.erase(it);
        }

        return request;
    }

    return std::nullopt;
}

/**
 * @brief Removes requests associated with file
 */
void RequestCache::remove_file(MPI_File& fh) {
    const std::lock_guard<std::mutex> lock(request_lock);

    auto it = requests.find(fh);
    if(it != requests.end()) {
        for(auto& req : it->second) {
            MPI_Cancel(req.mpi_request.get());
            MPI_Wait(req.mpi_request.get(), MPI_STATUS_IGNORE);
            total_cached_bytes -= req.buffer.size();
        }
        requests.erase(it);
    }
}

/**
 * @brief Frees space from the cache
 * @param bytes_to_evict number of bytes to remove free in cache
 * @note Call only with request_lock engaged
 * @return true if enough space has been freed
 */
bool RequestCache::evict_request(size_t bytes_to_evict) {

    while(bytes_to_evict > 0) {
        
        std::optional<decltype(requests.begin())> min_req;
        std::optional<double> oldest_access;

        for(auto it_req = requests.begin(); it_req != requests.end(); it_req++) {
                
            if(!oldest_access || oldest_access.value() > it_req->second.begin()->last_access) {
                min_req = it_req; 
                oldest_access = it_req->second.begin()->last_access;
            }
        }

        if (!min_req) return false; // No smallest found

        Request& to_delete = *min_req.value()->second.begin();
        PMPI_Cancel(to_delete.mpi_request.get());
        PMPI_Wait(to_delete.mpi_request.get(), MPI_STATUS_IGNORE);

        size_t evicted_bytes = to_delete.buffer.size();
        min_req.value()->second.erase(min_req.value()->second.begin());

        if (min_req.value()->second.empty()) requests.erase(min_req.value());

        Prefetcher::Log<VerbosityLevel::BASIC_LOG>(
            "Prefetcher > Cache full >> evicting request with %ld bytes\n", evicted_bytes);

        bytes_to_evict = evicted_bytes > bytes_to_evict? 0 : bytes_to_evict - evicted_bytes;
        total_cached_bytes -= evicted_bytes;
    }

    return true;
}

void RequestCache::insert_request(MPI_File fh, Request request) {
    const std::lock_guard<std::mutex> lock(request_lock);
    size_t request_size = request.buffer.size();

    if(total_cached_bytes + request_size > max_cache_size_bytes) {
    
        if(!evict_request(total_cached_bytes + request_size - max_cache_size_bytes)) {
            Prefetcher::Log<VerbosityLevel::BASIC_LOG>(
            "Prefetcher > Cache full >> no prefetch possible\n");
            PMPI_Cancel(request.mpi_request.get());
            PMPI_Wait(request.mpi_request.get(), MPI_STATUS_IGNORE);
            return;
        }
    }

    total_cached_bytes += request_size;

    requests[fh].emplace_back(std::move(request));
}

bool RequestCache::reserve_cache_space(size_t required_space) {
    const std::lock_guard<std::mutex> lock(request_lock);

    if(total_cached_bytes + required_space > max_cache_size_bytes) {
    
        if(!evict_request(total_cached_bytes + required_space - max_cache_size_bytes)) {
            Prefetcher::Log<VerbosityLevel::BASIC_LOG>(
            "Prefetcher > Cache full >> Could not free space for prefetch\n");
            return false;
        }
        
    }
    return true;
}

Prefetcher::Prefetcher() : inititalized(false) {}

/**
 * @brief Initializes prefetching and spawns asnyc prefetch thread
 * @param max_cache_size maximum number of bytes cached by prefetcher
 * @param max_file_size maximum number of bytes a cached file can have
 * @param io_frequency frequency of the programs io access pattern
 */
void Prefetcher::init(double io_frequency, size_t max_cache_size, size_t max_file_size)
{
    if(inititalized) return;
    Prefetcher::Log<VerbosityLevel::BASIC_LOG>(
            "Initializing prefetcher with frequency %f, max cache size %.2f Mb, max file size %.2f Mb.\n",
             io_frequency, static_cast<double>(max_cache_size) / 1'000'000., static_cast<double>(max_file_size) / 1'000'000.);
    //test if MPI multithreading enabled
    int mpi_provided;
    MPI_Query_thread(&mpi_provided);
    if(mpi_provided != MPI_THREAD_MULTIPLE) {
        Prefetcher::Log<VerbosityLevel::BASIC_LOG>(
            "MPI multithreading not activated. I/O prefetching deactivated.");
        return;
    }
    
    callDBs = std::make_shared<CallDB>(io_frequency);
    requests_in_transit = std::make_shared<RequestCache>(max_cache_size, max_file_size);

    stop_token = false;

    prefetching_thread = std::thread([&] {Prefetcher::prefetching_routine(requests_in_transit, callDBs, stop_token);});
    phase_start = std::numeric_limits<double>::min();
    inititalized = true;
#if OVERHEAD == 1
    prefetch_missed_bytes = 0;
    total_missed_bytes = 0;
    init_time = MPI_Wtime();
    local_overhead = 0.0;
#endif
}

void Prefetcher::finalize() {
    if(!inititalized) return;

    start_overhead();
    stop_token.store(true);

    {
        std::lock_guard<std::mutex> lock(callDBs->event_mutex);
        callDBs->event_ready = true;
    }

    callDBs->prefetch_event.notify_all();
    
    if(prefetching_thread.joinable())
        prefetching_thread.join();


    end_overhead();
#if OVERHEAD == 1
    double run_time = MPI_Wtime() - init_time;
    Prefetcher::Log<VerbosityLevel::DETAILED_LOG>(
            "Prefetching overhead: %f.2s, %f.2% of runtime\n", total_overhead.load(), run_time);
    Prefetcher::Log<VerbosityLevel::DETAILED_LOG>(
            "Prefetching missed bytes: %ld\n", prefetch_missed_bytes.load());
    Prefetcher::Log<VerbosityLevel::DETAILED_LOG>(
            "Total missed bytes: %ld\n", total_missed_bytes.load() + prefetch_missed_bytes.load());
#endif

    inititalized = false;
}

/**
 * @brief Get MPI_Request corresponding to async call from prefetch cache. Replaces the actual file call.
 * @param cs Signature of the retrieved call
 * @param mpi_request [out] Returns MPI_Request corresponding to prefetched call
 * @param target_buffer Target buffer for read data
 */
int Prefetcher::retrieve_read_async(CallSignature& cs, MPI_Request* mpi_request, void* target_buffer) 
{
    if(!inititalized) {
        int err;
        mpi_iotrace.Read_Async_Start(cs.count, cs.type, mpi_request, cs.fh, cs.offset);

#if BW_LIMIT_GRANULARITY > 1
	    mpi_iotrace.apply_file_specific_bw(false, cs.fh, cs.count, cs.type);
#endif
        if (cs.ct == CallType::Read) {
            err = PMPI_File_iread(cs.fh, target_buffer, cs.count, cs.type, mpi_request); 
        } else{
            err = PMPI_File_iread_at(cs.fh, cs.offset, target_buffer, cs.count, cs.type, mpi_request);
        }
        return err;
    }
    
    start_overhead();
    check_phase();

    int err;

    if (cs.ct == CallType::Read) {
        MPI_Offset total_offset;
        PMPI_File_get_position(cs.fh, &total_offset);
        cs.offset = total_offset;
    }

    if (std::optional<Request> request = requests_in_transit->take_prefetched_by_call_signature(cs); request.has_value()) {
        *mpi_request = MPI_REQUEST_NULL;

        std::lock_guard lock(ar_lock);
        async_requests.emplace(mpi_request, std::move(AsyncBuffers(std::move(request.value()), target_buffer, cs.count, cs.offset)));

        err = MPI_SUCCESS;
    } else {
        Prefetcher::Log<VerbosityLevel::DETAILED_LOG>(
            "Prefetch miss on async read. Fetching read manually\n");
        end_overhead();
        mpi_iotrace.Read_Async_Start(cs.count, cs.type, mpi_request, cs.fh, cs.offset);
        err = PMPI_File_iread_at(cs.fh, cs.offset, target_buffer, cs.count, cs.type, mpi_request);
        start_overhead();
        #if OVERHEAD == 1
            int size;
            int err = PMPI_Type_size(cs.type, &size);
            if (err == MPI_SUCCESS) total_missed_bytes.fetch_add(static_cast<size_t>(size) * static_cast<size_t>(cs.count));
        #endif
    }

    if (cs.ct == CallType::Read) {
        int size;
        MPI_Type_size(cs.type, &size);
        PMPI_File_seek(cs.fh, cs.offset + (cs.count * size), MPI_SEEK_SET);
    }

    {
        std::lock_guard lock(rs_lock);
        request_signature.insert_or_assign(mpi_request, cs);
    }
    end_overhead();
    return err;
}

MPI_Request* Prefetcher::determine_valid_request(MPI_Request* request) {
    if(!inititalized) return request;

    auto it = async_requests.find(request);
    if (it != async_requests.end()) {
        return it->second.prefetch_request.mpi_request.get();
    } else {
        return request;
    }
}

/**
 * @brief Copy results of transaction into buffer
 * @param request Signature of the retrieved call
 * @param status Status of the call retrieval
 * @param int Status of the MPI call
 * @return MPI error value
 */
int Prefetcher::fetch_read_async_wait(MPI_Request* request, MPI_Status* status) {  
    
    MPI_Request* valid_request = determine_valid_request(request);

    int err = PMPI_Wait(valid_request, status);

    if(!inititalized || err != MPI_SUCCESS) return err;
    
    err = fetch_read_async_impl(request, status);
    
    return err;
}

/**
 * @brief Copy results of transaction into buffer
 * @param request Signature of the retrieved call
 * @param status Status of the call retrieval
 * @param int Status of the MPI call
 * @return MPI error value
 */
int Prefetcher::fetch_read_async_wait_all(int count, MPI_Request* requests, MPI_Status* statuses) {
         
    int err = MPI_SUCCESS;   

    for (int i = 0; i < count; i++)
	{
        MPI_Request* valid_request = determine_valid_request(&requests[i]);

        MPI_Status* status = statuses == MPI_STATUSES_IGNORE? 
            MPI_STATUS_IGNORE : &statuses[i];

		mpi_iotrace.Write_Async_Required(valid_request);
		mpi_iotrace.Read_Async_Required(valid_request);

        err = PMPI_Wait(valid_request, status);

        mpi_iotrace.Write_Async_End(valid_request);
		mpi_iotrace.Read_Async_End(valid_request);

        if(err != MPI_SUCCESS) return err;
    }

    if(!inititalized) return err;

    for(int i = 0; i < count; i++) {
        MPI_Status* status = statuses == MPI_STATUSES_IGNORE? 
            MPI_STATUS_IGNORE : &statuses[i];
        err = fetch_read_async_impl(&requests[i], status);
        if(err != MPI_SUCCESS) return err;
    }
    
    return err;
}

/**
 * @brief Copy results of successful test into buffer
 * @param request Signature of the retrieved call
 * @param status Status  of the call retieval
 * @param flag true if test successfull
 * @param int Status of the MPI call
 * @return MPI error value
 */
int Prefetcher::fetch_read_async_test(MPI_Request* request, MPI_Status* status, int* flag) {  
    
    MPI_Request* valid_request = determine_valid_request(request);

    int err = PMPI_Test(valid_request, flag, status);

#if TEST == 1
	mpi_iotrace.Write_Async_End(valid_request, *flag);
	mpi_iotrace.Read_Async_End(valid_request, *flag);
#endif

    if(!inititalized || err != MPI_SUCCESS || !*flag) return err;
    
    err = fetch_read_async_impl(request, status);   
    return err;
}

/**
 * @brief Copy results of successful test into buffer
 * @param requests Signatures of the retrieved call
 * @param statuss Statuses of the call retieval
 * @param flag true if test successfull
 * @param int Status of the MPI call
 * @return MPI error value
 */
int Prefetcher::fetch_read_async_test_all(int count, MPI_Request* requests, MPI_Status* statuses, int* flag) {  
    int err = MPI_SUCCESS;  
    
    for(int i = 0; i < count; i++) {
        MPI_Request* valid_request = determine_valid_request(&requests[i]);
        MPI_Status* status = statuses == MPI_STATUSES_IGNORE? 
            MPI_STATUS_IGNORE : &statuses[i];
        err = PMPI_Test(valid_request, flag, status);

#if TEST == 1
        mpi_iotrace.Write_Async_End(valid_request, *flag);
		mpi_iotrace.Read_Async_End(valid_request, *flag);
#endif
        if(err != MPI_SUCCESS || !*flag) return err;
	}

    if(!inititalized) return err;

    for(int i = 0; i < count; i++) {
        MPI_Status* status = statuses == MPI_STATUSES_IGNORE? 
            MPI_STATUS_IGNORE : &statuses[i];
        err = fetch_read_async_impl(&requests[i], status);

        if(err != MPI_SUCCESS) return err;
    }
    
    return err;
}

int Prefetcher::fetch_read_async_impl(MPI_Request* request, MPI_Status* status) {
    int err = MPI_SUCCESS;
    start_overhead();
    {
        std::lock_guard lock(ar_lock);
        auto it = async_requests.find(request);

        if (it != async_requests.end()) {
            size_t missed_bytes = 0;
            err = it->second.prefetch_request.fill_buffer_with_request(it->second.target_buffer, it->second.total_offset, it->second.count, status, missed_bytes);
            
            #if OVERHEAD == 1
            prefetch_missed_bytes.fetch_add(missed_bytes);
            #endif

            async_requests.erase(it);
        }
    }
    {
        std::lock_guard lock(rs_lock);
        auto rs = request_signature.find(request);
        if (rs != request_signature.end())
        {
            double time_req = MPI_Wtime();
            register_transaction(rs->second, time_req);
            request_signature.erase(rs);
        }
    }

    end_overhead();
    return err;
}

/**
 * @brief Removes all prefetches connected to file from the prefetching cache
 */
void Prefetcher::close_file(MPI_File file) {
    if(!inititalized) return;

    start_overhead();
    std::lock_guard lock(callDBs->call_lock);

    auto it = callDBs->prefetch_infos.find(file);
    if(it != callDBs->prefetch_infos.end()) {
        callDBs->prefetch_infos.erase(it);
    }
    
    requests_in_transit->remove_file(file);
    end_overhead();
}

/**
 * @brief Get results from prefetch cache, block if necessary. Replaces the actual file call.
 * @param cs Signature of the retrieved call
 * @param target_buffer Target buffer for read data
 * @param mpi_status [out] Returns status of read operation
 * @return MPI error value
 */
int Prefetcher::retrieve_read_sync(CallSignature& cs, void* target_buffer, MPI_Status* status)
{
    if(!inititalized) {
        int err;
        mpi_iotrace.Read_Sync_Start(cs.count, cs.type, cs.offset);
        if (cs.ct == CallType::Read) {
            err = PMPI_File_read(cs.fh, target_buffer, cs.count, cs.type, status); 
        } else{
            err = PMPI_File_read_at(cs.fh, cs.offset, target_buffer, cs.count, cs.type, status);
        }
        mpi_iotrace.Read_Sync_End();
        return err;
    }
    
    start_overhead();
    check_phase();
    double time_req = MPI_Wtime();

    
    if (cs.ct == CallType::Read) {
        MPI_Offset total_offset;
        PMPI_File_get_position(cs.fh, &total_offset);
        cs.offset = total_offset;
    }

    int err;
    if (std::optional<Request> request = std::move(requests_in_transit->take_prefetched_by_call_signature(cs)); request.has_value()) {
        mpi_iotrace.Read_Async_Required(request.value().mpi_request.get());
        err = PMPI_Wait(request.value().mpi_request.get(), status);
        mpi_iotrace.Read_Async_End(request.value().mpi_request.get());
        if(err == MPI_SUCCESS) {
            size_t missed_bytes = 0;
            request.value().fill_buffer_with_request(target_buffer, cs.offset, cs.count, status, missed_bytes);
#if BW_LIMIT_GRANULARITY == 1
	        mpi_iotrace.apply_bw_limit();
#elif defined CUSTOM_MPI
	        mpi_iotrace.set_custom_throughput();
#endif 
#if OVERHEAD == 1
            prefetch_missed_bytes.fetch_add(missed_bytes);
#endif
        }
    } else {
        Prefetcher::Log<VerbosityLevel::DETAILED_LOG>(
            "Prefetch miss on sync read. Fetching read manually\n");

        end_overhead();
        mpi_iotrace.Read_Sync_Start(cs.count, cs.type, cs.offset);
        err = PMPI_File_read_at(cs.fh, cs.offset, target_buffer, cs.count, cs.type, status); 
        mpi_iotrace.Read_Sync_End();
        start_overhead();
        #if OVERHEAD == 1
        int size;
        int err = PMPI_Type_size(cs.type, &size);
        if (err == MPI_SUCCESS) total_missed_bytes.fetch_add(static_cast<size_t>(size) * static_cast<size_t>(cs.count));
        #endif
    }
    
    if (cs.ct == CallType::Read) {
        int size;
        MPI_Type_size(cs.type, &size);
        PMPI_File_seek(cs.fh, cs.offset + (cs.count * size), MPI_SEEK_SET);
    }
    
    register_transaction(cs, time_req);
    end_overhead();
    return err;
}

/**
 * @brief Register a transaction for prefetching in next IO phase
 * @param cs Signature of the registered call
 * @param time_req Time stamp, when result of transaction is required
 */
void Prefetcher::register_transaction(CallSignature &cs, double time_req)
{
    int type_size;
    int err = PMPI_Type_size(cs.type, &type_size);

    if(!inititalized
        || err != MPI_SUCCESS 
        || static_cast<size_t>(type_size) * static_cast<size_t>(cs.count) > requests_in_transit->max_cache_bytes() 
        || !is_contiguous_type(cs.type)) {
        return;
    }

    {
        std::lock_guard<std::mutex> lock(callDBs->call_lock);

        auto it = callDBs->prefetch_infos.find(cs.fh);

        if(it == callDBs->prefetch_infos.end()) {
            PrefetchInfo prefetch_info;
            it = callDBs->prefetch_infos.emplace(cs.fh, prefetch_info).first;
        }

        it->second.add_callInfo(time_req, cs.offset, cs.count, cs.type);
    }

    {
        std::lock_guard<std::mutex> lock(callDBs->event_mutex);
        callDBs->event_ready = true;
    }

    callDBs->prefetch_event.notify_all();
}

/**
* @brief Checks if an io phase is done. Important for tracking file accesses
*/
void Prefetcher::check_phase() {
    double now = MPI_Wtime();

    std::lock_guard lock(callDBs->call_lock);
    if(now - phase_start > 0.5 * callDBs->io_interval) {
        for(auto& pi : callDBs->prefetch_infos) {
            pi.second.phase_end();
        }
        phase_start = now;
    }
}

/**
 * @brief Is MPI_Datatype contiguous
 */
bool Prefetcher::is_contiguous_type(MPI_Datatype type) {
    //https://github.com/jfmunoz00/MPICH-IOBandwidth-Limitation/blob/main/mpich-4.0.3_BW-limit/src/mpi/romio/adio/common/iscontig.c

    int nints, nadds, ntypes, combiner;

    PMPI_Type_get_envelope(type, &nints, &nadds, &ntypes, &combiner);

    switch (combiner) {
        case MPI_COMBINER_NAMED: {
            return true;
            break;
        }
        case MPI_COMBINER_CONTIGUOUS: {
            std::vector<int> ints(nints);
            std::vector<MPI_Aint> adds(nadds);
            std::vector<MPI_Datatype> types(ntypes);
            PMPI_Type_get_contents(type, nints, nadds, ntypes, ints.data(), adds.data(), types.data());
            bool flag = is_contiguous_type(types[0]);

            int ni, na, nt, cb;
        
            PMPI_Type_get_envelope(types[0], &ni, &na, &nt, &cb);
            if (cb != MPI_COMBINER_NAMED)
                PMPI_Type_free(types.data());

            return flag;
            break;
        }
        default: {
            return false;
            break;
        }
    }
}

/**
 * @brief Prefetch read transactions
 * @param rit Contains requests that are currently pending
 * @param cdb Information about calls
 */
void Prefetcher::prefetching_routine(std::shared_ptr<RequestCache> rit, std::shared_ptr<CallDB> cdb, std::atomic<bool> &stop_token) 
{
    Prefetcher::Log<VerbosityLevel::BASIC_LOG>(
            "Prefetcher start routine\n");
    std::optional<MPI_File> next_prefetch;
    while(!stop_token.load()) {
        
        double next_prefetch_time = 0.0;
        next_prefetch = determine_next_prefetch(cdb, next_prefetch_time);

        if(next_prefetch) {
            double now = MPI_Wtime();
            double sleep_s = next_prefetch_time - now;
            if(sleep_s > 0.) {
                std::this_thread::sleep_for(std::chrono::duration<double>(sleep_s));
            }
        
            std::optional<CallSignature> prefetch_signature;
            {
                std::lock_guard<std::mutex> lock(cdb->call_lock);
                auto it = cdb->prefetch_infos.find(*next_prefetch);
                if (it != cdb->prefetch_infos.end())
                prefetch_signature = determine_prefetch_signature(it->second.get_next_prefetch(), it->first);
                it->second.advance_prefetch();
            }
            if(prefetch_signature) {
                    prefetch_transaction(*prefetch_signature, rit);
            }
        } else { // Wait on new prefetch info to arrive
            std::unique_lock<std::mutex> lock(cdb->event_mutex);
            
            if(!cdb->event_ready && !stop_token) {
                cdb->prefetch_event.wait(lock, [&]{return cdb->event_ready || stop_token;});
            }
        }
    }
}

/**
 * @brief Starts prefetching of transaction
 * @param cs Signature of the prefetched call
 * @param rit MPI_Request associated with prefetch call
 */
void Prefetcher::prefetch_transaction(CallSignature& cs, std::shared_ptr<RequestCache>& rc)
{
    int type_size = 0;
    auto err = MPI_Type_size(cs.type, &type_size);

    if(err != MPI_SUCCESS || !rc->reserve_cache_space(type_size * cs.count)) return;
    
    Request request = Request(type_size * cs.count, cs, MPI_Wtime());

    // Start actual prefatch call
#if BW_LIMIT_GRANULARITY > 1
    mpi_iotrace.apply_file_specific_bw(false, cs.fh, cs.count, cs.type);
#endif
    mpi_iotrace.Read_Async_Start(cs.count, cs.type, request.mpi_request.get(), cs.fh, cs.offset);
    err = PMPI_File_iread_at(cs.fh, cs.offset, request.buffer.data(), cs.count, cs.type, request.mpi_request.get());

    if(err != MPI_SUCCESS || type_size <= 0) return;

    // Add prefetch to overall cache
    Prefetcher::Log<VerbosityLevel::BASIC_LOG>(
            "Prefetcher > Prefetch %ld b with offset %ld\n", cs.count * type_size, cs.offset);

    rc->insert_request(cs.fh, std::move(request));
}

/**
 * @brief Determines next prefetched MPI_File from callDB
 * @param cdb Information about previous calls
 * @param next_prefetch_time timestamp of the next prefetch call
 * @return Next file to prefetch on
 */
std::optional<MPI_File> Prefetcher::determine_next_prefetch(std::shared_ptr<CallDB>& cdb, double &next_prefetch_time) {
    Prefetcher::Log<VerbosityLevel::BASIC_LOG>(
            "Determining next prefetch \n");
    std::lock_guard<std::mutex> lock(cdb->call_lock);

    std::optional<decltype(cdb->prefetch_infos.begin())> min_element;

    for(auto it = cdb->prefetch_infos.begin(); it != cdb->prefetch_infos.end(); it++) {
    
        if(!it->second.valid_prefetch) {
            continue;
        }
        CallInfo& ci = it->second.get_next_prefetch();

        if (ci.prefetched) continue; // Already prefetched, at retrieval 

        if(!ci.next_prefetch_time) {
            ci.next_prefetch_time = calculate_next_prefetch(ci, cdb->io_interval, cdb->prefetch_ratio, cdb->prefetch_bandwidth);
        }

        if(ci.next_prefetch_time) {
            if(!min_element) {
                min_element = it;
            } else {
                min_element = 
                    *ci.next_prefetch_time < *(*min_element)->second.get_next_prefetch().next_prefetch_time?
                        it : min_element;
            }
        }
    }

    if(min_element) {
        next_prefetch_time = *(*min_element)->second.get_next_prefetch().next_prefetch_time;
        return (*min_element)->first;
    }
    
    // No min element found
    std::lock_guard<std::mutex> ev_lock(cdb->event_mutex);
    cdb->event_ready = false;

    return std::nullopt;
    
}

/**
 * @brief Determine call signature of next prefetch
 * @param prefetch_info Information about previous calls
 * @param fh File on which to prefetch
 * @return Call signature for prefetching
 */
CallSignature Prefetcher::determine_prefetch_signature(CallInfo& call_info, MPI_File fh) {
    MPI_Offset last = call_info.total_offset[0];
    MPI_Offset second_last = call_info.total_offset[1];

    MPI_Offset offset = last + (last - second_last);
    int count = call_info.count[0];

    return CallSignature(fh, count, call_info.datatype, offset, CallType::ReadAt);
}

/**
 * @brief Calculates the time point when the next prefetch should start
 * @param call_info Information about previous calls
 * @param io_period Length of one IO period
 * @param prefetch_ratio How far into an io period is prefetching started
 * @param bandwidth optional bandwidth for prefetching
 * @return 
 */
std::optional<double> Prefetcher::calculate_next_prefetch(CallInfo& call_info, double io_period, double prefetch_ratio, std::optional<double> bandwidth) {
    
    double next_call_time = io_period + call_info.last_call_time;
    std::optional<double> prefetch_time; 

    if (bandwidth.has_value()) {
        int type_size;

        int err = MPI_Type_size(call_info.datatype, &type_size);
        if(err != MPI_SUCCESS) return std::nullopt;

        double transfer_time = static_cast<double>(call_info.count[0] * type_size) / bandwidth.value();
        transfer_time = std::max(transfer_time, io_period);

        return next_call_time - transfer_time;
    }

    double pref = next_call_time - (io_period * prefetch_ratio);
    return pref;
}

/**
 * @brief Sets IO prefetching frequency. Initializes prefetch
 * @param frequency prefetching frequency
 */
void Prefetcher::set_io_frequency(double frequency) {
    if(!inititalized) {
        init(frequency);
    }

    if (frequency > 0.0) {
        std::lock_guard<std::mutex> lock(callDBs->call_lock);

        callDBs->io_interval = (1/frequency);
    }
}

/**
 * @brief Sets bandwidth for prefetching
 * @param bandwidth prefetching bandwidth
 */
void Prefetcher::set_prefetch_bandwidth(double bandwidth) {
    if(!inititalized) return;

    std::lock_guard<std::mutex> lock(callDBs->call_lock);
    callDBs->prefetch_bandwidth = bandwidth;
}

