#include "prefetch.h"
#include <thread>
#include <stdio.h>
#include <algorithm>
#include <cstring>
#include <cassert>
#include <iotrace.h>

Prefetcher::Prefetcher() : inititalized(false) {

}

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
    requests_in_transit = std::make_shared<RequestsInTransit>();

    std::thread([&] {Prefetcher::prefetching_routine(requests_in_transit, callDBs, mpi_iotrace);}).detach();
    inititalized = true;
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
    if (take_prefetched_by_call_signature(cs, mpi_request, prefetch_buffer)) {
        AsyncBuffers buffers = {std::move(prefetch_buffer), target_buffer};

        async_requests.emplace(mpi_request, buffers);

        err = MPI_SUCCESS;
    } else {
        Prefetcher::Log<VerbosityLevel::DETAILED_LOG>(
            "Prefetch miss. Fetching read manually");
        traces->Read_Async_Start(cs.count, cs.type, mpi_request, cs.offset);
        err = PMPI_File_iread_at(cs.fh, cs.offset, target_buffer, cs.count, cs.type, mpi_request);
    }

    request_signature[mpi_request] = cs;
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

    auto it = std::find(async_requests.begin(), async_requests.end(), request);

    if(it != async_requests.end()) {
        std::memcpy(it->second.target_buffer,
            it->second.prefetch_buffer.data(), it->second.prefetch_buffer.size());

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

    auto it = std::find(async_requests.begin(), async_requests.end(), request);

    if (it != async_requests.end()) {
        std::memcpy(it->second.target_buffer,
            it->second.prefetch_buffer.data(), it->second.prefetch_buffer.size());

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
 * @brief Get results from prefetch cache, block if necessary
 * @param cs Signature of the retrieved call
 * @param target_buffer Target buffer for read data
 * @param mpi_status [out] Returns status of read operation
 * @return MPI error value
 */
int Prefetcher::retrieve_read_snyc(CallSignature& cs, void* target_buffer, MPI_Status* status)
{
    MPI_Request mpi_request;
    std::vector<std::byte> prefetch_buffer;

    double time_req = MPI_Wtime();

    int err;
    if (take_prefetched_by_call_signature(cs, &mpi_request, prefetch_buffer)) {
        err = MPI_Wait(&mpi_request, status);
        if(err == MPI_SUCCESS) {
            std::memcpy(target_buffer, prefetch_buffer.data(), prefetch_buffer.size());
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
 * @brief Find prefetch in cache by call signature
 * @param cs Signature of the retrieved call
 * @param mpi_request [out] MPI_Request associated with prefetch call
 * @param prefetch_buffer [out] Returns buffer associated with prefetch
 * @return true if prefetch with same call_signature was found
 */
bool Prefetcher::take_prefetched_by_call_signature(CallSignature& cs, MPI_Request* mpi_request, std::vector<std::byte>& prefetch_buffer) {

    //TODO better cs finding
    const std::lock_guard<std::mutex> lock(requests_in_transit->request_lock);
    
    auto it = std::find_if(requests_in_transit->requests.begin(), requests_in_transit->requests.end(), [&](CallSignature& ref_cs){return cs == ref_cs;});
    
    bool found = it != requests_in_transit->requests.end();

    if(found) {
        prefetch_buffer = it->buffer;
        *mpi_request = it->request;

        requests_in_transit->requests.erase(it);
    }

    return found;
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

    if(err != MPI_SUCCESS || type_size * cs.count > max_file_size_bytes) {
        return;
    }

    {
        std::lock_guard<std::mutex> lock(callDBs->call_lock);

        auto it = callDBs->prefetch_infos.find(cs.fh);

        if(it == callDBs->prefetch_infos.end()) {
            PrefetchInfo prefetch_info = {time_req, cs, false, {}};

            callDBs->prefetch_infos.emplace(cs.fh, prefetch_info);
        } else {
            it->second.last_call_time = time_req;
            it->second.prefetched = false;
            it->second.next_prefetch_time = {};
            it->second.cs = cs;
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
void Prefetcher::prefetching_routine(std::shared_ptr<RequestsInTransit> rit, std::shared_ptr<CallDB> cdb, IOtraceMPI* traces) 
{
    //TODO Check for max chache size
    std::optional<MPI_File> next_prefetch;
    while(true) {
        if (next_prefetch){ // If prefetch is scheduled, begin prefetch
            std::lock_guard<std::mutex> lock(cdb->call_lock);
            auto it = cdb->prefetch_infos.find(*next_prefetch);
            if(it != cdb->prefetch_infos.end()) {
                prefetch_transaction(it->second.cs, rit, traces);
            }
        } else { // Wait on new prefetch info to arrive
            std::unique_lock<std::mutex> lock(cdb->event_mutex);
            if(!cdb->event_ready) {
                cdb->prefetch_event.wait(lock, [&]{return cdb->event_ready;});
            }
        }

        double next_prefetch_time = 0.0;
        next_prefetch = determine_next_prefetch(cdb, next_prefetch_time);

        if(next_prefetch) {
            double now = MPI_Wtime();
            double sleep_s = next_prefetch_time - now;
            if(sleep_s > 0.) {
                std::this_thread::sleep_for(std::chrono::duration<double>(sleep_s));
            }
        }
    }
}

/**
 * @brief Starts prefetching of transaction
 * @param cs Signature of the prefetched call
 * @param rit MPI_Request associated with prefetch call
 */
void Prefetcher::prefetch_transaction(CallSignature& cs, std::shared_ptr<RequestsInTransit>& rit, IOtraceMPI* traces)
{
    // TODO BW Limit
    int type_size = 0;
    auto err = MPI_Type_size(cs.type, &type_size);

    // TODO error handling
    if(err != MPI_SUCCESS || type_size <= 0) return;
    
    std::vector<std::byte> prefetch_buffer(type_size * cs.count);
    MPI_Request mpi_request;

    if constexpr (BW_LIMIT_GRANULARITY > 1)
		traces->apply_file_specific_bw(false, cs.fh, cs.count, cs.type);
    traces->Read_Async_Start(cs.count, cs.type, &mpi_request, cs.offset);
    err = PMPI_File_iread(cs.fh, prefetch_buffer.data(), cs.count, cs.type, &mpi_request);

    // TODO error handling
    if(err != MPI_SUCCESS || type_size <= 0) return;

    const std::lock_guard<std::mutex> lock(rit->request_lock);

    rit->requests.emplace_back(mpi_request, std::move(prefetch_buffer), cs);
}

/**
 * @brief Determines next prefetched MPI_File from callDB
 * @param cdb 
 * @param next_prefetch_time timestamp of the next prefetch call
 */
std::optional<MPI_File> Prefetcher::determine_next_prefetch(std::shared_ptr<CallDB>& cdb, double &next_prefetch_time) {
    std::lock_guard<std::mutex> lock(cdb->call_lock);

    double io_period = 1/(cdb->io_frequency);

    std::optional<decltype(cdb->prefetch_infos.begin())> min_element;

    for(auto it = cdb->prefetch_infos.begin(); it != cdb->prefetch_infos.end(); it++) {
        if(it->second.prefetched) continue;

        if(!it->second.next_prefetch_time) {
            int type_size;

            int err = MPI_Type_size(it->second.cs.type, &type_size);

            if(err == MPI_SUCCESS) {
                double transfer_time = (it->second.cs.count * type_size) / cdb->prefetch_bandwidth;

                double next_call_time = io_period + it->second.last_call_time;

                it->second.next_prefetch_time.emplace(next_call_time - transfer_time);
            }
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
    } else {
        std::lock_guard<std::mutex> ev_lock(cdb->event_mutex);
        cdb->event_ready = false;

        return {};
    }
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

