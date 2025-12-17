# POSIX Sanity Check Example

This directory contains a basic sanity check designed to verify the correct interception and execution of POSIX I/O calls within the TMIO environment. It demonstrates a simple workflow: opening a file, reading content, and closing the file, all while running within an MPI context.

## Overview

The `POSIX_example.cxx` test performs the following operations:
1.  **MPI Initialization**: Sets up the MPI environment (`MPI_Init`).
2.  **File Open**: Uses the standard POSIX `open()` system call to create or open `testfile.txt`.
3.  **File Read**: Uses the standard POSIX `read()` system call to read data from the file into a buffer.
4.  **File Close**: Uses the standard POSIX `close()` system call to release the file descriptor.
5.  **MPI Finalization**: Cleans up the MPI environment (`MPI_Finalize`).

This test ensures that the TMIO library correctly intercepts these standard libc calls without breaking basic functionality.

## Compilation

To compile this example, you need an MPI C++ compiler (like `mpicxx`).

```bash
mpicxx -o POSIX_example POSIX_example.cxx
```

*Note: Ensure that your TMIO library is linked correctly if required by your specific build environment (e.g., via `LD_PRELOAD` at runtime or direct linking during compilation).*

## Running the Example

This example is intended to be run using `mpirun` or `mpiexec`.

### 1. Create a dummy file
Since the test reads from `testfile.txt`, create this file with some content before running:

```bash
echo "Hello TMIO World" > testfile.txt
```

### 2. Execute the binary
Run the compiled binary using `mpirun`.

```bash
# Run with 1 MPI process
mpirun -n 1 ./POSIX_example
```

**Using with TMIO Interception:**
To test TMIO's functionality, you usually preload the TMIO library.

```bash
mpirun -n 1 -x LD_PRELOAD=/path/to/libtmio.so ./POSIX_example
```

## Expected Output

Upon successful execution, the output should look similar to:

```text
===========================
        TMIO Settings      
===========================
Test    : 1
Calc    : 0
Samples : 5
Tracing : MPI
File    : JSON/JSONL
===========================


===========================
        TMIO Settings      
===========================
Test    : 1
Calc    : 0
Samples : 5
Tracing : Libc
File    : JSON/JSONL
===========================

...

Summary
***************************
I/O library: Libc
Ranks: 1 

 _________________Read_________________
|
| _________Async_Read_________
| | Max number of ranks        : 0   
| | Total read bytes           : 0.00 B
| | Max read bytes per rank    : 0.00 B
| | Max transfersize           : 0.00 B
| |
| |  ___Throughput___
| | | Max # of I/O read phases per rank       : 0 
| | | Aggregated # of I/O read phases         : 0 
| | | Max # of I/O read ops in phase          : 0 
| | | Max # of I/O overlaping phases          : 0 
| | | Max # of I/O read ops per rank          : 0 
| | | Aggregated # of I/O read ops            : 0 
| | | Weighted harmonic mean                  : 0.000 MB/s
| | | Harmonic mean                           : 0.000 MB/s
| | | Arithmetic mean                         : 0.000 MB/s
| | | Median                                  : 0.000 MB/s
| | | Max                                     : 0.000 MB/s
| | | Min                                     : 0.000 MB/s
| | | Harmonic mean x ranks                   : 0.000 MB/s
| | | Aritmetic mean x ranks                  : 0.000 MB/s
| |
| |  ___Required Bandwidth___
| | | Max # of I/O read phases per rank       : 0 
| | | Aggregated # of I/O read phases         : 0 
| | | Max # of I/O read ops in phase          : 0 
| | | Max # of I/O overlaping phases          : 0 
| | | Max # of I/O read ops per rank          : 0 
| | | Aggregated # of I/O read ops            : 0 
| | | Weighted harmonic mean                  : 0.000 MB/s
| | | Harmonic mean                           : 0.000 MB/s
| | | Arithmetic mean                         : 0.000 MB/s
| | | Median                                  : 0.000 MB/s
| | | Max                                     : 0.000 MB/s
| | | Min                                     : 0.000 MB/s
| | | Harmonic mean x ranks                   : 0.000 MB/s
| | | Aritmetic mean x ranks                  : 0.000 MB/s
| | |  ___Average___
| | | | Weighted harmonic mean                : 0.000 MB/s
| | | | Harmonic mean                         : 0.000 MB/s
| | | | Arithmetic mean                       : 0.000 MB/s
| | | | Median                                : 0.000 MB/s
| | | | Max                                   : 0.000 MB/s
| | | | Min                                   : 0.000 MB/s
| | | | Harmonic mean x ranks                 : 0.000 MB/s
| | | | Aritmetic mean x ranks                : 0.000 MB/s
|
|  _________Sync_Read_________
| | Max number of ranks        : 1   
| | Total read bytes           : 99.00 B
| | Max read bytes per rank    : 99.00 B
| | Max transfersize           : 99.00 B
| |
| | Max # of I/O read phases per rank         : 1 
| | Aggregated # of I/O read phases           : 1 
| | Max # of I/O read ops in phase            : 1 
| | Max # of I/O overlaping phases            : 0 
| | Max # of I/O read ops per rank            : 1 
| | Aggregated # of I/O read ops              : 1 
| | Weighted harmonic mean                    : 3.084 MB/s
| | Harmonic mean                             : 3.084 MB/s
| | Arithmetic mean                           : 3.084 MB/s
| | Median                                    : 3.084 MB/s
| | Max                                       : 3.084 MB/s
| | Min                                       : 3.084 MB/s
| | Harmonic mean x ranks                     : 3.084 MB/s
| | Aritmetic mean x ranks                    : 3.084 MB/s



 _________________Write_________________
|
| _________Async_Write_________
| | Max number of ranks        : 0   
| | Total written bytes        : 0.00 B
| | Max written bytes per rank : 0.00 B
| | Max transfersize           : 0.00 B
| |
| |  ___Throughput___
| | | Max # of I/O write phases per rank      : 0 
| | | Aggregated # of I/O write phases        : 0 
| | | Max # of I/O write ops in phase         : 0 
| | | Max # of I/O overlaping phases          : 0 
| | | Max # of I/O write ops per rank         : 0 
| | | Aggregated # of I/O write ops           : 0 
| | | Weighted harmonic mean                  : 0.000 MB/s
| | | Harmonic mean                           : 0.000 MB/s
| | | Arithmetic mean                         : 0.000 MB/s
| | | Median                                  : 0.000 MB/s
| | | Max                                     : 0.000 MB/s
| | | Min                                     : 0.000 MB/s
| | | Harmonic mean x ranks                   : 0.000 MB/s
| | | Aritmetic mean x ranks                  : 0.000 MB/s
| |
| |  ___Required Bandwidth___
| | | Max # of I/O write phases per rank      : 0 
| | | Aggregated # of I/O write phases        : 0 
| | | Max # of I/O write ops in phase         : 0 
| | | Max # of I/O overlaping phases          : 0 
| | | Max # of I/O write ops per rank         : 0 
| | | Aggregated # of I/O write ops           : 0 
| | | Weighted harmonic mean                  : 0.000 MB/s
| | | Harmonic mean                           : 0.000 MB/s
| | | Arithmetic mean                         : 0.000 MB/s
| | | Median                                  : 0.000 MB/s
| | | Max                                     : 0.000 MB/s
| | | Min                                     : 0.000 MB/s
| | | Harmonic mean x ranks                   : 0.000 MB/s
| | | Aritmetic mean x ranks                  : 0.000 MB/s
| | |  ___Average___
| | | | Weighted harmonic mean                : 0.000 MB/s
| | | | Harmonic mean                         : 0.000 MB/s
| | | | Arithmetic mean                       : 0.000 MB/s
| | | | Median                                : 0.000 MB/s
| | | | Max                                   : 0.000 MB/s
| | | | Min                                   : 0.000 MB/s
| | | | Harmonic mean x ranks                 : 0.000 MB/s
| | | | Aritmetic mean x ranks                : 0.000 MB/s
|
|  _________Sync_Write_________
| | Max number of ranks        : 1   
| | Total written bytes        : 7.12 KB
| | Max written bytes per rank : 7.12 KB
| | Max transfersize           : 4.36 KB
| |
| | Max # of I/O write phases per rank        : 3 
| | Aggregated # of I/O write phases          : 3 
| | Max # of I/O write ops in phase           : 1 
| | Max # of I/O overlaping phases            : 0 
| | Max # of I/O write ops per rank           : 3 
| | Aggregated # of I/O write ops             : 3 
| | Weighted harmonic mean                    : 72.252 MB/s
| | Harmonic mean                             : 0.129 MB/s
| | Arithmetic mean                           : 90.804 MB/s
| | Median                                    : 106.413 MB/s
| | Max                                       : 165.956 MB/s
| | Min                                       : 0.043 MB/s
| | Harmonic mean x ranks                     : 0.129 MB/s
| | Aritmetic mean x ranks                    : 90.804 MB/s

ellapsed time (rank 0)             = 0.001312 sec
|-> application time               = 0.001203 sec       -> from ellapsed time 91.73 %
|-> overhead during runtime        = 0.000069 sec       -> from ellapsed time 5.25 %
'-> overhead post runtime          = 0.000040 sec       -> from ellapsed time 3.02 %

total run time                     = 0.001311 sec
|-> lib overhead time              = 0.000108 sec       -> from run time 8.22 %
|     |-> during runtime           = 0.000069 sec       -> from overhead 63.83 %
|     '-> post runtime             = 0.000039 sec       -> from overhead 36.17 %
|
'-> app time                       = 0.001203 sec       -> from run time 91.78 %
    |-> total compute/comm. time   = 0.001073 sec       -> from app time 89.15 %
    '-> total I/O time             = 0.000131 sec       -> from app time 10.85 %
        |-> sync read time         = 0.000032 sec       -> from app time 2.67 %
        |-> sync write time        = 0.000099 sec       -> from app time 8.19 %
        |
        |-> req. async read time   = 0.000000 sec       -> from app time 0.00 %
        |-> async read time        = 0.000000 sec       -> from app time 0.00 %
        |    |-> approx. wait time = 0.000000 sec       -> from app time 0.00 %
        |    '-> real wait time    = 0.000000 sec       -> from app time 0.00 %
        |
        |-> req. async write time  = 0.000000 sec       -> from app time 0.00 %
        '-> async write time       = 0.000000 sec       -> from app time 0.00 %
             |-> approx. wait time = 0.000000 sec       -> from app time 0.00 %
             '-> real wait time    = 0.000000 sec       -> from app time 0.00 %
```

## Code Breakdown

### Initialization
```cpp
MPI_Init(&argc, &argv);
```
Initializes the MPI execution environment. This is required even for POSIX tests to ensure the TMIO runtime (which often relies on MPI) is correctly set up.

### File Operations
```cpp
int fd = open("testfile.txt", O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
// ...
ssize_t bytesRead = read(fd, buffer, sizeof(buffer) - 1);
// ...
close(fd);
```
These standard UNIX system calls are the key targets for this test. In a standard run, the OS kernel handles them. In a TMIO run, the library intercepts these calls to potentially redirect I/O or track metadata.

### Error Handling
The code checks for return values of `-1` for `open`, `read`, and `close`. If an error occurs, it prints a system error message using `perror`, finalizes MPI, and exits with a failure code.

