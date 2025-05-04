#include "test.h"

double *computation(double *, int, int, MPI_Request &, MPI_Status &, int, MPI_File &, int, int, double);
void write_to_file_async(int, std::string, int, int, int, int &, double *, double *, MPI_Status &, MPI_Request &, MPI_File &, int, int, MPI_Comm, double &);
void read_from_file(std::string, int, int, int, int, int, MPI_Comm);

int main(int argc, char *argv[])
{
    bool strong = 0; //weak or strong scaling
    int rank, size, N = 10'000;
    int loops = 8;
    int counter = 1; //counts simulation runs (don't modify)
    double *result = (double *)malloc(sizeof(double) * N);
    double *result_array;
    double t_io = 0;
    std::string filename = "file";
    //! each process does these things
    MPI_Init(&argc, &argv); //? after this, each process sees everything here private
    int hours, minutes;
    double seconds = MPI_Wtime();
    MPI_Comm_rank(MPI_COMM_WORLD, &rank); //? each process gets its id
    MPI_Comm_size(MPI_COMM_WORLD, &size); //? get number of processes
    if (strong)
        N = floor(N / (size * loops));
     
    if (rank == 0)
    {
#if DEBUG >= 0
        std::cerr << "DEUBG flag is active in test.cpp" << std::endl;
#endif
        printf("file of size %.li KB will be generated if all ranks do I/O\n", (N * sizeof(MPI_DOUBLE) * size * loops) / 1000);
    }

    for (int i = 0; i < N; i++)
        result[i] = rank * rank;
#if DEBUG >= 4
    printf("Here is process %d, at start result contains %d times the value  %f \n", rank, N, result[1]);
#endif

    //! process 0 allocates memory
    if (rank == 0)
    {
        result_array = (double *)malloc(sizeof(double) * N * size);
    }

    //! process 0 gathers from all
    MPI_Gather(result, N, MPI_DOUBLE, result_array, N, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    //! process 0 modifies the data
    if (rank == 0)
    {
#if DEBUG >= 4
        printf("\n--> process %d:\n", rank);
#endif
        for (int i = 0; i < size * N; i++)
        {
#if DEBUG >= 4
            printf("    gathered result_array[%d] = %f --> ", i, result_array[i]);
            result_array[i] = result_array[i] + i;
            printf("assigning result_array[%d] = %f \n", i, result_array[i]);
#endif
        }
#if DEBUG >= 4
        printf("Sending result to processes\n\n");
#endif
    }
    //! process 0 sends to all
    MPI_Scatter(result_array, N, MPI_DOUBLE, result, N, MPI_DOUBLE, 0, MPI_COMM_WORLD);
#if DEBUG >= 4
    printf("process %d: Final result is: ", rank);
    for (int i = 0; i < N; i++)
        printf(" %.2f ", result[i]);
    printf("\n");
    printf("------ Process %d Inititaion done ------\n", rank);
#endif
    // double x = iotrace.Get("aw", "t_end_act");
    // printf("x is %f",x);
    //! write
    //! ***************
    MPI_Info info;
    MPI_File mpi_file;
    MPI_Status status;
    MPI_Request request;

    //? 1) write mode
    /*!
    * @param mode: sets the writing mode of the test function
    * 1: serial single file                                         : MPI_File_write
    * 2: sync parallel    + shared file pointer + explicit offset   : MPI_File_write_at
    * 3: sync collective  + shared file pointer                     : MPI_File_write_all
    * 4: sync parallel    + independent files                       : MPI_File_write
    * 5: async parallel   + shared file pointer + explicit offset   : MPI_File_iwrite_at
    * 6: async collective + shared file pointer                     : MPI_File_iwrite_all
    * 7: async paralllel  + independent files                       : MPI_File_iwrite
    * 8: 3 (collective)       + sellective rankes                   : MPI_File_write_all
    * 9: 6 (async_collective) + sellective ranks                    : MPI_File_iwrite_all
    */
    int mode = 0;
    mode = 1;
    mode = 2;
    mode = 3;
    mode = 4;
    mode = 5;
    mode = 6;
    mode = 7;
    mode = 8;
    mode = 9;

    //? 2)
    //? which ranks should do collective IO (mode 3 and 6) --> if not all, set all=0 and adjust n_collective_ranks & collective_ranks
    int n_collective_ranks = 2;
    int collective_ranks[n_collective_ranks] = {0, 1};
    bool all = 0;

    MPI_Comm COMM;
    if (all || mode <= 7 || size == 1)
        COMM = MPI_COMM_WORLD;
    else
    {
        MPI_Group world_group;
        MPI_Comm_group(MPI_COMM_WORLD, &world_group);
        // Keep only the processes 0 and 1 in the new group.
        MPI_Group new_group;
        MPI_Group_incl(world_group, n_collective_ranks, collective_ranks, &new_group);
        // Create the new communicator from that group of processes.
        MPI_Comm_create(MPI_COMM_WORLD, new_group, &COMM);
    }

    //! write to file
    for (int i = 0; i < loops; i++)
    {
        result = computation(result, N, counter, request, status, rank, mpi_file, loops,i, t_io);
        // iotrace.Summary();
        write_to_file_async(mode, filename, rank, size, N, counter, result, result_array, status, request, mpi_file, loops, i, COMM, t_io);
        //std::cout << "counter is " << counter;
    }

    // wait for last IO loop
    MPI_Wait(&request, &status);

    //! close the file in async mode
    if (mode > 4 && mode != 8 && COMM != MPI_COMM_NULL)
        MPI_File_close(&mpi_file);

    //! read file
    read_from_file(filename, rank, mode, size, N, counter, COMM);

    if (rank == 0)
    {
        seconds = MPI_Wtime() - seconds;
        minutes = seconds / 60;
        hours = minutes / 60;
        fprintf(stdout, "Elapsed time: %f seconds\n", seconds);
        fprintf(stdout, "--> %.2i h %.2i m %f s\n", hours % 60, minutes % 60, seconds - hours % 60 * 3600 - minutes % 60 * 60);
    }

    if (rank == 0 && size == 1)
        std::cout << "\t  --------- END -----------\n\n";



#if NODE_DEBUG >= 1
    char name[MPI_MAX_PROCESSOR_NAME];
    int len; 
    MPI_Get_processor_name(name, &len);
    printf("Rank %d/%d was on %s\n", rank, size, name);fflush(stdout);
#endif
    
IOflush trace; 


// iotrace.Summary();
trace.start(5'000'000);
trace.end();
//trace.summary();
trace.short_summary();
trace.clear();
trace.start(1);
trace.end();
trace.short_summary();

free(result);
// free(result_array);

printf("------ test %d  done ------\n", rank);

// double x = iotrace.Get("aw", "t_end_act");
// printf("x is %f",x);
MPI_Finalize();
    return 0;
}

//! **************************************************************
//! Read function
//! **************************************************************
void read_from_file(std::string filename, int rank, int mode, int size, int N, int counter, MPI_Comm COMM)
{
    // Wait for all processes to reach this line
    //std::string a = "file";
    MPI_Barrier(MPI_COMM_WORLD);

    if (mode == 4 || mode == 7)
    {
        // all read their files
        MPI_File mpi_file;
        MPI_Status status;
        int read_doubles = 0;
        //double read_array[ N * (counter - 1)];
        double *read_array;
        read_array = (double *)malloc(sizeof(double) * N * (counter - 1));
        MPI_File_open(MPI_COMM_SELF, (filename + std::to_string(rank)).c_str(), MPI_MODE_RDONLY, MPI_INFO_NULL, &mpi_file);
        MPI_File_read(mpi_file, read_array, N * (counter - 1), MPI_DOUBLE, &status);
        MPI_Get_count(&status, MPI_DOUBLE, &read_doubles);
        free(read_array);
        MPI_File_close(&mpi_file);
    }
    else
    {
        if (rank == 0)
        {
            MPI_File mpi_file;
            MPI_Status status;
            int read_doubles = 0;
            double *read_array;
            read_array = (double *)malloc(sizeof(double) * size * N * (counter - 1));
            MPI_File_open(MPI_COMM_SELF, filename.c_str(), MPI_MODE_RDONLY, MPI_INFO_NULL, &mpi_file);
            
            if (mode == 5){
                MPI_Request request;
                MPI_File_iread(mpi_file, read_array, N * size * (counter - 1), MPI_DOUBLE, &request);
                MPI_Wait(&request,MPI_STATUS_IGNORE);
            }
            else
            MPI_File_read(mpi_file, read_array, N * size * (counter - 1), MPI_DOUBLE, &status);


            MPI_Get_count(&status, MPI_DOUBLE, &read_doubles);
#if DEBUG >= 1
            std::cout << "real read: " << read_doubles * sizeof(MPI_DOUBLE) / 1000 << " KB \n";
#endif
            MPI_File_close(&mpi_file);
#if DEBUG >= 1
            printf("\n%d Process were in totoal used with %d IO phases. In each run %d doubles were generated =  %d doubles in totoal = %lu KB \n", size, (counter - 1), N, N * size * (counter - 1), (sizeof(MPI_DOUBLE) * N * size * (counter - 1)) / 1000);
#endif
#if DEBUG >= 4
            {
                printf("values are:\n[");
                for (int i = 0; i < read_doubles; i++)
                {
                    if (i % N == 0)
                        printf("\n");
                    printf("%.2f ", read_array[i]);
                }
            }
            printf("]\n\n -------------------------- End --------------------------\n\n");
#endif
            free(read_array);
        }
    }

    MPI_Barrier(MPI_COMM_WORLD);
}

//! **************************************************************
//! Write function
//! **************************************************************
void write_to_file_async(int mode, std::string filename, int rank, int size, int N, int &counter, double *result, double *result_array, MPI_Status &status, MPI_Request &request, MPI_File &mpi_file, int loops, int currentLoop, MPI_Comm COMM, double &t)
{

    switch (mode)
    {
    //! (1) write to a single file with one process
    case 1: //! "serial"
    {
        if (rank == 0 && currentLoop == 0)
            printf("*******************************************\n* MPI : only process 0 writes to file\n*******************************************\n");

        MPI_Gather(result, N, MPI_DOUBLE, result_array, N, MPI_DOUBLE, 0, MPI_COMM_WORLD);

        if (rank == 0)
        {
#if DEBUG >= 3
            printf("Opening file only by process 0 \n");
#endif
            if (currentLoop == 0)
                MPI_File_open(MPI_COMM_SELF, filename.c_str(), MPI_MODE_APPEND | MPI_MODE_CREATE | MPI_MODE_WRONLY, MPI_INFO_NULL, &mpi_file);

#if DEBUG >= 3
            printf("write to file \n");
#endif
#if DEBUG >= 2
            t = MPI_Wtime();
            printf("rank %i: I/O start %.4f\n", rank, t);
#endif
            MPI_File_write(mpi_file, result_array, N * size, MPI_DOUBLE, &status);

#if DEBUG >= 3
            printf("closing file only by proces 0\n");
#endif
            if (currentLoop == loops)
                MPI_File_close(&mpi_file);
        }
        request = MPI_REQUEST_NULL;
        break;
    }

    //! (2) write to a single file with several processes independently
    case 2: //! "parallel"
    {

        if (rank == 0 && currentLoop == 0)
            printf("*******************************************\n* MPI : All process write in parallel to the same file \n*******************************************\n");
#if DEBUG >= 3
        printf("Opening file by process %d \n", rank);
#endif

        if (currentLoop == 0)
            MPI_File_open(MPI_COMM_SELF, filename.c_str(), MPI_MODE_APPEND | MPI_MODE_CREATE | MPI_MODE_WRONLY, MPI_INFO_NULL, &mpi_file);

        long long offset = rank * (N) * sizeof(double) + (counter - 1) * size * N * sizeof(double);
#if DEBUG >= 3
        printf("Rank: %d, Offset: %li\n", rank, offset);
#endif
#if DEBUG >= 2
        t = MPI_Wtime();
        printf("rank %i: I/O start %.4f\n", rank, t);
#endif
        //? writes N elements of type MPI_DOUBLE from memory result (pointer) to the file starting at offset
        MPI_File_write_at(mpi_file, offset, result, N, MPI_DOUBLE, &status);

#if DEBUG >= 3
        printf("Closing file by process %d \n", rank);
#endif
        if (currentLoop == loops)
            MPI_File_close(&mpi_file);

        request = MPI_REQUEST_NULL;
        break;
    }

    //! (3) write to a single file with several processes collectively
    case 3: //! "collective"
    {

        if (rank == 0 && currentLoop == 0)
            printf("*******************************************\n* MPI : All process write collectivly to file \n*******************************************\n");

#if DEBUG >= 3
        printf("Opening file by process %d \n", rank);
#endif
        if (currentLoop == 0)
            MPI_File_open(MPI_COMM_WORLD, filename.c_str(), MPI_MODE_APPEND | MPI_MODE_CREATE | MPI_MODE_WRONLY, MPI_INFO_NULL, &mpi_file);

        // result_array is only know to 0 --> send to all process if collective is used
        //! process 0 allocated allready memory
        if (rank != 0)
            result_array = (double *)malloc(sizeof(double) * N * size);
        MPI_Bcast(result_array, N * size, MPI_DOUBLE, 0, MPI_COMM_WORLD);

        // collective write
#if DEBUG >= 2
        t = MPI_Wtime();
        printf("rank %i: I/O start %.4f\n", rank, t);
#endif
        MPI_File_write_all(mpi_file, result_array, N * size, MPI_DOUBLE, &status);
#if DEBUG >= 3
        printf("Closing file by process %d \n", rank);
#endif
        if (currentLoop == loops)
            MPI_File_close(&mpi_file);
        request = MPI_REQUEST_NULL;
        break;
    }

    //!(4) write to a several files with several processes
    case 4: //! "independent"
    {
        if (rank == 0 && currentLoop == 0)
            printf("*******************************************\n* MPI : All process write independent files \n*******************************************\n");
#if DEBUG >= 3
        printf("Opening file by process %d \n", rank);
#endif
        if (currentLoop == 0)
            MPI_File_open(MPI_COMM_SELF, (filename + std::to_string(rank)).c_str(), MPI_MODE_APPEND | MPI_MODE_CREATE | MPI_MODE_WRONLY, MPI_INFO_NULL, &mpi_file);

#if DEBUG >= 2
        t = MPI_Wtime();
        printf("rank %i: I/O start %.4f\n", rank, t);
#endif
        MPI_File_write(mpi_file, result, N, MPI_DOUBLE, &status);

// if (b)
#if DEBUG >= 3
        printf("Closing file by process %d \n", rank);
#endif
        if (currentLoop == loops)
            MPI_File_close(&mpi_file);

        request = MPI_REQUEST_NULL;
        break;
    }

    case 5: //! "parallel"
    {       //! (5) write to a single file with several processes independently

        if (rank == 0 && currentLoop == 0) // print this stament only once
            printf("*********************************************************\n* MPI : All process write in parallel to the same file \n*********************************************************\n");

#if DEBUG >= 3
        printf("Opening file by process %d \n", rank);
#endif

        if (currentLoop == 0)
            MPI_File_open(MPI_COMM_SELF, filename.c_str(), MPI_MODE_APPEND | MPI_MODE_CREATE | MPI_MODE_WRONLY, MPI_INFO_NULL, &mpi_file);
        //MPI_File_open(MPI_COMM_WORLD, filename.c_str(), MPI_MODE_APPEND | MPI_MODE_CREATE | MPI_MODE_WRONLY, MPI_INFO_NULL, &mpi_file);

        // offset = position relative to current view, in units of etype
        long long offset = rank * (N) * sizeof(double) + (counter - 1) * size * N * sizeof(double);
#if DEBUG >= 3
        printf("Rank: %d, Offset: %li\n", rank, offset);
#endif
#if DEBUG >= 2
        t = MPI_Wtime();
        printf("rank %i: I/O start %.4f\n", rank, t);
#endif
        //? writes N elements of type MPI_DOUBLE from memory result (pointer) to the file starting at offset
        MPI_File_iwrite_at(mpi_file, offset, result, N, MPI_DOUBLE, &request);

        break;
    }

        //! (6) write to a single file with several processes collectively async
    case 6: //! "collective"
    {

        if (rank == 0 && currentLoop == 0) // print this stament only once
            printf("*********************************************************\n* MPI : All process write async collectivly to file \n*********************************************************\n");

#if DEBUG >= 3
        printf("Opening file by process %d \n", rank);
#endif

        if (currentLoop == 0)
            MPI_File_open(MPI_COMM_WORLD, filename.c_str(), MPI_MODE_APPEND | MPI_MODE_CREATE | MPI_MODE_WRONLY, MPI_INFO_NULL, &mpi_file);
        //MPI_File_open(MPI_COMM_WORLD, filename.c_str(),  MPI_MODE_CREATE | MPI_MODE_WRONLY, MPI_INFO_NULL, &mpi_file);

        // result_array is only know to 0 --> send to all process if collective is used
        //! process 0 allocated allready memory
        if (rank != 0)
            result_array = (double *)malloc(sizeof(double) * N * size);
        //get the new values of result and put the in result_array
        MPI_Gather(result, N, MPI_DOUBLE, result_array, N, MPI_DOUBLE, 0, MPI_COMM_WORLD);

        // send result array to all
        MPI_Bcast(result_array, N * size, MPI_DOUBLE, 0, MPI_COMM_WORLD);

#if DEBUG >= 2
        t = MPI_Wtime();
        printf("rank %i: I/O start %.4f\n", rank, t);
#endif
        // collective write
        MPI_File_iwrite_all(mpi_file, result_array, N * size, MPI_DOUBLE, &request);

        break;
    }

    //! (7) write async to file
    case 7: //! "async"
    {
        if (rank == 0 && currentLoop == 0)
            printf("*********************************************************\n* MPI : All process write async to diffrent files \n*********************************************************\n");
#if DEBUG >= 3
        printf("Opening file by process %d \n", rank);
#endif
        if (currentLoop == 0)
        {
            // if (rank == 0)
            //    MPI_File_open(MPI_COMM_SELF, (filename).c_str(), MPI_MODE_APPEND | MPI_MODE_CREATE | MPI_MODE_WRONLY, MPI_INFO_NULL, &mpi_file);
            // else
            MPI_File_open(MPI_COMM_SELF, (filename + std::to_string(rank)).c_str(), MPI_MODE_APPEND | MPI_MODE_CREATE | MPI_MODE_WRONLY, MPI_INFO_NULL, &mpi_file);
        }

#if DEBUG >= 2
        t = MPI_Wtime();
        printf("rank %i: I/O start %.4f\n", rank, t);
#endif

        MPI_File_iwrite(mpi_file, result, N, MPI_DOUBLE, &request);
        break;
    }

        //! (8) write to a single file with several processes collectively + selective ranks
    case 8:
    {

        if (rank == 0 && currentLoop == 0)
            printf("*******************************************\n* MPI : All process write collectivly to file \n*******************************************\n");

#if DEBUG >= 3
        printf("Opening file by process %d \n", rank);
#endif
        if (COMM != MPI_COMM_NULL && currentLoop == 0)
            MPI_File_open(COMM, filename.c_str(), MPI_MODE_APPEND | MPI_MODE_CREATE | MPI_MODE_WRONLY, MPI_INFO_NULL, &mpi_file);

        // result_array is only know to 0 --> send to all process if collective is used
        //! process 0 allocated allready memory
        if (rank != 0)
            result_array = (double *)malloc(sizeof(double) * N * size);
        MPI_Bcast(result_array, N * size, MPI_DOUBLE, 0, MPI_COMM_WORLD);

        // collective write
        if (COMM != MPI_COMM_NULL)
        {
#if DEBUG >= 2
            t = MPI_Wtime();
            printf("rank %i: I/O start %.4f\n", rank, t);
#endif
            MPI_File_write_all(mpi_file, result_array, N * size, MPI_DOUBLE, &status);

#if DEBUG >= 3
            printf("Closing file by process %d \n", rank);
#endif
            if (currentLoop == loops)
                MPI_File_close(&mpi_file);
        }
        request = MPI_REQUEST_NULL;
        break;
    }

        //! (9) write to a single file with several processes collectively async + selective ranks
    case 9: //! "collective" + selective ranks
    {

        if (rank == 0 && currentLoop == 0) // print this stament only once
            printf("*********************************************************\n* MPI : All process write async collectivly to file \n*********************************************************\n");

#if DEBUG >= 3
        printf("Opening file by process %d \n", rank);
#endif

        if (COMM != MPI_COMM_NULL && currentLoop == 0)
            MPI_File_open(COMM, filename.c_str(), MPI_MODE_APPEND | MPI_MODE_CREATE | MPI_MODE_WRONLY, MPI_INFO_NULL, &mpi_file);
        //MPI_File_open(MPI_COMM_WORLD, filename.c_str(),  MPI_MODE_CREATE | MPI_MODE_WRONLY, MPI_INFO_NULL, &mpi_file);

        // result_array is only know to 0 --> send to all process if collective is used
        //! process 0 allocated allready memory
        if (rank != 0)
            result_array = (double *)malloc(sizeof(double) * N * size);
        //get the new values of result and put the in result_array
        MPI_Gather(result, N, MPI_DOUBLE, result_array, N, MPI_DOUBLE, 0, MPI_COMM_WORLD);

        // send result array to all
        MPI_Bcast(result_array, N * size, MPI_DOUBLE, 0, MPI_COMM_WORLD);

        // collective write
        if (COMM != MPI_COMM_NULL)
        {
#if DEBUG >= 2
            t = MPI_Wtime();
            printf("rank %i: I/O start %.4f\n", rank, t);
#endif
            MPI_File_iwrite_all(mpi_file, result_array, N * size, MPI_DOUBLE, &request);
        }
        else
            request = MPI_REQUEST_NULL;
        break;
    }

    default:
    {
        printf("not implemented\n");
        request = MPI_REQUEST_NULL;
        break;
    }
    }

    counter++;
}

//! *************************************************************************
//! Compute function
//! *************************************************************************
double *computation(double *result, int N, int counter, MPI_Request &request, MPI_Status &status, int rank, MPI_File &mpi_file, int loops, int currentLoop, double t_io)
{
#if DEBUG >= 2
    double t = MPI_Wtime();
    double t_io_end = 0;
    printf("rank %i: compute start %.4f\n", rank, t);
#endif

    #ifdef SCOREP
    SCOREP_USER_REGION_DEFINE(my_region_handle)
    SCOREP_USER_REGION_BEGIN(my_region_handle, "compute_calc", SCOREP_USER_REGION_TYPE_COMMON)
    #endif
    double tmp[N];
    double dummy[N];
    double dummy2;
    int flag = 0, IOcomplete = 0;
    for (int i = 0; i < N; i++)
    {
        tmp[i] = pow(result[i], 2);
        tmp[i] = sqrt(tmp[i] + result[i]);
        tmp[i] = tmp[i] + 1;
        tmp[i] = result[i] / tmp[i];
        if (i > 0)
            dummy2 = pow(cos(tmp[i]), 2) + pow(sin(tmp[i]), 2) - 1 + tmp[i - 1];
        else
            dummy2 = 0;

        dummy[i] = (1 - pow(tan(dummy2), 2)) / (1 + pow(tan(dummy2), 2)) - pow(sin(dummy2), 2) + pow(sin(dummy2), 2);

        if (i > 0)
            dummy[i - 1] = (1 - pow(tan(dummy[i]), 2)) / (1 + pow(tan(dummy[i]), 2)) - pow(sin(dummy[i]), 2) + pow(sin(dummy[i]), 2);
        else
            dummy[i - 1] = 0;

        if (tmp[i] <= 10)
            tmp[i] = 0.01 * tmp[i] + rank + 0.1 * (counter - 1);
        else if (tmp[i] <= 100)
            tmp[i] = 0.001 * tmp[i] + rank + 0.1 * (counter - 1);
        else if (tmp[i] <= 1000)
            tmp[i] = 0.0001 * tmp[i] + rank + 0.1 * (counter - 1);
        else
            tmp[i] = 0.00001 * tmp[i] + rank + 0.1 * (counter - 1);

        if (i % (int)ceil(N / 10) == 0 && IOcomplete == 0 && currentLoop > 0)
        {
            MPI_Test(&request, &flag, &status);
            if (flag == 1)
            {
                IOcomplete = 1;
#if DEBUG >= 2
                t_io_end = MPI_Wtime();
                printf("rank %i: I/O end %.4f --> duration %.4f\n", rank, t_io_end, t_io_end - t_io);
#endif
            }
        }
    }

    #ifdef SCOREP
    SCOREP_USER_REGION_END(my_region_handle)
    #endif
    //if (currentLoop > 0 && flag == 0)
    if (currentLoop > 0)
    {
        #if DEBUG >= 2
        t_io_end = MPI_Wtime();
        printf("rank %i: I/O required %.4f --> duration %.4f\n", rank, t_io_end, t_io_end - t_io);
        #endif
        MPI_Wait(&request, &status);
#if DEBUG >= 2
if (IOcomplete == 1){
        t_io_end = MPI_Wtime();
        printf("rank %i: I/O end %.4f --> duration %.4f\n", rank, t_io_end, t_io_end - t_io);
}
#endif
    }

    #ifdef SCOREP
    SCOREP_USER_REGION_DEFINE(my_region_handle2)
    SCOREP_USER_REGION_BEGIN(my_region_handle2, "comput_copy_result", SCOREP_USER_REGION_TYPE_COMMON)
    #endif
    memcpy(result, tmp, N * sizeof(*tmp));
    #ifdef SCOREP
    SCOREP_USER_REGION_END(my_region_handle2)
    #endif

#if DEBUG >= 2
    double t_end = MPI_Wtime();
    printf("rank %i: compute end %.4f --> duration %.4f\n", rank, t_end, t_end - t);
#endif
    return result;
}
