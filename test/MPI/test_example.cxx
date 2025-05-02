#define CATCH_CONFIG_RUNNER
#include "catch.hpp"
#include <mpi.h>
#include <vector>
#include <iostream>

// Example function to test
int sum(int a, int b) {
    return a + b;
}

TEST_CASE("MPI and Catch2 test") {
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // Each process performs a simple test
    REQUIRE(sum(rank, 1) == rank + 1);

    // Example of using MPI to gather results
    std::vector<int> results(size);
    MPI_Gather(&rank, 1, MPI_INT, results.data(), 1, MPI_INT, 0, MPI_COMM_WORLD);

    if (rank == 0) {
        for (int i = 0; i < size; ++i) {
            std::cout << "Process " << i << " result: " << results[i] << std::endl;
        }
    }
}

int main(int argc, char* argv[]) {
    MPI_Init(&argc, &argv);

    int result = Catch::Session().run(argc, argv);

    MPI_Finalize();
    return result;
}