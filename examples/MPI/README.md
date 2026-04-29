# MPI Example

This project demonstrates file operations within an MPI context. The MPI example program initializes the MPI environment, opens a file, reads its content into a buffer, and then closes the file. This example is designed to verify the correct execution of file operations in a parallel computing environment.

## Overview

The `MPI_example.cxx` test performs the following operations:
1. **MPI Initialization**: Sets up the MPI environment (`MPI_Init`).
2. **File Open**: Opens `testfile.txt` for reading and writing, creating it if it does not exist.
3. **File Read**: Reads data from the file into a buffer and prints the number of bytes read.
4. **File Close**: Closes the file descriptor.
5. **MPI Finalization**: Cleans up the MPI environment (`MPI_Finalize`).

This example ensures that file operations are correctly handled in an MPI context.

## Compilation

To compile this example, you need an MPI C++ compiler (like `mpicxx`).

```bash
mpicxx -o MPI_example MPI_example.cxx
```

*Note: Ensure that your TMIO library is linked correctly if required by your specific build environment (e.g., via `LD_PRELOAD` at runtime or direct linking during compilation).*

## Running the Example

This example is intended to be run using `mpirun` or `mpiexec`.

### 1. Create a dummy file
Since the test reads from `testfile.txt`, create this file with some content before running:

```bash
echo "Hello MPI World" > testfile.txt
```

### 2. Execute the binary
Run the compiled binary using `mpirun`.

```bash
# Run with 1 MPI process
mpirun -n 1 ./MPI_example
```

**Using with TMIO Interception:**
To test TMIO's functionality, you usually preload the TMIO library.

```bash
mpirun -n 1 -x LD_PRELOAD=/path/to/libtmio.so ./MPI_example
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

File opened successfully.
Read 0 bytes: 
File closed successfully.


Summary
***************************
I/O library: MPI
Ranks: 4 

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
| | Max number of ranks        : 4   
| | Total read bytes           : 396.00 B
| | Max read bytes per rank    : 99.00 B
| | Max transfersize           : 99.00 B
| |
| | Max # of I/O read phases per rank         : 1 
| | Aggregated # of I/O read phases           : 4 
| | Max # of I/O read ops in phase            : 1 
| | Max # of I/O overlaping phases            : 0 
| | Max # of I/O read ops per rank            : 1 
| | Aggregated # of I/O read ops              : 4 
| | Weighted harmonic mean                    : 2.318 MB/s
| | Harmonic mean                             : 2.318 MB/s
| | Arithmetic mean                           : 2.370 MB/s
| | Median                                    : 2.159 MB/s
| | Max                                       : 3.034 MB/s
| | Min                                       : 2.128 MB/s
| | Harmonic mean x ranks                     : 9.271 MB/s
| | Aritmetic mean x ranks                    : 9.480 MB/s



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

ellapsed time (rank 0)             = 0.009659 sec
|-> application time               = 0.009541 sec       -> from ellapsed time 98.78 %
|-> overhead during runtime        = 0.000021 sec       -> from ellapsed time 0.22 %
'-> overhead post runtime          = 0.000097 sec       -> from ellapsed time 1.01 %

total run time                     = 0.038445 sec
|-> lib overhead time              = 0.000361 sec       -> from run time 0.94 %
|     |-> during runtime           = 0.000093 sec       -> from overhead 25.64 %
|     '-> post runtime             = 0.000269 sec       -> from overhead 74.36 %
|
'-> app time                       = 0.038084 sec       -> from run time 99.06 %
    |-> total compute/comm. time   = 0.037913 sec       -> from app time 99.55 %
    '-> total I/O time             = 0.000171 sec       -> from app time 0.45 %
        |-> sync read time         = 0.000171 sec       -> from app time 0.45 %
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
```