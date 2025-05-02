#include "tmio.h"
#include "tmio_c.h"

extern "C"
{
	void iotrace_summary(void)
	{
#if ENABLE_MPI_TRACE == 1
		mpi_iotrace.Summary();
#endif

#if ENABLE_LIBC_TRACE == 1
		libc_iotrace.Summary();
#endif
	}
}