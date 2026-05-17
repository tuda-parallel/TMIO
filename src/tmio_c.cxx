#include "tmio.h"
#include "tmio_c.h"
#include "tmio_helper_functions.h"

extern "C"
{
	void iotrace_summary(void)
	{
#if ENABLE_MPI_TRACE == 1
		mpi_iotrace.Summary();
#if FETCH_FTIO_FREQ
		// Get results from last summary call
		double frequency, confidence;
		retrieve_FTIO_frequency(frequency, confidence);
#ifdef PREFETCH
		prefetcher.set_io_frequency(frequency);
#endif
#if BW_LIMIT_FREQ == 1
		mpi_iotrace.set_bw_limit_freq(frequency);
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
#if defined PREFETCH && ENABLE_MPI_TRACE == 1
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
#if defined PREFETCH && ENABLE_MPI_TRACE == 1
		prefetcher.set_io_frequency(frequency);
#endif
#if BW_LIMIT_FREQ == 1
		mpi_iotrace.set_bw_limit_freq(frequency);
#endif
    }

	void retrieve_FTIO_frequency() {
#if FETCH_FTIO_FREQ == 1
		// Get results from last summary call
		double frequency, confidence;
		retrieve_FTIO_frequency(frequency, confidence);
#ifdef PREFETCH
		prefetcher.set_io_frequency(frequency);
#endif
#if BW_LIMIT_FREQ == 1
		mpi_iotrace.set_bw_limit_freq(frequency);
#endif
#endif
	}
}