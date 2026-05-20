#include <mpi.h>
#include <limits>
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <tmio.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    int numtasks, myrank, status;
    int runs = 30;
    int provided;
    status = MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
    if (MPI_SUCCESS != status)
    {
        printf(" Error Starting the MPI Program \n");
        MPI_Abort(MPI_COMM_WORLD, status);
    }

    if (provided < MPI_THREAD_MULTIPLE) {
        printf(" MPI_THREAD_MULTIPLE not available \n");
        MPI_Abort(MPI_COMM_WORLD, status);
    }

    MPI_Comm_size(MPI_COMM_WORLD, &numtasks);
    MPI_Comm_rank(MPI_COMM_WORLD, &myrank);

    std::string file = "testfile_rank_" + std::to_string(myrank) + ".txt";
    int count = 20;

    MPI_File fh;
    MPI_File_open(MPI_COMM_WORLD, file.c_str(), MPI_MODE_CREATE | MPI_MODE_RDWR, MPI_INFO_NULL, &fh);

    std::vector<int> nums;
    for(int i = 0; i < count; i++) {
        nums.push_back(i);
    }

    std::vector<int> in(nums.size());

    tmio::init_prefetcher(0.333, 10'000, 10'000);

    MPI_File_write(fh, nums.data(), nums.size(), MPI_INT, MPI_STATUS_IGNORE);
    
    for(int i = 0; i < runs; i++) {
        
        sleep(3);

        MPI_File_read_at(fh, 0, in.data(), count, MPI_INT, MPI_STATUS_IGNORE);
        /*
        if(i == 10) {
            tmio::iotrace_summary();
        } else if (i == 11) {
            tmio::retrieve_frequencies();
        }
        */    
        
    }

    MPI_File_close(&fh);
    MPI_Finalize();
}