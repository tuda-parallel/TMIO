# IO_Uring Sanity Check Example

This directory contains a basic sanity check designed to verify the correct interception and execution of asynchronous I/O calls using `io_uring` within the TMIO environment.

## Overview

The `IO_Uring_example.cxx` test performs the following operations:
1.  **MPI Initialization**: Sets up the MPI environment (`MPI_Init`).
2.  **Uring Initialization**: Initializes the `io_uring` queue (`io_uring_queue_init`).
3.  **File Open**: Opens `testfile.txt` using standard POSIX `open`.
4.  **Submission**: Prepares a `readv` operation via `io_uring_prep_readv` and submits it to the kernel.
5.  **Completion**: Waits for the operation to complete (`io_uring_wait_cqe`) and prints the result.
6.  **Cleanup**: Closes the file and exits the ring.

This test ensures that TMIO can handle modern Linux asynchronous I/O interfaces.

## Prerequisites

You must have `liburing` installed on your system.
*   **Ubuntu/Debian**: `sudo apt-get install liburing-dev`
*   **Fedora/RHEL**: `sudo dnf install liburing-devel`

## Compilation

To compile this example, you need an MPI C++ compiler and must link against `liburing`.

```bash
mpicc ./IO_Uring_example.c -o IO_Uring_example -luring
```

**Using with TMIO Interception:**
To test TMIO's functionality, you usually preload the TMIO library.

```bash
mpirun -n 1 -x LD_PRELOAD=/path/to/libtmio.so  mpirun -np 1  ./IO_Uring_example README.md
```

## Expected Output

Upon successful execution, the output should look similar to:

```code
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


===========================
        TMIO Settings      
===========================
Test    : 1
Calc    : 0
Samples : 5
Tracing : IOuring
File    : JSON/JSONL
===========================

# IO_Uring Sanity Check Example

This directory contains a basic sanity check designed to verify the correct interception and execution of asynchronous I/O calls using `io_uring` within the TMIO environment.

## Overview

The `IO_Uring_example.cxx` test performs the following operations:
1.  **MPI Initialization**: Sets up the MPI environment (`MPI_Init`).
2.  **Uring Initialization**: Initializes the `io_uring` queue (`io_uring_queue_init`).
3.  **File Open**: Opens `testfile.txt` using standard POSIX `open`.
4.  **Submission**: Prepares a `readv` operation via `io_uring_prep_readv` and submits it to the kernel.
5.  **Completion**: Waits for the operation to complete (`io_uring_wait_cqe`) and prints the result.
6.  **Cleanup**: Closes the file and exits the ring.

This test ensures that TMIO can handle modern Linux asynchronous I/O interfaces.

## Prerequisites

You must have `liburing` installed on your system.
*   **Ubuntu/Debian**: `sudo apt-get install liburing-dev`
*   **Fedora/RHEL**: `sudo dnf install liburing-devel`

## Compilation

To compile this example, you need an MPI C++ compiler and must link against `liburing`.

```bash
mpicc ./IO_Uring_example.c -o IO_Uring_example -luring
```

Summary
***************************
I/O library: MPI
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
| | Max number of ranks        : 0   
| | Total read bytes           : 0.00 B
| | Max read bytes per rank    : 0.00 B
| | Max transfersize           : 0.00 B
| |
| | Max # of I/O read phases per rank         : 0 
| | Aggregated # of I/O read phases           : 0 
| | Max # of I/O read ops in phase            : 0 
| | Max # of I/O overlaping phases            : 0 
| | Max # of I/O read ops per rank            : 0 
| | Aggregated # of I/O read ops              : 0 
| | Weighted harmonic mean                    : 0.000 MB/s
| | Harmonic mean                             : 0.000 MB/s
| | Arithmetic mean                           : 0.000 MB/s
| | Median                                    : 0.000 MB/s
| | Max                                       : 0.000 MB/s
| | Min                                       : 0.000 MB/s
| | Harmonic mean x ranks                     : 0.000 MB/s
| | Aritmetic mean x ranks                    : 0.000 MB/s



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
| | Max number of ranks        : 0   
| | Total written bytes        : 0.00 B
| | Max written bytes per rank : 0.00 B
| | Max transfersize           : 0.00 B
| |
| | Max # of I/O write phases per rank        : 0 
| | Aggregated # of I/O write phases          : 0 
| | Max # of I/O write ops in phase           : 0 
| | Max # of I/O overlaping phases            : 0 
| | Max # of I/O write ops per rank           : 0 
| | Aggregated # of I/O write ops             : 0 
| | Weighted harmonic mean                    : 0.000 MB/s
| | Harmonic mean                             : 0.000 MB/s
| | Arithmetic mean                           : 0.000 MB/s
| | Median                                    : 0.000 MB/s
| | Max                                       : 0.000 MB/s
| | Min                                       : 0.000 MB/s
| | Harmonic mean x ranks                     : 0.000 MB/s
| | Aritmetic mean x ranks                    : 0.000 MB/s

ellapsed time (rank 0)             = 0.000660 sec
|-> application time               = 0.000606 sec       -> from ellapsed time 91.86 %
|-> overhead during runtime        = 0.000000 sec       -> from ellapsed time 0.00 %
'-> overhead post runtime          = 0.000054 sec       -> from ellapsed time 8.14 %

total run time                     = 0.000658 sec
|-> lib overhead time              = 0.000052 sec       -> from run time 7.84 %
|     |-> during runtime           = 0.000000 sec       -> from overhead 0.00 %
|     '-> post runtime             = 0.000052 sec       -> from overhead 100.00 %
|
'-> app time                       = 0.000606 sec       -> from run time 92.16 %
    |-> total compute/comm. time   = 0.000606 sec       -> from app time 100.00 %
    '-> total I/O time             = 0.000000 sec       -> from app time 0.00 %
        |-> sync read time         = 0.000000 sec       -> from app time 0.00 %
        |-> sync write time        = 0.000000 sec       -> from app time 0.00 %
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
| | Max number of ranks        : 0   
| | Total read bytes           : 0.00 B
| | Max read bytes per rank    : 0.00 B
| | Max transfersize           : 0.00 B
| |
| | Max # of I/O read phases per rank         : 0 
| | Aggregated # of I/O read phases           : 0 
| | Max # of I/O read ops in phase            : 0 
| | Max # of I/O overlaping phases            : 0 
| | Max # of I/O read ops per rank            : 0 
| | Aggregated # of I/O read ops              : 0 
| | Weighted harmonic mean                    : 0.000 MB/s
| | Harmonic mean                             : 0.000 MB/s
| | Arithmetic mean                           : 0.000 MB/s
| | Median                                    : 0.000 MB/s
| | Max                                       : 0.000 MB/s
| | Min                                       : 0.000 MB/s
| | Harmonic mean x ranks                     : 0.000 MB/s
| | Aritmetic mean x ranks                    : 0.000 MB/s



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
| | Weighted harmonic mean                    : 127.811 MB/s
| | Harmonic mean                             : 0.264 MB/s
| | Arithmetic mean                           : 183.881 MB/s
| | Median                                    : 120.484 MB/s
| | Max                                       : 431.070 MB/s
| | Min                                       : 0.088 MB/s
| | Harmonic mean x ranks                     : 0.264 MB/s
| | Aritmetic mean x ranks                    : 183.881 MB/s

ellapsed time (rank 0)             = 0.001221 sec
|-> application time               = 0.001159 sec       -> from ellapsed time 94.89 %
|-> overhead during runtime        = 0.000015 sec       -> from ellapsed time 1.26 %
'-> overhead post runtime          = 0.000047 sec       -> from ellapsed time 3.85 %

total run time                     = 0.001221 sec
|-> lib overhead time              = 0.000062 sec       -> from run time 5.07 %
|     |-> during runtime           = 0.000015 sec       -> from overhead 24.85 %
|     '-> post runtime             = 0.000046 sec       -> from overhead 75.15 %
|
'-> app time                       = 0.001159 sec       -> from run time 94.93 %
    |-> total compute/comm. time   = 0.001103 sec       -> from app time 95.20 %
    '-> total I/O time             = 0.000056 sec       -> from app time 4.80 %
        |-> sync read time         = 0.000000 sec       -> from app time 0.00 %
        |-> sync write time        = 0.000056 sec       -> from app time 4.80 %
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



Summary
***************************
I/O library: IOuring
Ranks: 1 

 _________________Read_________________
|
| _________Async_Read_________
| | Max number of ranks        : 1   
| | Total read bytes           : 1.21 KB
| | Max read bytes per rank    : 1.21 KB
| | Max transfersize           : 1.21 KB
| |
| |  ___Throughput___
| | | Max # of I/O read phases per rank       : 1 
| | | Aggregated # of I/O read phases         : 1 
| | | Max # of I/O read ops in phase          : 1 
| | | Max # of I/O overlaping phases          : 0 
| | | Max # of I/O read ops per rank          : 1 
| | | Aggregated # of I/O read ops            : 1 
| | | Weighted harmonic mean                  : 9.099 MB/s
| | | Harmonic mean                           : 9.099 MB/s
| | | Arithmetic mean                         : 9.099 MB/s
| | | Median                                  : 9.099 MB/s
| | | Max                                     : 9.099 MB/s
| | | Min                                     : 9.099 MB/s
| | | Harmonic mean x ranks                   : 9.099 MB/s
| | | Aritmetic mean x ranks                  : 9.099 MB/s
| |
| |  ___Required Bandwidth___
| | | Max # of I/O read phases per rank       : 1 
| | | Aggregated # of I/O read phases         : 1 
| | | Max # of I/O read ops in phase          : 1 
| | | Max # of I/O overlaping phases          : 0 
| | | Max # of I/O read ops per rank          : 1 
| | | Aggregated # of I/O read ops            : 1 
| | | Weighted harmonic mean                  : 10.412 MB/s
| | | Harmonic mean                           : 10.412 MB/s
| | | Arithmetic mean                         : 10.412 MB/s
| | | Median                                  : 10.412 MB/s
| | | Max                                     : 10.412 MB/s
| | | Min                                     : 10.412 MB/s
| | | Harmonic mean x ranks                   : 10.412 MB/s
| | | Aritmetic mean x ranks                  : 10.412 MB/s
| | |  ___Average___
| | | | Weighted harmonic mean                : 10.412 MB/s
| | | | Harmonic mean                         : 10.412 MB/s
| | | | Arithmetic mean                       : 10.412 MB/s
| | | | Median                                : 10.412 MB/s
| | | | Max                                   : 10.412 MB/s
| | | | Min                                   : 10.412 MB/s
| | | | Harmonic mean x ranks                 : 10.412 MB/s
| | | | Aritmetic mean x ranks                : 10.412 MB/s
|
|  _________Sync_Read_________
| | Max number of ranks        : 0   
| | Total read bytes           : 0.00 B
| | Max read bytes per rank    : 0.00 B
| | Max transfersize           : 0.00 B
| |
| | Max # of I/O read phases per rank         : 0 
| | Aggregated # of I/O read phases           : 0 
| | Max # of I/O read ops in phase            : 0 
| | Max # of I/O overlaping phases            : 0 
| | Max # of I/O read ops per rank            : 0 
| | Aggregated # of I/O read ops              : 0 
| | Weighted harmonic mean                    : 0.000 MB/s
| | Harmonic mean                             : 0.000 MB/s
| | Arithmetic mean                           : 0.000 MB/s
| | Median                                    : 0.000 MB/s
| | Max                                       : 0.000 MB/s
| | Min                                       : 0.000 MB/s
| | Harmonic mean x ranks                     : 0.000 MB/s
| | Aritmetic mean x ranks                    : 0.000 MB/s



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
| | Max number of ranks        : 0   
| | Total written bytes        : 0.00 B
| | Max written bytes per rank : 0.00 B
| | Max transfersize           : 0.00 B
| |
| | Max # of I/O write phases per rank        : 0 
| | Aggregated # of I/O write phases          : 0 
| | Max # of I/O write ops in phase           : 0 
| | Max # of I/O overlaping phases            : 0 
| | Max # of I/O write ops per rank           : 0 
| | Aggregated # of I/O write ops             : 0 
| | Weighted harmonic mean                    : 0.000 MB/s
| | Harmonic mean                             : 0.000 MB/s
| | Arithmetic mean                           : 0.000 MB/s
| | Median                                    : 0.000 MB/s
| | Max                                       : 0.000 MB/s
| | Min                                       : 0.000 MB/s
| | Harmonic mean x ranks                     : 0.000 MB/s
| | Aritmetic mean x ranks                    : 0.000 MB/s

ellapsed time (rank 0)             = 0.001406 sec
|-> application time               = 0.001309 sec       -> from ellapsed time 93.10 %
|-> overhead during runtime        = 0.000076 sec       -> from ellapsed time 5.41 %
'-> overhead post runtime          = 0.000021 sec       -> from ellapsed time 1.49 %

total run time                     = 0.001406 sec
|-> lib overhead time              = 0.000097 sec       -> from run time 6.87 %
|     |-> during runtime           = 0.000076 sec       -> from overhead 78.69 %
|     '-> post runtime             = 0.000021 sec       -> from overhead 21.31 %
|
'-> app time                       = 0.001309 sec       -> from run time 93.13 %
    |-> total compute/comm. time   = 0.001293 sec       -> from app time 98.72 %
    '-> total I/O time             = 0.000017 sec       -> from app time 1.28 %
        |-> sync read time         = 0.000000 sec       -> from app time 0.00 %
        |-> sync write time        = 0.000000 sec       -> from app time 0.00 %
        |
        |-> req. async read time   = 0.000117 sec       -> from app time 8.90 %
        |-> async read time        = 0.000133 sec       -> from app time 10.18 %
        |    |-> approx. wait time = 0.000017 sec       -> from app time 1.28 %
        |    '-> real wait time    = 0.000017 sec       -> from app time 1.28 %
        |
        |-> req. async write time  = 0.000000 sec       -> from app time 0.00 %
        '-> async write time       = 0.000000 sec       -> from app time 0.00 %
             |-> approx. wait time = 0.000000 sec       -> from app time 0.00 %
             '-> real wait time    = 0.000000 sec       -> from app time 0.00 %
```