#include "ioanalysis.h"

/*!
 * @file ioanalysis.cpp
 * @brief Contains definitions of methods for I/O analysis
 * @author Ahmad Tarraf
 * @date 21.06.2022
 */

//! ----------------------- COMMUNICATION ------------------------------

//**********************************************************************
//*                       1. Gather_Collect
//**********************************************************************
/**
 * @brief gathers data from all ranks using \e colllect strucutre
 *
 * @param all_data [out] places all data in this structure
 * @param n array contating number of io operations of every rank
 * @param rank current rank
 * @param processes all processes
 */
collect *ioanalysis::Gather_Collect(IOdata *iodata, int *n, int rank, int processes, MPI_Comm IO_WORLD, bool finilize)
{

	iohf::Function_Debug(__PRETTY_FUNCTION__);
	collect *all_data = NULL;
	int m = 9;
	static int counter = 0;

	static MPI_Datatype GATHER_collect;
	if (counter == 0)
	{
		//* create new type
		//****************

		int length[m] = {1, 1, 1, 1, 1, 1, 1, 1, 1}; // length of each element in strcuture

		MPI_Aint dis[m]; // contain a memory address.
		MPI_Aint base_address;
		MPI_Get_address(&iodata->tmp, &base_address);
		MPI_Get_address(&iodata->tmp.data, &dis[0]);
		MPI_Get_address(&iodata->tmp.t_start, &dis[1]);
		MPI_Get_address(&iodata->tmp.t_end_act, &dis[2]);
		MPI_Get_address(&iodata->tmp.t_end_req, &dis[3]);
		MPI_Get_address(&iodata->tmp.T_sum, &dis[4]);
		MPI_Get_address(&iodata->tmp.T_avr, &dis[5]);
		MPI_Get_address(&iodata->tmp.B_sum, &dis[6]);
		MPI_Get_address(&iodata->tmp.B_avr, &dis[7]);
		MPI_Get_address(&iodata->tmp.n_op, &dis[8]);

		dis[0] = MPI_Aint_diff(dis[0], base_address);
		dis[1] = MPI_Aint_diff(dis[1], base_address);
		dis[2] = MPI_Aint_diff(dis[2], base_address);
		dis[3] = MPI_Aint_diff(dis[3], base_address);
		dis[4] = MPI_Aint_diff(dis[4], base_address);
		dis[5] = MPI_Aint_diff(dis[5], base_address);
		dis[6] = MPI_Aint_diff(dis[6], base_address);
		dis[7] = MPI_Aint_diff(dis[7], base_address);
		dis[8] = MPI_Aint_diff(dis[8], base_address);

		// commit data type
		MPI_Datatype type[m];
		type[0] = MPI_LONG_LONG;
		type[1] = MPI_DOUBLE;
		type[2] = MPI_DOUBLE;
		type[3] = MPI_DOUBLE;
		type[4] = MPI_DOUBLE;
		type[5] = MPI_DOUBLE;
		type[6] = MPI_DOUBLE;
		type[7] = MPI_DOUBLE;
		type[8] = MPI_INT;

		// int flag = MPI_Type_create_struct(m, length, dis, type, &GATHER_collect);
		MPI_Type_create_struct(m, length, dis, type, &GATHER_collect);
		MPI_Type_commit(&GATHER_collect);
	}
	counter++;

	//* gather using the new type
	//***************************
	int *displacement = NULL;
	if (rank == 0)
	{
		displacement = (int *)malloc(sizeof(int) * processes);
		int sum = 0;
		for (int i = 0; i < processes; i++)
		{
			displacement[i] = (i > 0) ? displacement[i - 1] + n[i - 1] : 0;
			sum += n[i];
		}

		all_data = (collect *)malloc(sizeof(collect) * sum);
	}

	// gather collected data
	MPI_Gatherv(&iodata->phase_data[0], iodata->phase_data.size(), GATHER_collect, all_data, n, displacement, GATHER_collect, 0, IO_WORLD);

	// clean up
	if (rank == 0)
		free(displacement);

	// remove unneeded elements
	if (finilize)
		iodata->phase_data.clear();

	// clean after the code has been executed 4 time (one for each mode: async/sync write/read)
	if (counter == 4)
	{
		MPI_Type_free(&GATHER_collect);
		counter = 0;
	}

	return all_data;
}

//**********************************************************************
//*                      2. Gather_N_OP
//**********************************************************************
/**
 * @brief Gathers number of phases for all modes (async|read write|read)
 * over all ranks using a the structure \e n_struct using a single communication call
 *
 * @param n \e n_struct structure containg number of phases of the current rank
 * @param rank current rank
 * @param processes number of processes
 * @return n_struct* array of \e n_struct objects contating number of phases for all modes (async|read write|read)
 *
 * @details return value n_struct* is only for rank 0, the remaing ranks return NULL
 */
n_struct *ioanalysis::Gather_N_OP(n_struct n, int rank, int processes, MPI_Comm IO_WORLD)
{

	iohf::Function_Debug(__PRETTY_FUNCTION__);
	//* create new type
	//****************
	// Calculate displacements
	n_struct *out = NULL;
	MPI_Aint dis[4]; // contain a memory address.
	MPI_Aint base_address;

	MPI_Get_address(&n, &base_address);

	MPI_Datatype GATHER_n_op;
	int length[4] = {1, 1, 1, 1}; // length of each element in strcuture
	MPI_Get_address(&n.aw, &dis[0]);
	MPI_Get_address(&n.ar, &dis[1]);
	MPI_Get_address(&n.sw, &dis[2]);
	MPI_Get_address(&n.sr, &dis[3]);

	dis[0] = MPI_Aint_diff(dis[0], base_address);
	dis[1] = MPI_Aint_diff(dis[1], base_address);
	dis[2] = MPI_Aint_diff(dis[2], base_address);
	dis[3] = MPI_Aint_diff(dis[3], base_address);

	// commit data type
	MPI_Datatype type[4] = {MPI_INT, MPI_INT, MPI_INT, MPI_INT};
	MPI_Type_create_struct(4, length, dis, type, &GATHER_n_op);
	MPI_Type_commit(&GATHER_n_op);

	//* gather using the new type
	//***************************
	int *displace = NULL;
	int all_n[processes];
	// find displacment based on number of operations (all_n)
	if (rank == 0)
	{
		displace = (int *)malloc(sizeof(int) * processes);
		for (int i = 0; i < processes; i++)
		{
			displace[i] = (i > 0) ? displace[i - 1] + 1 : 0;
			all_n[i] = 1;
		}

		// set memory to collect all data
		out = (n_struct *)malloc(sizeof(n_struct) * processes);
	}

	// finally gather data into all_data
	int err = MPI_Gatherv(&n, 1, GATHER_n_op, out, all_n, displace, GATHER_n_op, 0, IO_WORLD);
	if (displace != NULL)
		free(displace);

	if (err != MPI_SUCCESS)
	{
		int r, s, len;
		char message[MPI_MAX_ERROR_STRING];
		MPI_Error_string(err, message, &len);
		MPI_Comm_rank(IO_WORLD, &r);
		MPI_Comm_size(IO_WORLD, &s);
		printf("%sWORLD RANK: %d \t SIZE: %d%s\n", BLUE, r, s, BLACK);
		// printf ("%.*s\n",length, message) ;
		printf("%s%.*s%s\n", RED, len, message, BLACK);
		MPI_Abort(IO_WORLD, 1);
	}

#if IOANALYSIS_VERBOSE >= 1
	if (rank == 0)
	{
		for (int i = 0; i < processes; i++)
		{
			std::cout << "out[" << i << "].aw = " << out[i].aw << std::endl;
			std::cout << "out[" << i << "].ar = " << out[i].ar << std::endl;
			std::cout << "out[" << i << "].sw = " << out[i].sw << std::endl;
			std::cout << "out[" << i << "].sr = " << out[i].sr << std::endl;
		}
	}
#endif

	MPI_Type_free(&GATHER_n_op);
	return out;
}

//**********************************************************************
//*                       3. Get_N_From_ALL_N
//**********************************************************************
/**
 * @brief extracts number of phases for async/sync write/read from \e all_n
 *
 * @param all_n [in] n_struct pointer contating all io opertaion of all ranks
 * @param iodata [in] \e iodata object contating all I/O information
 * @param rank [in] curremt rank
 * @param processes [in] number of pocesses
 * @return int* array containing number of I/O operations for a specified mode (async/sync write/read)
 * @details basically extract all elements of a a specified mode (async/sync write/read) from the structure \e all_a
 */
int *ioanalysis::Get_N_From_ALL_N(IOdata *iodata, n_struct *all_n, int rank, int processes)
{
	iohf::Function_Debug(__PRETTY_FUNCTION__);
	int *n = NULL;

	if (rank == 0)
	{
		// find number of elements:
		n = (int *)malloc(sizeof(int) * processes);
		if (iodata->a_or_s_flag)
		{
			if (iodata->w_or_r_flag)
			{
				for (int i = 0; i < processes; i++)
					n[i] = all_n[i].aw;
			}
			else
			{
				for (int i = 0; i < processes; i++)
					n[i] = all_n[i].ar;
			}
		}
		else
		{
			if (iodata->w_or_r_flag)
			{
				for (int i = 0; i < processes; i++)
					n[i] = all_n[i].sw;
			}
			else
			{
				for (int i = 0; i < processes; i++)
					n[i] = all_n[i].sr;
			}
		}
	}

	return n;
}
//**********************************************************************

//! ------------------------ ANALYSIS ----------------------------------

//**********************************************************************
//*                         1. Sum
//**********************************************************************
/**
 * @brief sums the io operations over all ranks
 *
 * @param all_n [in] n_struct pointer contating all io opertaion of all ranks
 * @param rank  [in] curremt rank
 * @param pocesses [in] number of pocesses
 * @param N_aw [out] aggregated number of asynchrnous write operations
 * @param N_ar [out] aggregated number of asynchrnous read operations
 * @param N_sw [out] aggregated number of synchrnous write operations
 * @param N_ar [out] aggregated number of synchrnous read operations
 */
void ioanalysis::Sum_N(n_struct *all_n, n_struct &n_phase, int rank, int pocesses)
{
	iohf::Function_Debug(__PRETTY_FUNCTION__);
	n_phase.aw = 0;
	n_phase.ar = 0;
	n_phase.sw = 0;
	n_phase.sr = 0;

	if (rank == 0)
	{
		for (int i = 0; i < pocesses; i++)
		{
			n_phase.aw += all_n[i].aw;
			n_phase.ar += all_n[i].ar;
			n_phase.sw += all_n[i].sw;
			n_phase.sr += all_n[i].sr;

#if IOANALYSIS_VERBOSE >= 1
			std::cout << "\t [n_phase.aw n_phase.ar n_phase.sw n_phase.sr] = [" << n_phase.aw << " " << n_phase.ar << " " << n_phase.sw << " " << n_phase.sr << "]  (i = " << i << ")\n ";
#endif
		}
	}
}
