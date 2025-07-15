#include "tmio_helper_functions.h"
#include "tmio.h"
#include "iotrace.h"
#include "ioflags.h"
#include <sstream>
#include <iostream>
#include <mutex>
#include <thread>
#include <unistd.h> // Required for getpid()

namespace functiontracing
{
    /**
     * @class FunctionTracer
     * @brief A thread-safe class to trace and aggregate function call information.
     *
     * This class encapsulates the buffer, counters, and the mutex required to
     * safely record function calls from multiple threads (e.g., main thread and
     * MPI progress threads).
     */
    class FunctionTracer
    {
    public:
        // Deleted copy and move constructors to prevent accidental copies of the singleton.
        FunctionTracer(const FunctionTracer &) = delete;
        FunctionTracer &operator=(const FunctionTracer &) = delete;

        /**
         * @brief Records a function call. Aggregates consecutive identical calls.
         * @param function_name The name of the function that was called.
         */
        void add_trace(const std::string &function_name)
        {
            // Lock the mutex for the duration of this function call.
            // The lock is automatically released when 'lock' goes out of scope.
            std::lock_guard<std::mutex> lock(mtx_);

            if (current_function_name_ == function_name)
            {
                test_counter_++;
                return; // Increment counter for consecutive calls and exit.
            }

            // If the function name changed, process the previous function's data.
            flush_current_trace_to_stream();
            current_function_name_ = function_name;
        }

        /**
         * @brief Gathers trace data from all MPI ranks and returns it for printing.
         * @return A vector of strings containing the formatted trace for all ranks.
         */
        std::vector<std::string> finalize()
        {
            // Lock to ensure no other thread is modifying data during finalization.
            std::lock_guard<std::mutex> lock(mtx_);

            // Process the very last function that was being traced.
            flush_current_trace_to_stream();
            current_function_name_ = "finalize"; // Mark as finalized.
            flush_current_trace_to_stream();

            // The rest of the MPI communication logic remains the same.
            int rank = 0;
            int size = 0;
            MPI_Comm_size(MPI_COMM_WORLD, &size);
            if (size < 1)
            {
                return {};
            }
            MPI_Comm_rank(MPI_COMM_WORLD, &rank);

            std::string function_trace_str = oss_.str();
            oss_.str(""); // Clear the stream buffer.

            size_t my_tracing_size = function_trace_str.size();
            std::vector<size_t> tracing_sizes(size, 0);
            MPI_Gather(&my_tracing_size, 1, MPI_UNSIGNED_LONG, tracing_sizes.data(), 1, MPI_UNSIGNED_LONG, 0, MPI_COMM_WORLD);

            std::vector<std::string> all_tracing;
            if (rank == 0)
            {
                all_tracing.emplace_back("Function calling trace for all ranks:\n");
                all_tracing.emplace_back("\n--- Rank " + std::to_string(0) + " Trace ---\n");
                all_tracing.emplace_back(function_trace_str);
                for (int i = 1; i < size; ++i)
                {
                    all_tracing.emplace_back("\n--- Rank " + std::to_string(i) + " Trace ---\n");
                    std::vector<char> buffer(tracing_sizes[i]);
                    MPI_Recv(buffer.data(), tracing_sizes[i], MPI_CHAR, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                    all_tracing.emplace_back(buffer.data(), tracing_sizes[i]);
                }
                all_tracing.emplace_back("Total number of ranks: " + std::to_string(size) + "\n");
            }
            else
            {
                MPI_Send(function_trace_str.data(), my_tracing_size, MPI_CHAR, 0, 0, MPI_COMM_WORLD);
            }
            return all_tracing;
        }

    private:
        // Private constructor for the singleton pattern.
        FunctionTracer() = default;

        // This is the helper that processes the buffered function name.
        // It's private because it should only be called internally when the mutex is held.
        void flush_current_trace_to_stream()
        {
            if (current_function_name_.empty())
            {
                return;
            }

            // Get the unique Process ID (PID) and Thread ID (TID)
            pid_t pid = getpid();
            std::thread::id tid = std::this_thread::get_id();

            if (test_counter_ > 0)
            {
                oss_ << "\tTMIO  > " << BLUE << "[pid " << pid << " | tid " << tid
                     << "] > executed " << RED << test_counter_ + 1 << BLUE << " times: "
                     << current_function_name_ << "\n"
                     << BLACK;
            }
            else
            {
                oss_ << "\tTMIO  > " << BLUE << "[pid " << pid << " | tid " << tid
                     << "] > executing " << current_function_name_ << BLACK << std::endl;
            }
            test_counter_ = 0; // Reset counter
        }

        // The singleton accessor is a friend so it can call the private constructor.
        friend FunctionTracer &get_tracer();

        // Data members are now private and protected by the mutex.
        mutable std::mutex mtx_; // `mutable` allows locking in const methods if needed.
        std::string current_function_name_ = "";
        size_t test_counter_ = 0;
        std::ostringstream oss_;
    };

    /**
     * @brief Provides access to the Mayer's singleton instance of FunctionTracer.
     */
    FunctionTracer &get_tracer()
    {

#if FUNCTION_INFO == 3
        static thread_local FunctionTracer instance;
#else
        static FunctionTracer instance;
#endif
        return instance;
    }

} // namespace functiontracing

void iotrace_init_helper()
{
#if ENABLE_MPI_TRACE == 1
    mpi_iotrace.Init();
#endif

#if ENABLE_LIBC_TRACE == 1
#if IO_BEFORE_MAIN == 0
    get_libc_iotrace().Enable_Tracing();
#endif
    get_libc_iotrace().Init();
#endif
}

void iotrace_finalize_helper()
{
    // Flush the cout buffer to ensure all function traces are printed before finalizing
    std::cout.flush();

#if ENABLE_MPI_TRACE == 1
    mpi_iotrace.Set("finalize", true);
    mpi_iotrace.Summary();
#endif

#if ENABLE_LIBC_TRACE == 1
    get_libc_iotrace().Set("finalize", true);
    get_libc_iotrace().Summary();
#endif

#if FUNCTION_INFO == 2 || FUNCTION_INFO == 3
    std::vector<std::string> all_tracing = functiontracing::get_tracer().finalize();
    for (const auto &trace : all_tracing)
    {
        std::cout << trace;
    }
#endif
}

void Function_Debug(std::string function_name, int test_flag)
{

#if FUNCTION_INFO == 1
    std::string s_test = "MPI_Test";
    static int test_counter = 0;
    static bool flag = true;
    if (function_name.find(s_test) != std::string::npos)
    {
        if (flag)
        {

            std::cout << "\tTMIO  > " << BLUE << "\t[thread "
                      << std::this_thread::get_id() << "] > executing " << function_name << BLACK << std::endl;
            flag = false;
        }
        test_counter++;
    }
    else
    {
        if ((test_counter) > 0)
        {
            std::cout << "\tTMIO  > " << BLUE << "\t[thread "
                      << std::this_thread::get_id() << "] > executed " << RED << test_counter << BLUE << " "
                      << "int MPI_Test(ompi_request_t**, int*, MPI_Status*)\n"
                      << BLACK;
            test_counter = 0;
        }
        std::cout << "\tTMIO  > " << BLUE << "\t[thread "
                  << std::this_thread::get_id() << "] > executing " << function_name << BLACK << std::endl;
        flag = true;
    }
#elif FUNCTION_INFO == 2 || FUNCTION_INFO == 3
    // The public-facing function now calls the method on the singleton instance.
    functiontracing::get_tracer().add_trace(function_name);
#endif
}
