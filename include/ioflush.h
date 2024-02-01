//#include "hfunctions.h"

#include <iostream>
#include <mpi.h>
#include <vector>
#include "ioflags.h"
// #ifndef HFUNCTIONS_INCLUDE
// #define HFUNCTIONS_INCLUDE
// #include "hfunctions.h"
// #endif

/**
 *  IO flush class
 * @file   ioflush.h
 * @brief  Contains definitions of methods from the \e ioflush.h class.
 * @author Ahmad Tarraf
 * @date   05.08.2021
 */

#ifndef IOFLUSH_VERBOSE
#define IOFLUSH_VERBOSE 0
#endif

/**
* @class IOflush
* @brief Construct a new ioflush object. Used to trace flushing calls (e.g. Burst buffers)

* @details
*-
*/
class IOflush
{

public:
	IOflush();
	void start(double size = -1, double start_time = MPI_Wtime());
	void end(double end_time = MPI_Wtime());
	void summary(void);
	void short_summary(void);
	void clear(void);

private:
	struct ioflush_data
	{
		double start_time;
		double end_time;
		long long size;
	};

	ioflush_data tmp;
	int *all_n = NULL;
	int sum_n = 0;
	std::vector<ioflush_data> data; // all collected data
	ioflush_data *all_data = NULL;	// relevant for rank 0 only
	char caller[10] = "\tIOflush ";

	void init(void);
	void gather(int, int);
	void finilize(void);
};