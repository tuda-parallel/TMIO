#ifndef TMIO_H
#define TMIO_H

#include <stdio.h>
#include <mpi.h>
#include "iotrace.h"
#include "tmio_helper_functions.h"

#if ENABLE_MPI_TRACE == 1
#include "interfaces/mpi_interface.h"
#endif // ENABLE_MPI_TRACE

#if ENABLE_LIBC_TRACE == 1
#include "interfaces/libc_interface.h"
#endif // ENABLE_LIBC_TRACE

/**
 *  IO trace class
 * @file    iotrace.cpp
 * @author  Ahmad Tarraf
 * @date   05.08.2021
 */

#endif // TMIO_H
