#include "prefetch.h"
#include <thread>
#include <stdio.h>
#include <algorithm>
#include <cstring>

using CallType = CallSignature::CallType;
using Request = RequestCache::Request;

int Request::fill_buffer_with_request(void* target_buffer, MPI_Offset total_offset, int count) {
    int err = MPI_SUCCESS;
    char* buffer_ptr = static_cast<char*>(target_buffer);
    if (total_offset < cs.offset) {
        int pre_count = static_cast<int>(total_offset - cs.offset);
        err = MPI_File_read_at(cs.fh, total_offset, target_buffer, pre_count, cs.type, MPI_STATUS_IGNORE);
        buffer_ptr = buffer_ptr + pre_count;
    }

    if (err != MPI_SUCCESS) return err;

    MPI_Offset copy_count = std::min(cs.count, count);

    std::memcpy(buffer_ptr, buffer.data(), copy_count);

    buffer_ptr = buffer_ptr + copy_count;

    MPI_Offset req_end = cs.offset + cs.count;
    MPI_Offset fet_end = total_offset + count;

    if (req_end < fet_end) {
        int post_count = static_cast<int>(fet_end - req_end);
        err = MPI_File_read_at(cs.fh, req_end, buffer_ptr, post_count, cs.type, MPI_STATUS_IGNORE);
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
std::optional<Request> RequestCache::take_prefetched_by_call_signature(CallSignature& cs, MPI_Offset total_offset) {

    //TODO better cs finding
    const std::lock_guard<std::mutex> lock(request_lock);

    auto it = requests.find(cs.fh);
    bool found = it != requests.end();

    if(found) {
        MPI_Offset max_overlap = 0;

        MPI_Offset fet_begin = total_offset;
        MPI_Offset fet_end = total_offset + cs.count;

        std::optional<decltype(it->second.begin())> max_iterator = {};

        for(auto req = it->second.begin(); req < it->second.end(); req++) {
            if (req->cs.type != cs.type) continue;

            MPI_Offset begin = std::max(req->cs.offset, fet_begin);
            MPI_Offset end = std::min(req->cs.offset + req->cs.count, fet_end);

            if (end <= begin) continue;

            MPI_Offset overlap = end - begin;

            if (overlap > max_overlap) {
                max_overlap = overlap;
            }

            max_iterator = req;
        }
        if(max_overlap == 0) return {}; 
        Request request = std::move(*(max_iterator.value()));
        it->second.erase(max_iterator.value());
        return request;
    }

    return {};
}

/**
 * @brief Removes requests associated with file
 */
void RequestCache::remove_file(MPI_File& fh) {
    const std::lock_guard<std::mutex> lock(request_lock);

    auto it = requests.find(fh);
    for(auto& req : it->second) {
        if(MPI_Wait(&req.request, MPI_STATUS_IGNORE) != MPI_SUCCESS) abort();
    }
    requests.erase(fh);
}

/**
 * @brief Frees space from the cache
 * @param bytes_to_evict number of bytes to remove free in cache
 * @note Call only with request_lock engaged
 * @return true if enough space has been freed
 */
bool RequestCache::evict_request(int bytes_to_evict) {

    while(bytes_to_evict > 0) {
        std::optional<decltype(requests.begin())> min_req;
        std::optional<decltype(requests.begin()->second.begin())> min_vec;

        for(auto it_req = requests.begin(); it_req != requests.end(); it_req++) {
            for(auto it_vec = it_req->second.begin(); it_vec != it_req->second.end(); it_vec++) {
                
                if(min_vec && (*min_vec)->last_access < it_vec->last_access) continue;
                
                min_vec = it_vec;
                min_req = it_req; 
            }
        }

        if (!min_req) return false; // No smallest found

        int evicted_bytes = min_vec.value()->buffer.size();
        min_req.value()->second.erase(*min_vec);

        bytes_to_evict -= evicted_bytes;
        total_cached_bytes -= evicted_bytes;
    }

    return true;
}

void RequestCache::insert_request(MPI_File fh, Request request) {
    const std::lock_guard<std::mutex> lock(request_lock);

    int request_size = request.buffer.size();

    if(total_cached_bytes + request_size > max_cache_size_bytes) {
        evict_request(total_cached_bytes + request_size - max_cache_size_bytes);
    }

    total_cached_bytes += request_size;

    requests[fh].emplace_back(std::move(request));
}

Prefetcher::Prefetcher() : inititalized(false) {}

/**
 * @brief Initializes prefetching and spawns asnyc prefetch thread
 */
void Prefetcher::init(IOtraceMPI *mpi_iotrace, int *mpi_provided)
{
    //test if MPI multithreading enabled
    if(*mpi_provided != MPI_THREAD_MULTIPLE) {
        Prefetcher::Log<VerbosityLevel::BASIC_LOG>(
            "MPI multithreading not activated. I/O prefetching deactivated.");
        return;
    }

    traces = mpi_iotrace;
    callDBs = std::make_shared<CallDB>();
    requests_in_transit = std::make_shared<RequestCache>();

    stop_token = std::make_shared<std::atomic<bool>>(false);

    prefetching_thread = std::thread([&] {Prefetcher::prefetching_routine(requests_in_transit, callDBs, mpi_iotrace, stop_token);});
    inititalized = true;
}

void Prefetcher::finalize() {
    stop_token->store(true);

    {
        std::lock_guard<std::mutex> lock(callDBs->event_mutex);
        callDBs->event_ready = true;
    }

    callDBs->prefetch_event.notify_all();
    
    if(prefetching_thread.joinable())
        prefetching_thread.join();
}

/**
 * @brief Get MPI_Request corresponding to async call from prefetch cache
 * @param cs Signature of the retrieved call
 * @param mpi_request [out] Returns MPI_Request corresponding to prefetched call
 * @param target_buffer Target buffer for read data
 */
int Prefetcher::retrieve_read_asnyc(CallSignature& cs, MPI_Request* mpi_request, void* target_buffer) 
{
    std::vector<std::byte> prefetch_buffer;

    int err;

    MPI_Offset total_offset;
    if (cs.ct == CallType::Read) {
        MPI_File_get_position(cs.fh, &total_offset);
    } else {
        total_offset = cs.offset;
    }


    if (std::optional<Request> request = requests_in_transit->take_prefetched_by_call_signature(cs, total_offset); request.has_value()) {

        async_requests.emplace(mpi_request, std::move(AsyncBuffers(std::move(request.value()), target_buffer, cs.count, total_offset)));

        err = MPI_SUCCESS;
    } else {
        Prefetcher::Log<VerbosityLevel::DETAILED_LOG>(
            "Prefetch miss. Fetching read manually");
        traces->Read_Async_Start(cs.count, cs.type, mpi_request, cs.fh, cs.offset);
        err = PMPI_File_iread_at(cs.fh, cs.offset, target_buffer, cs.count, cs.type, mpi_request);
    }

    request_signature.insert_or_assign(mpi_request, cs);
    return err;
}

/**
 * @brief Copy results of transaction into buffer
 * @param request Signature of the retrieved call
 * @param int Status of the MPI call
 * @return MPI error value
 */
void Prefetcher::fetch_read_async_wait(MPI_Request* request, int err) {
    if (err != MPI_SUCCESS) return;

    auto it = async_requests.find(request);

    if(it != async_requests.end()) {
        it->second.prefetch_request.fill_buffer_with_request(it->second.target_buffer, it->second.total_offset, it->second.count);

        async_requests.erase(it);
    }

    auto rs = request_signature.find(request);
    if (rs != request_signature.end())
    {
        double time_req = MPI_Wtime();
        register_transaction(rs->second, time_req);
        request_signature.erase(rs);
    }
}

/**
 * @brief Copy results of successful test into buffer
 * @param request Signature of the retrieved call
 * @param flag true if test successfull
 * @param int Status of the MPI call
 * @return MPI error value
 */
void Prefetcher::fetch_read_async_test(MPI_Request* request, int* flag, int err) {  
    if(!*flag || err != MPI_SUCCESS) return;

    auto it = async_requests.find(request);

    if (it != async_requests.end()) {
        it->second.prefetch_request.fill_buffer_with_request(it->second.target_buffer, it->second.total_offset, it->second.count);

        async_requests.erase(it);
    }

    auto rs = request_signature.find(request);
    if (rs != request_signature.end())
    {
        double time_req = MPI_Wtime();
        register_transaction(rs->second, time_req);
        request_signature.erase(rs);
    }
    
}

void Prefetcher::close_file(MPI_File file) {
    std::lock_guard lock(callDBs->call_lock);

    auto it = callDBs->prefetch_infos.find(file);
    if(it != callDBs->prefetch_infos.end()) {
        callDBs->prefetch_infos.erase(it);
    }
    
}

/**
 * @brief Get results from prefetch cache, block if necessary
 * @param cs Signature of the retrieved call
 * @param target_buffer Target buffer for read data
 * @param mpi_status [out] Returns status of read operation
 * @return MPI error value
 */
int Prefetcher::retrieve_read_snyc(CallSignature& cs, void* target_buffer, MPI_Status* status)
{
    double time_req = MPI_Wtime();

    MPI_Offset total_offset;
    if (cs.ct == CallType::Read) {
        MPI_File_get_position(cs.fh, &total_offset);
    } else {
        total_offset = cs.offset;
    }

    int err;
    if (std::optional<Request> request = requests_in_transit->take_prefetched_by_call_signature(cs, total_offset); request.has_value()) {
        err = MPI_Wait(&request.value().request, status);
        if(err == MPI_SUCCESS) {
            request.value().fill_buffer_with_request(target_buffer, total_offset, cs.count);
        }
        
    } else {
        Prefetcher::Log<VerbosityLevel::DETAILED_LOG>(
            "Prefetch miss on sync read. Fetching read manually");
        traces->Read_Sync_Start(cs.count, cs.type, cs.offset);
        err = PMPI_File_read(cs.fh, target_buffer, cs.count, cs.type, status);
        traces->Read_Sync_End();
    }

    register_transaction(cs, time_req);
    return err;
}

/**
 * @brief Register a transaction for prefetching in next IO phase
 * @param cs Signature of the registered call
 * @param time_req Time stamp, when result of transaction is required
 * @param prev_call_count Times transactions to this file have been called
 */
void Prefetcher::register_transaction(CallSignature &cs, double time_req)
{
    int type_size;
    int err = MPI_Type_size(cs.type, &type_size);

    if(err != MPI_SUCCESS || type_size * cs.count > requests_in_transit->max_file_size_bytes) {
        return;
    }

    {
        std::lock_guard<std::mutex> lock(callDBs->call_lock);

        auto it = callDBs->prefetch_infos.find(cs.fh);

        MPI_Offset total_offset;
        if (cs.ct == CallType::Read) {
            MPI_File_get_position(cs.fh, &total_offset);
        } else {
            total_offset = cs.offset;
        }

        if(it == callDBs->prefetch_infos.end()) {
            PrefetchInfo prefetch_info = PrefetchInfo(time_req, total_offset, cs.count, cs.type);
            callDBs->prefetch_infos.emplace(cs.fh, prefetch_info);
        } else {
            it->second.total_offset.emplace_back(total_offset);
            it->second.last_call_time = time_req;
            it->second.count.emplace_back(cs.count);
        }
    }

    {
        std::lock_guard<std::mutex> lock(callDBs->event_mutex);
        callDBs->event_ready = true;
    }

    callDBs->prefetch_event.notify_all();
}

/**
 * @brief Prefetch read transactions
 * @param rit Contains requests that are currently pending
 * @param cdb Information about calls
 */
void Prefetcher::prefetching_routine(std::shared_ptr<RequestCache> rit, std::shared_ptr<CallDB> cdb, IOtraceMPI* traces, std::shared_ptr<std::atomic<bool>> stop_token) 
{
    std::optional<MPI_File> next_prefetch;
    while(!stop_token->load()) {
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
                prefetch_signature = determine_prefetch_signature(it->second, it->first);
            }

            if(prefetch_signature) {
                    prefetch_transaction(*prefetch_signature, rit, traces);
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
void Prefetcher::prefetch_transaction(CallSignature& cs, std::shared_ptr<RequestCache>& rc, IOtraceMPI* traces)
{
    int type_size = 0;
    auto err = MPI_Type_size(cs.type, &type_size);

    if(err != MPI_SUCCESS || type_size <= 0) return;
    
    std::vector<std::byte> prefetch_buffer(type_size * cs.count);
    MPI_Request mpi_request;


    // Start actual prefatch call
#if BW_LIMIT_GRANULARITY > 1
		traces->apply_file_specific_bw(false, cs.fh, cs.count, cs.type);
#endif
    traces->Read_Async_Start(cs.count, cs.type, &mpi_request, cs.fh, cs.offset);
    err = PMPI_File_iread(cs.fh, prefetch_buffer.data(), cs.count, cs.type, &mpi_request);

    if(err != MPI_SUCCESS || type_size <= 0) return;

    // Add prefetch to overall cache
    Request request = Request(std::move(mpi_request), std::move(prefetch_buffer), cs, MPI_Wtime());
    
    rc->insert_request(cs.fh, std::move(request));
}

/**
 * @brief Determines next prefetched MPI_File from callDB
 * @param cdb 
 * @param next_prefetch_time timestamp of the next prefetch call
 */
std::optional<MPI_File> Prefetcher::determine_next_prefetch(std::shared_ptr<CallDB>& cdb, double &next_prefetch_time) {
    {
        std::lock_guard<std::mutex> lock(cdb->call_lock);

        double io_period = 1/(cdb->io_frequency);

        std::optional<decltype(cdb->prefetch_infos.begin())> min_element;

        for(auto it = cdb->prefetch_infos.begin(); it != cdb->prefetch_infos.end(); it++) {
            if(!it->second.next_prefetch_time) {
                it->second.next_prefetch_time = calculate_next_prefetch(it->second, io_period, it->second.last_call_time);
            }

            if(it->second.next_prefetch_time) {
                if(!min_element) {
                    min_element = it;
                } else {
                    min_element = 
                        *it->second.next_prefetch_time < *(*min_element)->second.next_prefetch_time?
                            it : min_element;
                }
            }
        }

        if(min_element) {
            next_prefetch_time = *(*min_element)->second.next_prefetch_time;
            return (*min_element)->first;
        }
    }

    
    // No min element found
    std::lock_guard<std::mutex> ev_lock(cdb->event_mutex);
    cdb->event_ready = false;

    return {};
    
}

/**
 * @brief Determine call signature of next prefetch
 * @param prefetch_info
 */
CallSignature Prefetcher::determine_prefetch_signature(PrefetchInfo& prefetch_info, MPI_File fh) {
    MPI_Offset last = prefetch_info.total_offset[prefetch_info.total_offset.size()-1];
    MPI_Offset second_last = prefetch_info.total_offset[prefetch_info.total_offset.size()-2];

    MPI_Offset offset = last + (last - second_last);
    int count = prefetch_info.count.back();

    return CallSignature(fh, count, prefetch_info.datatype, offset, CallType::ReadAt);
}


std::optional<double> Prefetcher::calculate_next_prefetch(PrefetchInfo& prefetch_info, double io_period, double prefetch_bandwidth) {
    int type_size;

    int err = MPI_Type_size(prefetch_info.datatype, &type_size);

    if(err == MPI_SUCCESS) {
        double transfer_time = (prefetch_info.count.back() * type_size) / prefetch_bandwidth;

        double next_call_time = io_period + prefetch_info.last_call_time;

        return (next_call_time - transfer_time);
    }

    return {};
}

// TODO invalidate, if too long in cache

/**
 * @brief Sets IO prefetching frequency
 * @param frequency prefetching frequency
 */
void Prefetcher::set_io_frequency(double frequency) {
    std::lock_guard<std::mutex> lock(callDBs->call_lock);

    callDBs->io_frequency = frequency;
}

/**
 * @brief Sets bandwidth for prefetching
 * @param bandwidth prefetching bandwidth
 */
void Prefetcher::set_prefetch_bandwidth(int bandwidth) {
    std::lock_guard<std::mutex> lock(callDBs->call_lock);

    callDBs->prefetch_bandwidth = bandwidth;
}

