#ifndef TMIO_HELPER_FUNCTIONS_H
#define TMIO_HELPER_FUNCTIONS_H

#include <dlfcn.h>
#include <stdlib.h>
#include <atomic>
#include <cstdint> // For uint64_t
#include <iostream>
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
#define TMIO_DECL(__func) __func

/**
 * @brief Map the desired function call to a pointer called __real_NAME at run
 * time.  Note that we fall back to looking for the same symbol with a P
 * prefix to handle MPI bindings that call directly to the PMPI layer.
 */
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

//! debug
void Function_Debug(std::string function_name, int flag = 0);

void iotrace_init_helper();
void iotrace_finalize_helper();

namespace functiontracing
{
    class FunctionTracer;
    FunctionTracer &get_tracer();
}

// std::atomic<uint64_t> request_id_counter(0);
// uint64_t generate_unique_id()
// {
//     return request_id_counter.fetch_add(1, std::memory_order_relaxed);
// }
#endif // TMIO_HELPER_FUNCTIONS_H