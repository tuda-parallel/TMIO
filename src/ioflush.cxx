#include "ioflush.h"

IOflush::IOflush()
{
}

void IOflush::init(void)
{
}

/**
 * @brief gather \e data from all ranks into \e all_data from rank 0
 *
 * @param rank current rank
 * @param procs total number of procs
 */
void IOflush::gather(int rank, int procs)
{
	//iohf::Function_Debug(__PRETTY_FUNCTION__);
	//* create new type
	//****************
	// Calculate displacements
	ioflush_data tmp = {0,0,0};
	MPI_Aint dis[3]; // contain a memory address.
	MPI_Aint base_address;

	MPI_Get_address(&tmp, &base_address);
	// Create MPI custom data type
	MPI_Datatype MPI_ioflush_data;
	int length[3] = {1, 1, 1}; // length of each element in strcuture
	MPI_Get_address(&tmp.start_time, &dis[0]);
	MPI_Get_address(&tmp.end_time,   &dis[1]);
	MPI_Get_address(&tmp.size,       &dis[2]);

	dis[0] = MPI_Aint_diff(dis[0], base_address);
	dis[1] = MPI_Aint_diff(dis[1], base_address);
	dis[2] = MPI_Aint_diff(dis[2], base_address);

	// commit data type
	MPI_Datatype type[3] = {MPI_DOUBLE, MPI_DOUBLE, MPI_LONG_LONG};
	MPI_Type_create_struct(3, length, dis, type, &MPI_ioflush_data);
	MPI_Type_commit(&MPI_ioflush_data);

	//* gather using the new type
	//***************************
	int *displace = NULL;
	int n = data.size();

	if (rank == 0)
		all_n = (int *)malloc(sizeof(int) * procs); // cleaned later in finilize()

	// find size of individual operations performed by different ranks
	MPI_Gather(&n, 1, MPI_INT, all_n, 1, MPI_INT, 0, MPI_COMM_WORLD);

	// find displacment based on number of operations (all_n)
	if (rank == 0)
	{
		displace = (int *)malloc(sizeof(int) * procs);
		for (int i = 0; i < procs; i++)
		{
			displace[i] = (i > 0) ? displace[i - 1] + all_n[i - 1] : 0;
			sum_n += all_n[i];
		}

		// set memory to collect all data
		all_data = (ioflush_data *)malloc(sizeof(ioflush_data) * sum_n);
	}
	// finally gather data into all_data
	MPI_Gatherv(&data[0], n, MPI_ioflush_data, all_data, all_n, displace, MPI_ioflush_data, 0, MPI_COMM_WORLD);

#if IOFLUSH_VERBOSE >= 1
	if (rank == 0)
	{
		for (int i = 0; i < sum_n; i++)
		{
			std::cout << "all_data[" << i << "].start_time = " << all_data[i].start_time << std::endl;
			std::cout << "all_data[" << i << "].end_time = " << all_data[i].end_time << std::endl;
		}
	}
#endif

if (displace != NULL)
	free(displace);

MPI_Type_free(&MPI_ioflush_data);
}

/**
 * @brief starts tracing the avilable time window for the flush call. Collects the current time (start time) and the bytes transfered (size)
 *
 * @param size [in, optional] transfered bytes
 * @param start_time  [in, optional] start time of the flush operation
 */
void IOflush::start(double size, double start_time)
{
	
	// data.push_back(std::vector<ioflush_data>());
	// data.back().start_time = start_time;
	// data.back().size = size;
	
	tmp.start_time = start_time;
	tmp.size = size;
	data.push_back(tmp);
}
void IOflush::end(double end_time)
{
	data.back().end_time = end_time;
}

// TODO: make more intresing summary
void IOflush::summary(void)
{
	//iohf::Function_Debug(__PRETTY_FUNCTION__);
	int procs, rank;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &procs);

	// get all_data
	gather(rank, procs);

	// finilize
	finilize();
}

/**
 * @brief intermidate summary, can be called at anytime. Displays average avilable time. If transfered
 *  bytes where provided during tracing (see \e start ), the required bandwidth is calculated.
 *
 */
void IOflush::short_summary(void)
{
	//iohf::Function_Debug(__PRETTY_FUNCTION__);
	int procs, rank;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &procs);

#if IOFLUSH_VERBOSE >= 1
	printf("%s > rank %i > generating short summary \n", caller, rank);
#endif

	// get all_data
	gather(rank, procs);

	if (rank == 0)
	{
		double aval_time = 0;
		long long total_bytes = 0;
		bool flag = (sum_n > 0) ? true : false;

		for (int i = 0; i < sum_n; i++)
		{
			aval_time += all_data[i].end_time - all_data[i].start_time;
			if (all_data[i].size == -1)
			{
				flag = false;
				total_bytes = 0;
			}
			if (flag)
				total_bytes += all_data[i].size;
		}

		aval_time = (sum_n > 0) ? aval_time / sum_n : 0;

		std::cout << "Average avilable time is: " << aval_time << " sec\n";
		if (flag == 1)
			std::cout << "Average required bandwidth: " << total_bytes / (1'000'000 * aval_time) << " MB/sec\n";
	}

	// finilize
	finilize();
}

/**
 * @brief cleans up
 *
 */
void IOflush::finilize(void)
{

	//iohf::Function_Debug(__PRETTY_FUNCTION__);
	if (all_n != NULL)
	free(all_n);
	if (all_data != NULL)
	free(all_data);
}

/**
 * @brief clears all the content collected so far.
 *
 */
void IOflush::clear(void)
{
	//iohf::Function_Debug(__PRETTY_FUNCTION__);
	data.clear();
	sum_n = 0;
	all_n = NULL;
}

