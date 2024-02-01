#include "iodata.h"
// #include <complex>

/*!
 * @file ioanalysis.h
 * @brief Contains definitions of methods from the \e Ioanalysis namespace.
 * @details used for performaning analysis on the Input/Output collected
 * @author Ahmad Tarraf
 * @date 05.08.2021
 */

struct n_struct
{
	int aw, ar, sw, sr;
};

namespace ioanalysis
{

	collect *Gather_Collect(IOdata *, int *, int, int, MPI_Comm, bool finilize);
	n_struct *Gather_N_OP(n_struct, int, int, MPI_Comm);
	void Sum_N(n_struct *, n_struct &, int, int);
	int *Get_N_From_ALL_N(IOdata *, n_struct *, int, int);

}