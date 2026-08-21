#ifndef TMIO_HELPER_FUNCTIONS_H
#define TMIO_HELPER_FUNCTIONS_H

#include <atomic>
#include <cstdint> // For uint64_t
#include <dlfcn.h>
#include <iostream>
#include <stdlib.h>
#include <string>
#include <vector>

/**
 * @brief The TMIO_FORWARD_DECL macro is used to provide forward
 * declarations for wrapped funcions, regardless of whether TMIO is used with
 * statically or dynamically linked executables.
 */
#define TMIO_FORWARD_DECL(__func, __ret, __args) \
    __ret(*__real_##__func) __args = nullptr

/**
 * @brief The TMIO_DECL macro provides the appropriate wrapper function names,
 * depending on whether the TMIO library is statically or dynamically linked.
 */
#ifdef TMIO_STATIC_WRAP
// For static linking with --wrap
#define TMIO_DECL(__func) __wrap_##__func
#else
// For dynamic linking with LD_PRELOAD
#define TMIO_DECL(__func) __func
#endif
/**
 * @brief Map the desired function call to a pointer called __real_NAME at run
 * time.  Note that we fall back to looking for the same symbol with a P
 * prefix to handle MPI bindings that call directly to the PMPI layer.
 */
#ifdef TMIO_STATIC_WRAP
// In static mode, this does nothing. The linker has already connected
// __real_write for us.
#define MAP_OR_FAIL(__func)
#else
// In dynamic mode, it does the dlsym lookup.
#define MAP_OR_FAIL(__func)                                              \
    if (!(__real_##__func))                                              \
    {                                                                    \
        __real_##__func = reinterpret_cast<decltype(__real_##__func)>(   \
            dlsym(RTLD_NEXT, #__func));                                  \
        if (!(__real_##__func))                                          \
        {                                                                \
            fprintf(stderr, "TMIO failed to map symbol: %s\n", #__func); \
            fflush(stderr);                                              \
            exit(1);                                                     \
        }                                                                \
    }
#endif
namespace tmio
{
    // Set for the duration of an MPI-IO data call (MPI_File_{read,write,iread,iwrite}*),
    // so the POSIX interceptor can tag nested calls (e.g. ROMIO's ufs ADIO driver
    // calling pwrite/pread internally) instead of mistaking them for direct
    // application POSIX I/O. Tracing stays on either way; only bandwidth-limit
    // pacing decisions read this flag (see BW_LIMIT_LAYER).
    // NOTE: only visible on the calling thread. MPI-IO actually transferred on a
    // modified-MPICH-spawned worker thread (true async execution) is not tagged.
    inline thread_local bool in_mpi_io_call = false;

    class MPIIOCallGuard
    {
    public:
        MPIIOCallGuard() { in_mpi_io_call = true; }
        ~MPIIOCallGuard() { in_mpi_io_call = false; }
        MPIIOCallGuard(const MPIIOCallGuard &) = delete;
        MPIIOCallGuard &operator=(const MPIIOCallGuard &) = delete;
    };
}

//! debug
void Function_Debug(std::string function_name, int flag = 0);

void iotrace_init_helper();
void iotrace_finalize_helper();

namespace functiontracing
{
    class FunctionTracer;
    FunctionTracer &get_tracer();
}

#if FETCH_FTIO_FREQ == 1
void retrieve_FTIO_frequency(double& frequency, double& confidence);
#endif

// std::atomic<uint64_t> request_id_counter(0);
// uint64_t generate_unique_id()
// {
//     return request_id_counter.fetch_add(1, std::memory_order_relaxed);
// }
#endif // TMIO_HELPER_FUNCTIONS_H