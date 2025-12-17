#include <fcntl.h>
#include <unistd.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <errno.h>
#include <aio.h>
#include <pthread.h>
#include <mpi.h>
#include <iostream>

int main(int argc, char **argv)
{
    MPI_Init(&argc, &argv);

    // Use pthread open to open a file
    int fd = open("testfile.txt", O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    if (fd == -1) {
        perror("open");
        MPI_Finalize();
        return EXIT_FAILURE;
    }

    std::cout << "File opened successfully with file descriptor: " << fd << std::endl;

    // Use pthread read to read from the file
    char buffer[100];
    ssize_t bytesRead = read(fd, buffer, sizeof(buffer) - 1);
    if (bytesRead == -1) {
        perror("read");
        close(fd);
        MPI_Finalize();
        return EXIT_FAILURE;
    }
    buffer[bytesRead] = '\0'; // Null-terminate the string
    std::cout << "Read " << bytesRead << " bytes: " << buffer << std::endl;

    // Use pthread close to close the file
    if (close(fd) == -1) {
        perror("close");
        MPI_Finalize();
        return EXIT_FAILURE;
    }

    std::cout << "File closed successfully." << std::endl;

    MPI_Finalize();
    return 0;
}
