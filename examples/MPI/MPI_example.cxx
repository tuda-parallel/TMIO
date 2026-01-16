#include <mpi.h>
#include <iostream>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv)
{
    MPI_Init(&argc, &argv);

    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    // Use MPI_File_open to open a file
    MPI_File fh;
    // Using MPI_MODE_RDWR | MPI_MODE_CREATE to match O_RDWR | O_CREAT
    int err = MPI_File_open(MPI_COMM_WORLD, "testfile.txt", MPI_MODE_RDWR | MPI_MODE_CREATE, MPI_INFO_NULL, &fh);
    if (err != MPI_SUCCESS) {
        char error_string[MPI_MAX_ERROR_STRING];
        int length_of_error_string;
        MPI_Error_string(err, error_string, &length_of_error_string);
        std::cerr << "MPI_File_open failed: " << error_string << std::endl;
        MPI_Finalize();
        return EXIT_FAILURE;
    }

    if (rank == 0) {
        std::cout << "File opened successfully." << std::endl;
    }

    // Use MPI_File_read to read from the file
    char buffer[100];
    MPI_Status status;
    err = MPI_File_read(fh, buffer, sizeof(buffer) - 1, MPI_CHAR, &status);
    if (err != MPI_SUCCESS) {
        char error_string[MPI_MAX_ERROR_STRING];
        int length_of_error_string;
        MPI_Error_string(err, error_string, &length_of_error_string);
        std::cerr << "MPI_File_read failed: " << error_string << std::endl;
        MPI_File_close(&fh);
        MPI_Finalize();
        return EXIT_FAILURE;
    }

    int bytesRead;
    MPI_Get_count(&status, MPI_CHAR, &bytesRead);
    buffer[bytesRead] = '\0'; // Null-terminate the string

    if (rank == 0) {
        std::cout << "Read " << bytesRead << " bytes: " << buffer << std::endl;
    }

    // Use MPI_File_close to close the file
    err = MPI_File_close(&fh);
    if (err != MPI_SUCCESS) {
        char error_string[MPI_MAX_ERROR_STRING];
        int length_of_error_string;
        MPI_Error_string(err, error_string, &length_of_error_string);
        std::cerr << "MPI_File_close failed: " << error_string << std::endl;
        MPI_Finalize();
        return EXIT_FAILURE;
    }

    if (rank == 0) {
        std::cout << "File closed successfully." << std::endl;
    }

    MPI_Finalize();
    return 0;
}