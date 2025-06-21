#include "tmio_helper_functions.h"
#include "tmio.h"
#include "iotrace.h"
#include "ioflags.h"
#include <sstream>
#include <iostream>

void iotrace_init_helper()
{
#if ENABLE_MPI_TRACE == 1
    mpi_iotrace.Init();
#endif

#if ENABLE_LIBC_TRACE == 1
    libc_iotrace.Init();
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
    libc_iotrace.Set("finalize", true);
    libc_iotrace.Summary();
#endif

#if FUNCTION_INFO == 2
    std::vector<std::string> all_tracing = functiontracing::Function_Debug_finalize();
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

            std::cout << "\tTMIO  > " << BLUE << "\t> executing " << function_name << BLACK << std::endl;
            flag = false;
        }
        test_counter++;
    }
    else
    {
        if ((test_counter) > 0)
        {
            std::cout << "\tTMIO  > " << BLUE << "\t> executed " << RED << test_counter << BLUE << " "
                      << "int MPI_Test(ompi_request_t**, int*, MPI_Status*)\n"
                      << BLACK;
            test_counter = 0;
        }
        std::cout << "\tTMIO  > " << BLUE << "\t> executing " << function_name << BLACK << std::endl;
        flag = true;
    }
#elif FUNCTION_INFO == 2
    functiontracing::Function_Debug(function_name);
#endif
}

namespace functiontracing
{
    struct passed_func_name_buffer
    {
        std::string function_name = "";
        size_t test_counter = 0;
        std::ostringstream oss;
    };

    struct passed_func_name_buffer function_trace_buffer;

    // Helper function to handle function name and test counter logic
    void HandleFunctionNameChange(const std::string &function_name)
    {
        if (!function_trace_buffer.function_name.empty())
        {
            if (function_trace_buffer.test_counter != 0)
            {
                function_trace_buffer.oss << "\tTMIO  > " << BLUE << "\t> executed " << RED << function_trace_buffer.test_counter << BLUE << " "
                    << function_trace_buffer.function_name << "\n"
                    << BLACK;
                function_trace_buffer.test_counter = 0; // Reset counter for the next function
            }
            else
            {
                function_trace_buffer.oss << "\tTMIO  > " << BLUE << "\t> executing " << function_trace_buffer.function_name << BLACK << std::endl;
            }
        }
        function_trace_buffer.function_name = function_name;
    }

    void Function_Debug(const std::string & function_name)
    {
        if (function_trace_buffer.function_name == function_name)
        {
            function_trace_buffer.test_counter++;
            return; // Skip duplicate function calls
        }

        HandleFunctionNameChange(function_name);
    }

    std::vector<std::string> Function_Debug_finalize()
    {
        int rank = 0;
        int size = 0;
        MPI_Comm_size(MPI_COMM_WORLD, &size);
        if (size < 1)
        {
            std::cerr << "Error: MPI_COMM_WORLD size is less than 1." << std::endl;
            return {};
        }
        MPI_Comm_rank(MPI_COMM_WORLD, &rank);

        // Finalize the current function name and test counter
        HandleFunctionNameChange("finalize");

        std::string function_trace_str;
        function_trace_str += function_trace_buffer.oss.str();
        function_trace_buffer.oss.str(""); 

        size_t my_tracing_size = function_trace_str.size();
        std::vector<size_t> tracing_sizes(size, 0);
        int ret = MPI_Gather(&my_tracing_size, 1, MPI_UNSIGNED_LONG, tracing_sizes.data(), 1, MPI_UNSIGNED_LONG, 0, MPI_COMM_WORLD);

        if (ret != MPI_SUCCESS)
        {
            std::cerr << "Error: MPI_Gather failed." << std::endl;
            return {};
        }

        std::vector<std::string> all_tracing;
        // Rank 0 print the sizes gathered
        if (rank == 0)
        {
            all_tracing.emplace_back("Function calling trace for all ranks:\n");
            all_tracing.emplace_back("\n--- Rank " + std::to_string(0) + " Trace ---\n");
            all_tracing.emplace_back(function_trace_str);
            for (int i = 1; i < size; ++i)
            {
                // Add spliter and rank information for clarity
                all_tracing.emplace_back("\n--- Rank " + std::to_string(i) + " Trace ---\n");

                std::vector<char> buffer(tracing_sizes[i]);
                int ret = MPI_Recv(buffer.data(), tracing_sizes[i], MPI_CHAR, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                if (ret != MPI_SUCCESS)
                {
                    std::cerr << "Error: MPI_Recv failed for rank " << i << "." << std::endl;
                    return {};
                }
                all_tracing.emplace_back(buffer.data(), tracing_sizes[i]);
            }
            all_tracing.emplace_back("Total number of ranks: " + std::to_string(size) + "\n");
        }
        else
        {
            // Non-root ranks send their tracing data
            int ret = MPI_Send(function_trace_str.data(), my_tracing_size, MPI_CHAR, 0, 0, MPI_COMM_WORLD);
            if (ret != MPI_SUCCESS)
            {
                std::cerr << "Error: MPI_Send failed for rank " << rank << "." << std::endl;
                return {};
            }
        }
        return all_tracing;
    }
} // namespace functiontracing