#if defined PREFETCH
#include "prefetch.h"
#include <cstring>

extern IOtraceMPI mpi_iotrace;

using CallType = CallSignature::CallType;
using Request = RequestCache::Request;

/**
 * @brief Copies prefetch data into request buffer. In case of no total overlap edges are retrieved from file
 * @param target_buffer [in] Buffer to copy request data into
 * @param requested_offset [in] File offset of the request
 * @param request_count [in] Required type count of the request
 * @param status [in] Request status
 * @param missed_bytes [out] Bytes that had to be fetched due to mismatch
 * @return MPI_Status of fetch edge retrieval
 */
int Request::fill_buffer_with_request(void* target_buffer, MPI_Offset requested_offset, int requested_count, MPI_Status* status, size_t& missed_bytes) {
    Prefetcher::Log<VerbosityLevel::BASIC_LOG>(
            "Prefetcher > Retrieving prefetch %ld b with offset %ld\n", requested_count, requested_offset);
    
    int err = MPI_SUCCESS;
    int type_size;
    int total_read_elements = 0;
    PMPI_Type_size(cs.type, &type_size);

    MPI_Status cache_miss_status;
    MPI_Offset copy_start = 0;

    char* target_buffer_ptr = static_cast<char*>(target_buffer);

    // Lower memory area not covered by prefetch
    if (requested_offset < cs.offset) {
        int const pre_count_bytes = static_cast<int>(cs.offset - requested_offset);
        
        missed_bytes += pre_count_bytes;
        int const pre_count = pre_count_bytes / type_size;

        // Retrieve missed data from file
        mpi_iotrace.Read_Sync_Start(pre_count, cs.type, requested_offset);
        err = PMPI_File_read_at(cs.fh, requested_offset, target_buffer, pre_count, cs.type, &cache_miss_status);
        mpi_iotrace.Read_Sync_End();

        PMPI_Get_count(&cache_miss_status, cs.type, &total_read_elements);

        // Advance target buffer pointer
        target_buffer_ptr = target_buffer_ptr + (pre_count * type_size);
    } else {
        copy_start = requested_offset - cs.offset;
    }

    if (err != MPI_SUCCESS) return err;

    MPI_Offset const fetched_end = cs.offset + (cs.count * type_size);
    MPI_Offset const requested_end = requested_offset + (requested_count * type_size);

    MPI_Offset const begin = std::max(cs.offset, requested_offset);
    MPI_Offset const end = std::min(fetched_end, requested_end);

    MPI_Offset const copy_count = end - begin;

    // Copy data from prefetch buffer to 
    std::memcpy(target_buffer_ptr, buffer.get() + copy_start, copy_count);
    total_read_elements += copy_count;

    // Advance target buffer pointer by copied data
    target_buffer_ptr = target_buffer_ptr + (copy_count * type_size);

    // Upper memory area not covered by  prefetch
    if (fetched_end < requested_end) {
        int const post_count_bytes = static_cast<int>(requested_end - fetched_end);
        missed_bytes += post_count_bytes;
        int const post_count = post_count_bytes / type_size;

        // Retrieve missing data from file
        mpi_iotrace.Read_Sync_Start(post_count, cs.type, fetched_end);
        err = PMPI_File_read_at(cs.fh, fetched_end, target_buffer_ptr, post_count, cs.type, &cache_miss_status);
        mpi_iotrace.Read_Sync_End();

        int post_read;
        PMPI_Get_count(&cache_miss_status, cs.type, &post_read);
        total_read_elements += post_read;
    }

    Prefetcher::Log<VerbosityLevel::BASIC_LOG>(
            "Prefetcher > Missed %ld b of %ld b\n", missed_bytes, requested_count * type_size);
    
    // Update total read bytes for MPI_Status
    if(status != MPI_STATUS_IGNORE && status != MPI_STATUSES_IGNORE) {
        PMPI_Status_set_elements(status, cs.type, total_read_elements);
        status->MPI_ERROR = err;
    }

    return err;
}

/**
 * @brief Find prefetch in cache by call signature and return best fitting request from cache
 * @param requested_cs [in] Signature of the retrieved call
 * @return Best fitting request if any was found
 * @note Pops request from cache
 */
std::optional<Request> RequestCache::take_prefetched_by_call_signature(CallSignature& requested_cs) {

    //TODO better cs finding
    const std::lock_guard<std::mutex> lock(request_lock);

    // Get requests for correct file
    auto it = requests.find(requested_cs.fh);
    bool const found = it != requests.end();

    if(found) {
        MPI_Offset max_overlap = 0;

        int type_size;
        PMPI_Type_size(requested_cs.type, &type_size);
        
        MPI_Offset const requested_begin = requested_cs.offset;
        MPI_Offset const requested_end = requested_cs.offset + (type_size * requested_cs.count);

        std::optional<decltype(it->second.begin())> max_iterator = std::nullopt;
        
        // Iterate through prefetches for best overlap
        for(auto fetch_request = it->second.begin(); fetch_request != it->second.end(); fetch_request++) {
            if (fetch_request->cs.type != requested_cs.type) continue;

            // Total overlap area
            MPI_Offset const fetched_begin = std::max(fetch_request->cs.offset, requested_begin);
            MPI_Offset const fetched_end = std::min(fetch_request->cs.offset + (type_size * fetch_request->cs.count), requested_end);

            // No overlap
            if (fetched_end <= fetched_begin) continue;

            MPI_Offset const overlap = fetched_end - fetched_begin;

            // If current prefetch povides best fit
            if (overlap > max_overlap) {
                max_overlap = overlap;
                max_iterator = fetch_request;
            }
        }

        // No overlapping prefetch found
        if(max_overlap == 0) return std::nullopt;

        // Move request out of cache
        Request request = std::move(*(max_iterator.value()));
        total_cached_bytes -= request.cs.count * type_size;
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
 * @param fh [in] Remove requests associated with this handle 
 */
void RequestCache::remove_file(MPI_File& fh) {
    const std::lock_guard<std::mutex> lock(request_lock);

    auto it = requests.find(fh);
    if(it != requests.end()) {
        for(auto& req : it->second) {
            PMPI_Cancel(req.mpi_request.get());
            mpi_iotrace.Read_Async_Required(req.mpi_request.get());
            PMPI_Wait(req.mpi_request.get(), MPI_STATUS_IGNORE);
            mpi_iotrace.Read_Async_End(req.mpi_request.get());
            int type_size;
            PMPI_Type_size(req.cs.type, &type_size);
            total_cached_bytes -= req.cs.count * type_size;
        }
        requests.erase(it);
    }
}

/**
 * @brief Empties cache
 */
void RequestCache::remove_all() {
    const std::lock_guard<std::mutex> lock(request_lock);

    for(auto it = requests.begin(); it != requests.end(); it++) {
        for(auto& req : it->second) {
            PMPI_Cancel(req.mpi_request.get());
            mpi_iotrace.Read_Async_Required(req.mpi_request.get());
            PMPI_Wait(req.mpi_request.get(), MPI_STATUS_IGNORE);
            mpi_iotrace.Read_Async_End(req.mpi_request.get());
            int type_size;
            PMPI_Type_size(req.cs.type, &type_size);
            total_cached_bytes -= req.cs.count * type_size;
        }
    }
    requests.clear();
}

/**
 * @brief Frees space from the cache
 * @param bytes_to_evict [in] Number of bytes to free in cache
 * @note Call only with request_lock engaged
 * @return true if enough space has been freed
 */
bool RequestCache::evict_request(size_t bytes_to_evict) {

    // Continue removing requests until enough is freed
    while(bytes_to_evict > 0) {
        
        std::optional<decltype(requests.begin())> min_req;
        std::optional<double> oldest_access;

        // Search throug all file entries in cache
        for(auto it_req = requests.begin(); it_req != requests.end(); it_req++) {
            // First element is always the oldest
            if(!oldest_access || oldest_access.value() > it_req->second.begin()->last_access) {
                min_req = it_req; 
                oldest_access = it_req->second.begin()->last_access;
            }
        }

        // No smallest found
        if (!min_req) return false; 

        // Evict oldest entry
        Request const& to_delete = *min_req.value()->second.begin();
        PMPI_Cancel(to_delete.mpi_request.get());

        mpi_iotrace.Read_Async_Required(to_delete.mpi_request.get());
        PMPI_Wait(to_delete.mpi_request.get(), MPI_STATUS_IGNORE);
        mpi_iotrace.Read_Async_End(to_delete.mpi_request.get());

        int type_size;
        PMPI_Type_size(to_delete.cs.type, &type_size);

        size_t const evicted_bytes = to_delete.cs.count * type_size;
        min_req.value()->second.erase(min_req.value()->second.begin());
        
        if (min_req.value()->second.empty()) requests.erase(min_req.value());
        
        Prefetcher::Log<VerbosityLevel::BASIC_LOG>(
            "Prefetcher > Cache full >> evicting request with %ld bytes\n", evicted_bytes);
            
        // Update cached bytes
        bytes_to_evict = evicted_bytes > bytes_to_evict? 0 : bytes_to_evict - evicted_bytes;
        total_cached_bytes -= evicted_bytes;
    }

    return true;
}
/**
 * @brief Insert request into cache
 * @param fh [in] File request corresponds to
 * @param request [in] Request to be inserted
 * @note Cancels request if it could not be inserted
 */
void RequestCache::insert_request(MPI_File fh, Request request) {
    const std::lock_guard<std::mutex> lock(request_lock);
    int type_size;
    PMPI_Type_size(request.cs.type, &type_size);
    size_t const request_size = request.cs.count * type_size;

    // Check if enough space is in cache
    if(total_cached_bytes + request_size > max_cache_size_bytes) {
    
        // Not enough space could be freed
        if(!evict_request(total_cached_bytes + request_size - max_cache_size_bytes)) {
            Prefetcher::Log<VerbosityLevel::BASIC_LOG>(
            "Prefetcher > Cache full >> no prefetch possible\n");
            // Cancel Request that could not be inserted
            PMPI_Cancel(request.mpi_request.get());
            mpi_iotrace.Read_Async_Required(request.mpi_request.get());
            PMPI_Wait(request.mpi_request.get(), MPI_STATUS_IGNORE);
            mpi_iotrace.Read_Async_End(request.mpi_request.get());
            return;
        }
    }

    total_cached_bytes += request_size;

    requests[fh].emplace_back(std::move(request));
}

/**
 * @brief Free space in cache
 * @param required_space [in] Space to be reserved in bytes
 */
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

/**
 * @brief Initializes prefetching and spawns asnyc prefetch thread
 * @param max_cache_size [in] Maximum number of bytes cached by prefetcher
 * @param max_request_size [in] Maximum number of bytes a cached request can have
 * @param io_frequency [in] Frequency of the programs io access pattern
 */
void Prefetcher::init(double io_frequency, size_t max_cache_size, size_t max_request_size)
{
    if(inititalized) return;
    Prefetcher::Log<VerbosityLevel::BASIC_LOG>(
            "Initializing prefetcher with frequency %f, max cache size %.2f Mb, max file size %.2f Mb.\n",
             io_frequency, static_cast<double>(max_cache_size) / 1'000'000., static_cast<double>(max_request_size) / 1'000'000.);
    // Test if MPI multithreading enabled
    int mpi_provided;
    PMPI_Query_thread(&mpi_provided);
    if(mpi_provided != MPI_THREAD_MULTIPLE) {
        Prefetcher::Log<VerbosityLevel::BASIC_LOG>(
            "MPI multithreading not activated. I/O prefetching deactivated.");
        return;
    }

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &processes);
    
    callDBs = std::make_shared<CallDB>(io_frequency);
    requests_in_transit = std::make_shared<RequestCache>(max_cache_size, max_request_size);

    stop_token = false;

    // Initiate prefetch thread
    prefetching_thread = std::thread([&] {Prefetcher::prefetching_routine(requests_in_transit, callDBs, stop_token);});
    phase_start = std::numeric_limits<double>::min();
    inititalized = true;
#if OVERHEAD == 1
    prefetch_missed_bytes = 0;
    total_missed_bytes = 0;
    init_time = PMPI_Wtime();
    local_overhead = 0.0;
#endif
}

/**
 * @brief Finish prefetching operation, clear cache and join prefetch thread
 */
void Prefetcher::finalize() {
    if(!inititalized) return;

    start_overhead();
    stop_token.store(true);

    {
        std::lock_guard<std::mutex> const lock(callDBs->event_mutex);
        callDBs->event_ready = true;
    }

    callDBs->prefetch_event.notify_all();
    
    if(prefetching_thread.joinable())
        prefetching_thread.join();

    requests_in_transit->remove_all();
    
    end_overhead();
    
#if OVERHEAD == 1
    // Collect statistics across ranks
    double global_overhead;
    unsigned long long global_prefetch_missed_bytes;
    unsigned long long global_total_missed_bytes;

    double local_overhead = total_overhead.load();
    unsigned long long local_prefetch_missed_bytes = static_cast<unsigned long long>(prefetch_missed_bytes.load());
    unsigned long long local_total_missed_bytes = static_cast<unsigned long long>(total_missed_bytes.load());

    int root = 0;
    MPI_Reduce(&local_overhead, &global_overhead, 1, MPI_DOUBLE, MPI_SUM, root, MPI_COMM_WORLD);
    MPI_Reduce(&local_prefetch_missed_bytes, &global_prefetch_missed_bytes, 1, MPI_UNSIGNED_LONG_LONG, MPI_SUM, root, MPI_COMM_WORLD);
    MPI_Reduce(&local_total_missed_bytes, &global_total_missed_bytes, 1, MPI_UNSIGNED_LONG_LONG, MPI_SUM, root, MPI_COMM_WORLD);

    // Print statistics
    if (rank == root) {
        double const run_time = MPI_Wtime() - init_time;

        Prefetcher::Log<VerbosityLevel::BASIC_LOG>(
                "Aggregated prefetching overhead: %.2fs, %.2f of runtime\n", global_overhead, run_time * static_cast<double>(processes));
        Prefetcher::Log<VerbosityLevel::BASIC_LOG>(
                "Average prefetching overhead:  %.2fs, %.2f of runtime\n", global_overhead / processes, run_time);

        Prefetcher::Log<VerbosityLevel::BASIC_LOG>(
                "Aggregated prefetching missed bytes: %ld\n", global_prefetch_missed_bytes);
        Prefetcher::Log<VerbosityLevel::BASIC_LOG>(
                "Average prefetching missed bytes: %ld\n", global_prefetch_missed_bytes / static_cast<unsigned long long>(processes));

        Prefetcher::Log<VerbosityLevel::BASIC_LOG>(
                "Aggregated total missed bytes: %ld\n", global_total_missed_bytes + global_prefetch_missed_bytes);
        Prefetcher::Log<VerbosityLevel::BASIC_LOG>(
                "Average total missed bytes: %ld\n", (global_total_missed_bytes + global_prefetch_missed_bytes) / static_cast<unsigned long long>(processes));
        
    }
#endif

    inititalized = false;
}

/**
 * @brief Get MPI_Request corresponding to async call from prefetch cache. Replaces the actual file call.
 * @param cs [in] Signature of the retrieved call
 * @param mpi_request [out] Returns MPI_Request handle corresponding to data
 * @param target_buffer [in] Target buffer for read data
 */
int Prefetcher::retrieve_read_async(CallSignature& cs, MPI_Request* mpi_request, void* target_buffer) 
{
    if(!inititalized) {
        int err;
#if defined BW_LIMIT && BW_LIMIT_GRANULARITY > 1
        mpi_iotrace.apply_file_specific_bw(false, cs.fh, cs.count, cs.type);
#endif
        mpi_iotrace.Read_Async_Start(cs.count, cs.type, mpi_request, cs.fh, cs.offset);

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

    // Get file pointer offset for offset read
    if (cs.ct == CallType::Read) {
        MPI_Offset total_offset;
        PMPI_File_get_position(cs.fh, &total_offset);
        cs.offset = total_offset;
    }

    // If request prefetched
    if (std::optional<Request> request = requests_in_transit->take_prefetched_by_call_signature(cs); request.has_value()) {
        // Set dummy handle
        *mpi_request = MPI_REQUEST_NULL;

        std::lock_guard const lock(ar_lock);

        // Keep request data reserved for later retrival
        async_requests.emplace(mpi_request, std::move(AsyncBuffers(std::move(request.value()), target_buffer, cs.count, cs.offset)));

        err = MPI_SUCCESS;
    } else { // Request not prefetched
        Prefetcher::Log<VerbosityLevel::DETAILED_LOG>(
            "Prefetch miss on async read. Fetching read manually\n");
        end_overhead();
#if defined BW_LIMIT && BW_LIMIT_GRANULARITY > 1
        mpi_iotrace.apply_file_specific_bw(false, cs.fh, cs.count, cs.type);
#endif
        mpi_iotrace.Read_Async_Start(cs.count, cs.type, mpi_request, cs.fh, cs.offset);
        err = PMPI_File_iread_at(cs.fh, cs.offset, target_buffer, cs.count, cs.type, mpi_request);
        start_overhead();
        #if OVERHEAD == 1
            // Track as missed bytes
            int type_size;
            int const err = PMPI_Type_size(cs.type, &type_size);
            if (err == MPI_SUCCESS) { 
                total_missed_bytes.fetch_add(static_cast<size_t>(type_size) * static_cast<size_t>(cs.count));
            }
        #endif
    }

    // Offset file pointer for pointer consistency
    if (cs.ct == CallType::Read) {
        int type_size;
        PMPI_Type_size(cs.type, &type_size);
        PMPI_File_seek(cs.fh, cs.offset + (cs.count * type_size), MPI_SEEK_SET);
    }

    {
        std::lock_guard const lock(rs_lock);
        // Track request data
        request_signature.insert_or_assign(mpi_request, cs);
    }
    end_overhead();
    return err;
}

/**
 * @brief Check if request is a dummy handle
 * @param request [in] Verified request handel
 * @return Actual handle associated with file call
 */
MPI_Request* Prefetcher::determine_valid_request(MPI_Request* request) {
    if(!inititalized) return request;

    auto it = async_requests.find(request);
    if (it != async_requests.end()) {
        // Dummy handle, return prefetch handle
        return it->second.prefetch_request.mpi_request.get();
    } else {
        // Valid file handle
        return request;
    }
}

/**
 * @brief Copy results of transaction into buffer
 * @param request [in] Request handle for call
 * @param status [out] Status of the call retrieval
 * @return MPI error value
 */
int Prefetcher::fetch_read_async_wait(MPI_Request* request, MPI_Status* status) {  
    
    MPI_Request* valid_request = determine_valid_request(request);
    mpi_iotrace.Read_Async_Required(valid_request);
    mpi_iotrace.Write_Async_Required(valid_request);
    int err = PMPI_Wait(valid_request, status);
    mpi_iotrace.Read_Async_End(valid_request);
    mpi_iotrace.Write_Async_End(valid_request);

    if(!inititalized || err != MPI_SUCCESS) return err;
    
    err = fetch_read_async_impl(request, status);
    
    return err;
}

/**
 * @brief Copy results of transaction into buffer
 * @param count [in] Status of the MPI call
 * @param requests [out] Request handles for call
 * @param statuses [out] Statuses of the call retrieval
 * @return MPI error value
 */
int Prefetcher::fetch_read_async_wait_all(int count, MPI_Request* requests, MPI_Status* statuses) {
         
    int err = MPI_SUCCESS;   

    for (int i = 0; i < count; i++)
	{
        MPI_Request* valid_request = determine_valid_request(&requests[i]);

        MPI_Status* status = statuses == MPI_STATUSES_IGNORE? 
            MPI_STATUS_IGNORE : &statuses[i];

		mpi_iotrace.Read_Async_Required(valid_request);
		mpi_iotrace.Write_Async_Required(valid_request);
        err = PMPI_Wait(valid_request, status);
		mpi_iotrace.Read_Async_End(valid_request);
		mpi_iotrace.Write_Async_End(valid_request);

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
 * @param request [in] Request handle for call
 * @param status [out] Status  of the call retieval
 * @param flag [out] True if test successfull
 * @return MPI error value
 */
int Prefetcher::fetch_read_async_test(MPI_Request* request, MPI_Status* status, int* flag) {  
    
    MPI_Request* valid_request = determine_valid_request(request);

    int err = PMPI_Test(valid_request, flag, status);

#if TEST == 1
	mpi_iotrace.Read_Async_End(valid_request, *flag);
	mpi_iotrace.Write_Async_End(valid_request, *flag);
#endif

    if(!inititalized || err != MPI_SUCCESS || !*flag) return err;
    
    err = fetch_read_async_impl(request, status);   
    return err;
}

/**
 * @brief Copy results of successful test into buffer
 * @param requests [in] Request handle for call
 * @param statuses [out] Statuses of the call retieval
 * @param flag [out] true if test successfull
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
		mpi_iotrace.Read_Async_End(valid_request, *flag);
		mpi_iotrace.Write_Async_End(valid_request, *flag);
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

/**
 * @brief Copy results of successful request into buffer
 * @param request [in] Request handle for call
 * @param status [out] Status of the call retieval
 * @return MPI error value
 */
int Prefetcher::fetch_read_async_impl(MPI_Request* request, MPI_Status* status) {
    int err = MPI_SUCCESS;
    start_overhead();
    {
        std::lock_guard const lock(ar_lock);
        auto it = async_requests.find(request);

        // If prefetch was reserved for this request call
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
        std::lock_guard const lock(rs_lock);
        auto rs = request_signature.find(request);
        if (rs != request_signature.end())
        {
            double const time_req = MPI_Wtime();
            register_transaction(rs->second, time_req);
            request_signature.erase(rs);
        }
    }

    end_overhead();
    return err;
}

/**
 * @brief Removes all prefetches connected to file from the prefetching cache
 * @param file [in] Remove all prefetches associated with file
 */
void Prefetcher::close_file(MPI_File file) {
    if(!inititalized) return;

    start_overhead();
    std::lock_guard const lock(callDBs->call_lock);

    auto it = callDBs->prefetch_infos.find(file);
    if(it != callDBs->prefetch_infos.end()) {
        callDBs->prefetch_infos.erase(it);
    }
    
    requests_in_transit->remove_file(file);
    end_overhead();
}

/**
 * @brief Get results from prefetch cache, block if necessary. Replaces the actual file call.
 * @param cs [in] Signature of the retrieved call
 * @param target_buffer [out] Target buffer for read data
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
    double const time_req = MPI_Wtime();

    // Get file pointer offset for offset read
    if (cs.ct == CallType::Read) {
        MPI_Offset total_offset;
        PMPI_File_get_position(cs.fh, &total_offset);
        cs.offset = total_offset;
    }

    int err;
    // If request prefetched
    if (std::optional<Request> request = requests_in_transit->take_prefetched_by_call_signature(cs); request.has_value()) {
        mpi_iotrace.Read_Async_Required(request.value().mpi_request.get());
        err = PMPI_Wait(request.value().mpi_request.get(), status);
        mpi_iotrace.Read_Async_End(request.value().mpi_request.get());
        if(err == MPI_SUCCESS) {
            size_t missed_bytes = 0;
            request.value().fill_buffer_with_request(target_buffer, cs.offset, cs.count, status, missed_bytes);
#if defined BW_LIMIT && BW_LIMIT_GRANULARITY == 1
	        mpi_iotrace.apply_bw_limit();
#elif defined CUSTOM_MPI
	        mpi_iotrace.set_custom_throughput();
#endif 
#if OVERHEAD == 1
            prefetch_missed_bytes.fetch_add(missed_bytes);
#endif
        }
    } else { // Request not prefetched
        Prefetcher::Log<VerbosityLevel::DETAILED_LOG>(
            "Prefetch miss on sync read. Fetching read manually\n");

        end_overhead();
        mpi_iotrace.Read_Sync_Start(cs.count, cs.type, cs.offset);
        err = PMPI_File_read_at(cs.fh, cs.offset, target_buffer, cs.count, cs.type, status); 
        mpi_iotrace.Read_Sync_End();
        start_overhead();
        #if OVERHEAD == 1
        int size;
        int const err = PMPI_Type_size(cs.type, &size);
        if (err == MPI_SUCCESS) total_missed_bytes.fetch_add(static_cast<size_t>(size) * static_cast<size_t>(cs.count));
        #endif
    }
    
    // Offset file pointer for pointer consistency
    if (cs.ct == CallType::Read) {
        int size;
        MPI_Type_size(cs.type, &size);
        PMPI_File_seek(cs.fh, cs.offset + (cs.count * size), MPI_SEEK_SET);
    }
    // Track request data
    register_transaction(cs, time_req);
    end_overhead();
    return err;
}

/**
 * @brief Register a transaction for prefetching in next IO phase
 * @param cs [in] Signature of the registered call
 * @param time_req [in] Time stamp when result of transaction is required
 */
void Prefetcher::register_transaction(CallSignature &cs, double time_req)
{
    int type_size;
    int const err = PMPI_Type_size(cs.type, &type_size);

    if(!inititalized
        || err != MPI_SUCCESS 
        || static_cast<size_t>(type_size) * static_cast<size_t>(cs.count) > requests_in_transit->max_cache_bytes() 
        || !is_contiguous_type(cs.type)) {
        return;
    }

    {
        std::lock_guard<std::mutex> const lock(callDBs->call_lock);

        auto it = callDBs->prefetch_infos.find(cs.fh);

        if(it == callDBs->prefetch_infos.end()) {
            PrefetchInfo prefetch_info;
            it = callDBs->prefetch_infos.emplace(cs.fh, prefetch_info).first;
        }

        it->second.add_callInfo(time_req, cs.offset, cs.count, cs.type);
    }

    {
        std::lock_guard<std::mutex> const lock(callDBs->event_mutex);
        callDBs->event_ready = true;
    }

    callDBs->prefetch_event.notify_all();
}

/**
* @brief Checks if an io phase is done. Tracks patterns with frequency
*/
void Prefetcher::check_phase() {
    double const now = PMPI_Wtime();

    std::lock_guard const lock(callDBs->call_lock);
    if(now - phase_start > 0.5 * callDBs->io_interval) {
        for(auto& pi : callDBs->prefetch_infos) {
            pi.second.phase_end();
        }
        phase_start = now;
    }
}

/**
 * @brief Is MPI_Datatype contiguous
 * @param type [in] Type to check
 * @return Is type contiguous
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
            bool const flag = is_contiguous_type(types[0]);

            int ni, na, nt, cb;
        
            PMPI_Type_get_envelope(types[0], &ni, &na, &nt, &cb);
            if (cb != MPI_COMBINER_NAMED) {
                PMPI_Type_free(types.data());
            }

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
 * @param request_cache [in] Pending requests
 * @param pattern_data [in] Information about call patterns
 * @param stop_token [in] Signal for termination of prefetching routine
 */
void Prefetcher::prefetching_routine(std::shared_ptr<RequestCache> request_cache, std::shared_ptr<CallDB> pattern_data, std::atomic<bool> &stop_token) 
{
    Prefetcher::Log<VerbosityLevel::DETAILED_LOG>(
            "Prefetcher start routine\n");
    std::optional<MPI_File> next_prefetch;
    while(!stop_token.load()) {
        
        double next_prefetch_time = 0.0;
        next_prefetch = determine_next_prefetch(pattern_data, next_prefetch_time);

        if(next_prefetch) { // Valid prefetch was determined
            double const now = PMPI_Wtime();
            double const sleep_s = next_prefetch_time - now;
            if(sleep_s > 0.) { // Wait until prefetch
                Prefetcher::Log<VerbosityLevel::DETAILED_LOG>(
                "Prefetching in %.2f seconds\n", sleep_s);
                std::this_thread::sleep_for(std::chrono::duration<double>(sleep_s));
            }
            if(stop_token.load()) break;

            double call_time;
            std::optional<CallSignature> prefetch_signature;
            {
                std::lock_guard<std::mutex> const lock(pattern_data->call_lock);
                auto it = pattern_data->prefetch_infos.find(*next_prefetch);
                if (it != pattern_data->prefetch_infos.end()) {
                    CallInfo &next_prefetch = it->second.get_next_prefetch();
                    prefetch_signature = determine_prefetch_signature(next_prefetch, it->first);
                    call_time = next_prefetch.last_call_time + pattern_data->io_interval;
                }
                // Advance in pattern
                it->second.advance_prefetch();
            }

            if(stop_token.load()) break;
            
            if(prefetch_signature) 
                prefetch_transaction(*prefetch_signature, request_cache, call_time);
        
        } else { // Wait on new prefetch info to arrive
            std::unique_lock<std::mutex> lock(pattern_data->event_mutex);
            
            if(!pattern_data->event_ready && !stop_token) {
                pattern_data->prefetch_event.wait(lock, [&]{return pattern_data->event_ready || stop_token;});
            }
        }
    }
}

/**
 * @brief Starts prefetching of transaction
 * @param cs [in] Signature of the prefetched call
 * @param request_cache [in] MPI_Request associated with prefetch call
 * @param call_time [in] Timepoint when prefetch is expected
 */
void Prefetcher::prefetch_transaction(CallSignature& cs, std::shared_ptr<RequestCache>& request_cache, double call_time)
{
    Prefetcher::Log<VerbosityLevel::DETAILED_LOG>(
                "Start Prefetch\n");
    int type_size = 0;
    auto err = PMPI_Type_size(cs.type, &type_size);

    if(err != MPI_SUCCESS || !request_cache->reserve_cache_space(type_size * cs.count)) { 
        return;
    }
    
    Request request = Request(type_size * cs.count, cs, PMPI_Wtime());

    // Start actual prefatch call
#if defined BW_LIMIT && BW_LIMIT_GRANULARITY > 1
    mpi_iotrace.apply_prefetch_limit_impl(type_size * cs.count, call_time - MPI_Wtime());
#endif
    mpi_iotrace.Read_Async_Start(cs.count, cs.type, request.mpi_request.get(), cs.fh, cs.offset);
    err = PMPI_File_iread_at(cs.fh, cs.offset, request.buffer.get(), cs.count, cs.type, request.mpi_request.get());

    if(err != MPI_SUCCESS || type_size <= 0) { return;
}

    // Add prefetch to overall cache
    Prefetcher::Log<VerbosityLevel::BASIC_LOG>(
            "Prefetcher > Prefetch %ld b with offset %ld\n", cs.count * type_size, cs.offset);

    request_cache->insert_request(cs.fh, std::move(request));
}

/**
 * @brief Determines next prefetched MPI_File from callDB
 * @param pattern_data [in] Information about previous calls
 * @param next_prefetch_time [out] timestamp of the next prefetch call
 * @return Next file to prefetch on
 */
std::optional<MPI_File> Prefetcher::determine_next_prefetch(std::shared_ptr<CallDB>& pattern_data, double &next_prefetch_time) {
    Prefetcher::Log<VerbosityLevel::DETAILED_LOG>(
            "Determining next prefetch \n");
    std::lock_guard<std::mutex> const lock(pattern_data->call_lock);

    std::optional<decltype(pattern_data->prefetch_infos.begin())> min_element;

    for(auto it = pattern_data->prefetch_infos.begin(); it != pattern_data->prefetch_infos.end(); it++) {
    
        if(!it->second.valid_prefetch) {
            continue;
        }
        CallInfo& ci = it->second.get_next_prefetch();

        if (ci.prefetched) continue; // Already prefetched at retrieval 

        if(!ci.next_prefetch_time) {
            ci.next_prefetch_time = calculate_next_prefetch(ci, pattern_data->io_interval, pattern_data->prefetch_ratio, pattern_data->prefetch_bandwidth);
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
    Prefetcher::Log<VerbosityLevel::DETAILED_LOG>(
        "No suitable prefetch found\n");
    // No min element found
    std::lock_guard<std::mutex> const ev_lock(pattern_data->event_mutex);
    pattern_data->event_ready = false;

    return std::nullopt;
}

/**
 * @brief Determine call signature of next prefetch
 * @param call_info [in] Information about previous calls
 * @param fh [in] File on which to prefetch
 * @return Call signature for prefetching
 */
CallSignature Prefetcher::determine_prefetch_signature(CallInfo& call_info, MPI_File fh) {
    MPI_Offset offset = 0;
    int count = 0;

    if constexpr (CONSIDER_PREV_N > 1) {
        int avg_offset_diff = 0;
        int avg_count_diff = 0;
        for(int i = 1; i < CONSIDER_PREV_N; i++) {
            avg_offset_diff += static_cast<int>(call_info.total_offset[i-1]) - static_cast<int>(call_info.total_offset[i]);
            avg_count_diff += call_info.count[i-1] - call_info.count[i];
        }

        avg_offset_diff = avg_offset_diff / (CONSIDER_PREV_N - 1);
        avg_count_diff = avg_count_diff / (CONSIDER_PREV_N - 1);

        offset = call_info.total_offset[0] + avg_offset_diff;
        count = call_info.count[0] + avg_count_diff;
    } else if constexpr (CONSIDER_PREV_N == 1) {
        count = call_info.count[0];
        offset = call_info.total_offset[0];
    }

    return CallSignature(fh, count, call_info.datatype, offset, CallType::ReadAt);
}

/**
 * @brief Calculates the time point when the next prefetch should start
 * @param call_info [in] Information about previous calls
 * @param io_period [in] Length of one IO period
 * @param prefetch_ratio [in] How far into an io period is prefetching started
 * @param bandwidth [in, optional] min bandwidth for prefetching
 * @return 
 */
std::optional<double> Prefetcher::calculate_next_prefetch(CallInfo& call_info, double io_period, double prefetch_ratio, std::optional<double> bandwidth) {
    
    double const next_call_time = io_period + call_info.last_call_time;
    std::optional<double> const prefetch_time; 

    if (bandwidth.has_value()) {
        int type_size;

        int err = PMPI_Type_size(call_info.datatype, &type_size);
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
 * @param frequency [in] prefetching frequency
 */
void Prefetcher::set_io_frequency(double frequency) {
    if(!inititalized) {
        init(frequency);
    }

    if (frequency > 0.0) {
        std::lock_guard<std::mutex> const lock(callDBs->call_lock);

        callDBs->io_interval = (1/frequency);
    }
}

/**
 * @brief Sets bandwidth for prefetching
 * @param bandwidth [in] prefetching bandwidth
 */
void Prefetcher::set_prefetch_bandwidth(double bandwidth) {
    if(!inititalized) return;

    std::lock_guard<std::mutex> const lock(callDBs->call_lock);
    callDBs->prefetch_bandwidth = bandwidth;
}
#endif
