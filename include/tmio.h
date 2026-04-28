#ifndef TMIO_H
#define TMIO_H

#include <stdio.h>
#include <mpi.h>
#include "iotrace.h"
#include "prefetch.h"
#include "tmio_helper_functions.h"

/**
 *  IO trace class
 * @file    iotrace.cpp
 * @author  Ahmad Tarraf
 * @date   05.08.2021
 */

#if ENABLE_MPI_TRACE == 1
#include "interfaces/mpi_interface.h"
#endif // ENABLE_MPI_TRACE

#if ENABLE_LIBC_TRACE == 1
#include "interfaces/libc_interface.h"
#endif // ENABLE_LIBC_TRACE

#if ENABLE_IOURING_TRACE == 1
#include "interfaces/iouring_interface.h"
#endif // ENABLE_IOURING_TRACE


#include "tmio_c.h" // Include the C-compatible header

namespace tmio {
    inline void iotrace_summary() {
        ::iotrace_summary(); // Call the C-compatible function
    }

    inline void retrieve_frequencies() {
        ::retrieve_FTIO_frequency();
    }

    inline void init_prefetcher(double io_frequency, size_t max_cache_bytes, size_t max_file_bytes) {
        ::init_prefetcher(io_frequency, max_cache_bytes, max_file_bytes);
    }

    inline int Write_Checkpoint(MPI_File fh, MPI_Offset offset, const void *buf, int count, MPI_Datatype datatype, MPI_Request *request, double durations_sec) {
        return ::Write_Checkpoint(fh, offset, buf, count, datatype, request, durations_sec);
    }

    inline void set_io_freqency(double frequency) {
        ::set_io_freqency(frequency);
    }
}

#endif // TMIO_H
