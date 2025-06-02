#include "hfunctions.h"

/* Here are the helper functions which perform the actual calculations
 */

namespace iohf
{

	// Compute the Median
	double Median(double tmp[], int n)
	{
		double median = -1;
		if (n != 0)
		{
#if SKIP_LAST_WRITE > 0 || SKIP_FIRST_READ > 0 || SKIP_LAST_READ > 0 || SKIP_FIRST_WRITE > 0

			double arr[n];
			int N = 0;
			for (const int i : tmp)
				if (!isnan(i))
					arr[N++] = i;

			std::sort(arr, arr + N);

#else
			double arr[n];
			std::copy(tmp, tmp + n, arr);
			std::sort(arr, arr + n);
#endif

			if (n % 2 == 0) // even
				//{
				median = (arr[n / 2 - 1] + arr[n / 2]) / 2;
			else
				//{
				median = arr[n / 2];
		}

		return median;
	}

	double Arithmetic_Mean(double tmp[], int n)
	{

		double mean = -1;
		if (n != 0)
		{
			int N = 0;
			mean = 0;
			for (int i = 0; i < n; i++)
			{
				if (!isnan(tmp[i]))
				{
					mean += tmp[i];
					N++;
				}
			}

			mean = mean / N;
		}

		return mean;
	}

	// Compute the Harmonic mean
	double Harmonic_Mean(double tmp[], int n)
	{

		double hmean = -1;
		if (n != 0)
		{
			hmean = 0;
			int N = 0;
			for (int i = 0; i < n; i++)
			{
				if (!isnan(tmp[i]))
				{
					hmean += 1 / tmp[i];
					N++;
					// std::cout << 1 << "/" << tmp[i] << " + ";
				}
			}
			hmean = N / hmean;
			// std::cout << "\nhmean is = " << N << "/" << hmean << "\n";
		}
		return hmean;
	}

	// Compute the Harmonic mean
	double Harmonic_Mean_Non_Zero(double tmp[], int n)
	{

		double hmean = 0;
		if (n != 0)
		{
			hmean = 0;
			int N = 0;
			for (int i = 0; i < n; i++)
			{
				if (!isnan(tmp[i]) && tmp[i] != 0)
				{
					hmean += 1 / tmp[i];
					N++;
				}
			}
			hmean = N / hmean;
		}
		return hmean;
	}

	double Weighted_Harmonic_Mean(long long *b, double *tmp, int n)
	{

		double wmean = -1;
		if (n != 0)
		{
			wmean = 0;
			long long sum_b = 0;
			for (int i = 0; i < n; i++)
			{
				if (!isnan(tmp[i]))
				{
					wmean += b[i] / tmp[i];
					sum_b += b[i];
					// std::cout << b[i] << "/" << tmp[i] << " + ";
					// std::cout << "\nwmean is = " << wmean << "   sum_b = " << sum_b <<"\n";
				}
			}
			wmean = sum_b / wmean;
			// std::cout << "\nwmean is = " << sum_b << "/" << wmean << "\n";
		}
		return wmean;
	}

	// Compute the Minimum
	double Min(double tmp[], int n, int *ptr)
	{
		int index = -1;
		double min = -1;
		if (n != 0)
		{
			min = std::numeric_limits<double>::max();
			for (int i = 0; i < n; i++)
			{
				if (!isnan(tmp[i]) && min > tmp[i])
				{
					min = tmp[i];
					index = i;
				}
			}
		}
		if (ptr != NULL)
			*ptr = index;
		return min;
	}

	// Compute the Maximum
	template <class T>
	T Max(T *tmp, int n)
	{
		T max = 0;

		if (tmp != NULL)
		{
			for (int i = 0; i < n; i++)
			{
				if (!isnan(tmp[i]) && max < tmp[i])
					max = tmp[i];
			}
		}
		return max;
	}
	template double Max<double>(double *tmp, int n);
	template long Max<long>(long *tmp, int n);
	template int Max<int>(int *tmp, int n);

	//! compute the varriance of a array
	double Variance(double tmp[], int n, double mean)
	{
		double var = -1;
		if (n != 0)
		{
			var = 0;
			if (isnan(mean))
				mean = Arithmetic_Mean(tmp, n);
			for (int i = 0; i < n; i++)
			{
				if (!isnan(tmp[i]))
					var += pow(tmp[i] - mean, 2);
			}
			var = var / n;
		}

		return var;
	}

	double Standard_Deviation(double tmp[], int n, double mean)
	{
		return sqrt(Variance(tmp, n, mean));
	}

	template <class T>
	T Sum(T *a, int n)
	{
		T sum = 0;

		// computes sum
		if (a != NULL)
		{
			for (int i = 0; i < n; i++)
				sum += a[i];
		}

		return sum;
	}
	// explicit iniciations so that the linker knows about the template
	template int Sum<int>(int *a, int n);
	template double Sum<double>(double *a, int n);
	template long Sum<long>(long *a, int n);
	template long long Sum<long long>(long long *a, int n);

	template <class T>
	void Disp(T tmp, int n, std::string name, int mode)
	{

		if (mode == 0)
			for (int i = 0; i < n; i++)
				std::cout << i << ". " << name << tmp[i] << std::endl;
		else
		{
			std::cout << name << " = ";
			for (int i = 0; i < n; i++)
				std::cout << tmp[i] << " ";
			std::cout << std::endl;
		}
	}
	template void Disp<double *>(double *, int, std::string, int mode);
	template void Disp<int *>(int *, int, std::string, int mode);
	template void Disp<std::vector<int>>(std::vector<int>, int, std::string, int mode);
	template void Disp<std::vector<double>>(std::vector<double>, int, std::string, int mode);

	template <class T>
	void Phase_Info(T n, T &max, T &N, MPI_Datatype type)
	{
		// find Max number of read and write phases over all processes
		// MPI_Reduce(&n, &max, 1, type, MPI_MAX, 0, MPI_COMM_WORLD);
		// MPI_Allreduce(&n, &max, 1, type, MPI_MAX, MPI_COMM_WORLD);
		Phase_Max(n, max, type);

		// Sum to get total number of read and write phases
		// recive buffer size is only relevant for rank 0. Else use Allreduce
		// MPI_Reduce(&n, &N, 1, type, MPI_SUM, 0, MPI_COMM_WORLD);
		MPI_Allreduce(&n, &N, 1, type, MPI_SUM, MPI_COMM_WORLD);
	}
	// explicit iniciations so that the linker knows about the template
	template void Phase_Info<int>(int, int &, int &, MPI_Datatype);
	template void Phase_Info<long>(long, long &, long &, MPI_Datatype);
	template void Phase_Info<long long>(long long, long long &, long long &N, MPI_Datatype);

	template <class T>
	void Phase_Max(T n, T &max, MPI_Datatype type)
	{
		MPI_Allreduce(&n, &max, 1, type, MPI_MAX, MPI_COMM_WORLD);
	}
	// explicit iniciations so that the linker knows about the template
	template void Phase_Max<int>(int, int &, MPI_Datatype);
	template void Phase_Max<long>(long, long &, MPI_Datatype);
	template void Phase_Max<long long>(long long, long long &, MPI_Datatype);

	void Set_Unit(double d, std::string &unit, double &divisor)
	{

		if (d > 1e+9)
		{
			unit = "GB";
			divisor = 1e-9;
		}
		else if (d >= 1e+6)
		{
			unit = "MB";
			divisor = 1e-6;
		}
		else if (d >= 1e+3)
		{
			unit = "KB";
			divisor = 1e-3;
		}
		else
		{
			unit = "B";
			divisor = 1;
		}
	}
	template <class T>
	void Gather_Summary(int n, int processes, int rank, T *buff_all_values, std::vector<T> my_vector, int *arr_all_n, MPI_Comm IO_WORLD, MPI_Datatype type)
	{

		// buffer to store all values
		int displacment[processes];

		if (rank == 0)
		{
			displacment[0] = 0;
			for (int i = 1; i < processes; i++)
			displacment[i] = displacment[i - 1] + arr_all_n[i - 1];
		}
		
		#if HDEBUG >= 1
		if (rank == 0)
		{
		std::cout << "Rank " << rank << " calling MPI_Gatherv with n=" << n << std::endl;
				std::cout << "arr_all_n: ";
				for (int i = 0; i < processes; i++) std::cout << arr_all_n[i] << " ";
				std::cout << "\ndisplacement: ";
				for (int i = 0; i < processes; i++) std::cout << displacment[i] << " ";
				std::cout << std::endl;
			}
		else{
			std::cout << "Rank " << rank << " calling MPI_Gatherv with n=" << n << std::endl;
		}
		#endif

		MPI_Gatherv(my_vector.data(), n, type, buff_all_values, arr_all_n, displacment, type, 0, IO_WORLD);
	}
	template void Gather_Summary<int>(int, int, int, int *, std::vector<int>, int *, MPI_Comm, MPI_Datatype);
	template void Gather_Summary<long long>(int, int, int, long long *, std::vector<long long>, int *, MPI_Comm, MPI_Datatype);
	template void Gather_Summary<double>(int, int, int, double *, std::vector<double>, int *, MPI_Comm, MPI_Datatype);

	int Get_Io_Ranks(int procs, int *arr_all_n)
	{
		// see how many process participitated in I/O
		int counter = 0;

		if (arr_all_n != NULL)
		{
			for (int i = 0; i < procs; i++)
			{
				if (arr_all_n[i] > 0)
					counter++;
			}
		}

		return counter;
	}

	std::string Get_File_Format(int number)
	{
		if (number == 0)
		{
			return "File    : JSON/JSONL";
		}
		else if (number == 1)
		{
			return "File    : Binary";
		}
		else if (number == 2)
		{
			return "File    : MsgPack";
		}
		else if (number == 3)
		{
			return "Data    : ZMQ";
		}
			
		else
		{
			return "Data    : undefined!";
		}
	}

	// sorts array and returns index
	int *Sort_With_Index(double *v, int N)
	{

		int *id = (int *)malloc(N * sizeof(int));
		for (int i = 0; i < N; i++)
			id[i] = i;

		std::stable_sort(id, id + N, [&v](int i, int j)
						 { return v[i] < v[j]; });

		return id;
	}

	int *Sort_With_Index(collect *all_data, int N, std::string mode)
	{

		int *id = (int *)malloc(N * sizeof(int));
		for (int i = 0; i < N; i++)
			id[i] = i;

		std::stable_sort(id, id + N, [all_data, mode](int i, int j)
						 { return all_data[i].get(mode) < all_data[j].get(mode); });

		return id;
	}

	// prints graph of overlapping phases
	void Overlap_Graph(std::vector<std::vector<int>> &res, int N)
	{

		Function_Debug(__PRETTY_FUNCTION__);
		// print graph of overlapping phases
		if (N > 0)
		{
			unsigned int row = res.size();

			unsigned int col = 0;
			for (unsigned int i = 0; i < row; i++)
				col = (res[i].size() > col) ? res[i].size() : col;

			std::cout << "\n"
					  << std::string((int)log10(col), ' ') << " ^";

			for (unsigned int j = col; j > 0; j--)
			{
				std::cout << std::endl
						  << j << std::string((int)log10(col) - (int)log10(j), ' ') << "|";
				for (unsigned int i = 0; i < row; i++)
				{
					if (res[i].size() > j)
						std::cout << "\u2588";
					else if (res[i].size() == j)
						std::cout << "\u2588";
					else
						std::cout << " ";
				}
			}
			std::cout << "\n"
					  << std::string((int)log10(col), ' ') << " +" + std::string(row + 3, '-') + "> time" << std::endl;
		}
	}

	//**********************************************************************
	//*                        Overlap
	//**********************************************************************
	// function  for finding phases for the entire job
	void Overlap(std::vector<std::vector<int>> &res, std::vector<double> &time, double *t_start, double *t_end, int *phases_of_ranks, int N, int p)
	{
		Function_Debug(__PRETTY_FUNCTION__);
		// std::vector<std::vector<int>> res;
		std::vector<int> queue;

		int k_s = 0;
		int k_e = 0;

		// sort start and end times of the phases
		int *id_s = Sort_With_Index(t_start, N);
		int *id_e = Sort_With_Index(t_end, N);
		int n = 0;
		// Disp(t_start, N, "t_start = ");
		// Disp(t_end, N, "t_end = ");

		while (k_s < N || k_e < N)
		{

#if HDEBUG >= 2
			std::cout << "k_e = " << k_e << ", id_e[k_e] = " << id_e[k_e] << ",    t_end[id_e[k_e]] = " << t_end[id_e[k_e]] << std::endl;
			std::cout << "k_s = " << k_s << ", id_s[k_s] = " << id_s[k_s] << ", t_start[id_s[k_s]] = " << t_start[id_s[k_s]] << std::endl;
#endif

			//? case 1: end happend before start -> remove the phase from the queue
			// if it is the last phase, application I/O phase ended
			if (k_s == N || t_end[id_e[k_e]] < t_start[id_s[k_s]])
			{
				if (queue.size() == 1)
					queue.pop_back();
				else
				{
					for (unsigned int j = 0; j < queue.size(); j++)
					{
#if HDEBUG >= 2
						std::cout << "\t( " << queue[j] << " == " << id_e[k_e] << " )?";
#endif
						if (queue[j] == id_e[k_e])
						{
							queue.erase(queue.begin() + j);
#if HDEBUG >= 2
							std::cout << "-> removed " << id_e[k_e] << std::endl;
#endif
							break;
						}
					}
				}
//* debug: show current phase
#if HDEBUG >= 1
				std::cout << "phase " << id_e[k_e] << " -> end" << std::endl;
				std::cout << "queue: ";
				for (unsigned int i = 0; i < queue.size(); i++)
					std::cout << " " << queue[i] << " ";
				std::cout << std::endl;
#endif
// track time if ALL_SAMPLE is set
#if ALL_SAMPLES > 2
				time.push_back(t_end[id_e[k_e]]);
				// std::cout << "t_end[id_e[k_e]] " << t_end[id_e[k_e]]<<  std::endl;
#endif

				k_e++;
			}
			//? case 2: start is before end -> overlap. Add the overlaping phase to queue
			else
			{
				queue.push_back(id_s[k_s]);
#if HDEBUG >= 1
				std::cout << "phase " << id_s[k_s] << " -> start" << std::endl;
				std::cout << "queue: ";
				for (unsigned int i = 0; i < queue.size(); i++)
					std::cout << " " << queue[i] << " ";
				std::cout << std::endl;
#endif
#if ALL_SAMPLES > 2
				time.push_back(t_start[id_s[k_s]]);
				// std::cout << "t_start[id_s[k_s]] " << t_start[id_s[k_s]]<<  std::endl;
#endif
				k_s++;
			}

			// snapshot current queue to result
			res.push_back(std::vector<int>());
			res[n++] = queue;
		}

// plot graph if flag is set
#if OVERLAP_GRAPH > 0
		Overlap_Graph(res, N);
#endif

#if HDEBUG > 0
		if (N > 0)
		{
			// print sorted and unsorted start and end time
			for (int i = 0; i < N; i++)
				std::cout << "t_start(" << i << ") = " << t_start[i] << "\t"
						  << "sorted t_start(" << i << ") = " << t_start[id_s[i]] << std::endl;
			for (int i = 0; i < N; i++)
				std::cout << "t_end(" << i << ")  = " << t_end[i] << "\t"
						  << "sorted t_end(" << i << ") = " << t_end[id_e[i]] << std::endl;

			// print all phases
			for (unsigned int rowD = 0; rowD < res.size(); rowD++)
			{
				std::cout << std::endl
						  << "Phase " << rowD << ": ";
				for (unsigned int colD = 0; colD < res[rowD].size(); colD++)
					std::cout << res[rowD][colD] << " ";
			}
			std::cout << "\n\n";
		}
#endif

		free(id_s);
		free(id_e);
	}

	double *Phase_Bandwidth(std::vector<std::vector<int>> &res, double *buff)
	{

		// int app_phase = 0;

		// ALL_SAMPLES > 1 print also zeroes bandwidth values, else skip
		Function_Debug(__PRETTY_FUNCTION__);

#if ALL_SAMPLES > 1 // also print zero bandiwdth
		double *out = (double *)malloc(res.size() * sizeof(double));
		// add all bandidth in phases
		for (unsigned int i = 0; i < res.size(); i++)
		{
			out[i] = 0;
			for (unsigned int j = 0; j < res[i].size(); j++)
			{
				// if (!isnan(buff[res[i][j]]))
				out[i] += buff[res[i][j]];
			}

			// if (res[i].size() == 0)
			//   std::cout << "Application phase "<< ++app_phase << " over" << std::endl;
		}
#else
		// find number of non-empty overlaps (n_overla)
		int n = Non_Empty(res);
		double *out = (double *)malloc(n * sizeof(double));
		n = 0;
		// add all bandidth in phases
		for (int i = 0; i < res.size(); i++)
		{
			if (!res[i].empty())
			{
				out[n] = 0;
				for (int j = 0; j < res[i].size(); j++)
				{
					if (!isnan(buff[res[i][j]]))
					{
						if (j == 0)
							out[n] = buff[res[i][j]];
						else
							out[n] += buff[res[i][j]];
					}
				}

				n++;
			}
		}
#endif

		return out;
	}

	int Non_Empty(std::vector<std::vector<int>> &res)
	{

		int n = 0;
		// find non-zero phases
		for (unsigned int i = 0; i < res.size(); i++)
			if (!res[i].empty())
				n++;
		return n;
	}

	int *N_Phase(std::vector<std::vector<int>> &res)
	{
		if (res.size() == 0)
			return NULL;
		else
		{
			int *out = (int *)malloc(res.size() * sizeof(int));

			for (unsigned int i = 0; i < res.size(); i++)
				out[i] = res[i].size();
			return out;
		}
	}

	double Agg_Over_Ranks(double *b, int *phases_of_ranks, int p, int mode)
	{
		Function_Debug(__PRETTY_FUNCTION__);
		double out = 0;
		int n = 0;

		// int c = 0;

		// one array store the ranks, the other is just an index run
		for (int i = 0; i < p; i++)
		{
			switch (mode)
			{
			case 1:
				out += Max(b + n, phases_of_ranks[i]);
				break;
			case 2:
				out += Min(b + n, phases_of_ranks[i]);
				break;
			case 3:
				out += Harmonic_Mean(b + n, phases_of_ranks[i]);
				break;
			case 4:
				out += Median(b + n, phases_of_ranks[i]);
				break;
			default:
				out += Max(b + n, phases_of_ranks[i]);
				break;
			}
			n += phases_of_ranks[i];

			// Debug
			//
			// std::cout <<"bandwidth rank " << i << " ";
			// for (int j = 0; j < phases_of_ranks[i]; j++)
			// {
			//     std::cout<< b[c+j] << " ";
			// }
			// c = c + phases_of_ranks[i];
			// std::cout << std::endl << "out is " << out << std::endl;
		}

		return out;
	}

	double Sample_Time(double *v, int n)
	{

		double min = std::numeric_limits<double>::infinity();
		for (int i = 0; i < n; i++)
		{
			for (int j = 0; j < n; j++)
			{
				if (i != j)
					min = (min < std::abs(v[i] - v[j])) ? min : std::abs(v[i] - v[j]);
			}
		}
		return min;
	}

	void Function_Debug(std::string s)
	{
	#if FUNCTION_INFO == 1
		std::cout << CYAN << "\t> executing " << s << BLACK << std::endl;
	#endif
	}


}