#include "tmio.h"
#include "tmio_c.h"
#include "tmio_helper_functions.h"

extern "C"
{
	void iotrace_summary(void)
	{
#if ENABLE_MPI_TRACE == 1
		mpi_iotrace.Summary();
#if BW_LIMIT_FTIO == 1 || PREFETCH_FREQ == 1
		// Get results from last summary call
		double freq = retrieve_FTIO_frequency();
#if PREFETCH_FREQ == 1
		prefetcher.set_io_frequency(freq);
#endif
#if BW_LIMIT_FTIO == 1
		mpi_iotrace.set_bw_limit_freq(freq);
#endif
#endif
#endif

#if ENABLE_LIBC_TRACE == 1
		get_libc_iotrace().Summary();
#endif

#if ENABLE_IOURING_TRACE == 1
		get_iouring_iotrace().Summary();
#endif
	}
	void init_prefetcher(double io_frequency, size_t max_cache_bytes, size_t max_file_bytes) {
#if PREFETCH == 1 & ENABLE_MPI_TRACE == 1
		prefetcher.init(io_frequency, max_cache_bytes, max_file_bytes);
#endif
	}

	int Write_Checkpoint(MPI_File fh, MPI_Offset offset, const void *buf, int count, MPI_Datatype datatype, MPI_Request *request, double duration_sec) {
		Function_Debug(__PRETTY_FUNCTION__);
#if ENABLE_MPI_TRACE == 1
#if BW_LIMIT_GRANULARITY > 1
		mpi_iotrace.apply_checkpoint_limit(count, datatype, duration_sec);
#endif
		mpi_iotrace.Write_Async_Start(count, datatype, request, fh, offset);
#endif
		return PMPI_File_iwrite_at(fh, offset, buf, count, datatype, request);
	}

    void set_io_freqency(double frequency) {
#if PREFETCH == 1
		prefetcher.set_io_frequency(frequency);
#endif
#if BW_LIMIT_FTIO == 1
		mpi_iotrace.set_bw_limit_freq(frequency);
#endif
    }
}