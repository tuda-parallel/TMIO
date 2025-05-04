#include "tmio_helper_functions.h"
#include "tmio.h"
#include "iotrace.h"
#include "ioflags.h"

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
    
#if ENABLE_MPI_TRACE == 1
	mpi_iotrace.Set("finalize", true);
	mpi_iotrace.Summary();
#endif

#if ENABLE_LIBC_TRACE == 1
	libc_iotrace.Set("finalize", true);
	libc_iotrace.Summary();
#endif
}
	

void Function_Debug(std::string function_name, int test_flag)
{
#if FUNCTION_INFO == 1

    std::string s_test = "MPI_Test";
    static int test_counter = 0;
    static bool flag = true;
    static int flag_2 = 0;
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
    fflush(stdout);
#endif
}