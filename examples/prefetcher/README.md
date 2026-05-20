# Prefetch example
This simple example demonstrates the usage of the prefetcher.

## Running the Example
Simply run the following command inside this example folder:
```
make run
```
## Expected output
The resulting console output should look similar to the following:
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

Initializing prefetcher with frequency 0.333000, max cache size 0.01 Mb, max file size 0.01 Mb.
Prefetcher > Prefetch 80 b with offset 0
Prefetcher > Retrieving prefetch 20 b with offset 0
Prefetcher > Missed 0 b of 80 b
Prefetcher > Prefetch 80 b with offset 0
Prefetcher > Retrieving prefetch 20 b with offset 0
Prefetcher > Missed 0 b of 80 b
Prefetcher > Prefetch 80 b with offset 0
Prefetcher > Retrieving prefetch 20 b with offset 0
Prefetcher > Missed 0 b of 80 b
Prefetcher > Prefetch 80 b with offset 0
Prefetcher > Retrieving prefetch 20 b with offset 0
Prefetcher > Missed 0 b of 80 b
Prefetcher > Prefetch 80 b with offset 0
Prefetcher > Retrieving prefetch 20 b with offset 0
Prefetcher > Missed 0 b of 80 b
Prefetcher > Prefetch 80 b with offset 0
Prefetcher > Retrieving prefetch 20 b with offset 0
Prefetcher > Missed 0 b of 80 b
Prefetcher > Prefetch 80 b with offset 0
Prefetcher > Retrieving prefetch 20 b with offset 0
Prefetcher > Missed 0 b of 80 b
Prefetcher > Prefetch 80 b with offset 0
Prefetcher > Retrieving prefetch 20 b with offset 0
Prefetcher > Missed 0 b of 80 b
Prefetcher > Prefetch 80 b with offset 0
Prefetcher > Retrieving prefetch 20 b with offset 0
Prefetcher > Missed 0 b of 80 b
Prefetcher > Prefetch 80 b with offset 0
Prefetcher > Retrieving prefetch 20 b with offset 0
Prefetcher > Missed 0 b of 80 b
Prefetcher > Prefetch 80 b with offset 0
Prefetcher > Retrieving prefetch 20 b with offset 0
Prefetcher > Missed 0 b of 80 b
Prefetcher > Prefetch 80 b with offset 0
Prefetcher > Retrieving prefetch 20 b with offset 0
Prefetcher > Missed 0 b of 80 b
Prefetcher > Prefetch 80 b with offset 0
Prefetcher > Retrieving prefetch 20 b with offset 0
Prefetcher > Missed 0 b of 80 b
Prefetcher > Prefetch 80 b with offset 0
Prefetcher > Retrieving prefetch 20 b with offset 0
Prefetcher > Missed 0 b of 80 b
Prefetcher > Prefetch 80 b with offset 0
Prefetcher > Retrieving prefetch 20 b with offset 0
Prefetcher > Missed 0 b of 80 b
Prefetcher > Prefetch 80 b with offset 0
Prefetcher > Retrieving prefetch 20 b with offset 0
Prefetcher > Missed 0 b of 80 b
Prefetcher > Prefetch 80 b with offset 0
Prefetcher > Retrieving prefetch 20 b with offset 0
Prefetcher > Missed 0 b of 80 b
Prefetcher > Prefetch 80 b with offset 0
Prefetcher > Retrieving prefetch 20 b with offset 0
Prefetcher > Missed 0 b of 80 b
Prefetcher > Prefetch 80 b with offset 0
Prefetcher > Retrieving prefetch 20 b with offset 0
Prefetcher > Missed 0 b of 80 b
Prefetcher > Prefetch 80 b with offset 0
Prefetcher > Retrieving prefetch 20 b with offset 0
Prefetcher > Missed 0 b of 80 b
Prefetcher > Prefetch 80 b with offset 0
Prefetcher > Retrieving prefetch 20 b with offset 0
Prefetcher > Missed 0 b of 80 b
Prefetcher > Prefetch 80 b with offset 0
Prefetcher > Retrieving prefetch 20 b with offset 0
Prefetcher > Missed 0 b of 80 b
Prefetcher > Prefetch 80 b with offset 0
Prefetcher > Retrieving prefetch 20 b with offset 0
Prefetcher > Missed 0 b of 80 b
Prefetcher > Prefetch 80 b with offset 0
Prefetcher > Retrieving prefetch 20 b with offset 0
Prefetcher > Missed 0 b of 80 b
Prefetcher > Prefetch 80 b with offset 0
Prefetcher > Retrieving prefetch 20 b with offset 0
Prefetcher > Missed 0 b of 80 b
Prefetcher > Prefetch 80 b with offset 0
Prefetcher > Retrieving prefetch 20 b with offset 0
Prefetcher > Missed 0 b of 80 b
Prefetcher > Prefetch 80 b with offset 0
Prefetcher > Retrieving prefetch 20 b with offset 0
Prefetcher > Missed 0 b of 80 b
Aggregated prefetching overhead: 0.60s, 90.61 of runtime
Average prefetching overhead:  0.60s, 90.61 of runtime
Aggregated prefetching missed bytes: 0
Average prefetching missed bytes: 0
Aggregated total missed bytes: 240
Average total missed bytes: 240


Summary
***************************
I/O library: MPI
Ranks: 1 

 _________________Read_________________
|
| _________Async_Read_________
| | Max number of ranks        : 1   
| | Total read bytes           : 2.16 KB
| | Max read bytes per rank    : 2.16 KB
| | Max transfersize           : 80.00 B
| |
| |  ___Throughput___
| | | Max # of I/O read phases per rank       : 27 
| | | Aggregated # of I/O read phases         : 27 
| | | Max # of I/O read ops in phase          : 1 
| | | Max # of I/O overlaping phases          : 0 
| | | Max # of I/O read ops per rank          : 27 
| | | Aggregated # of I/O read ops            : 27 
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
| | | Max # of I/O read phases per rank       : 27 
| | | Aggregated # of I/O read phases         : 27 
| | | Max # of I/O read ops in phase          : 1 
| | | Max # of I/O overlaping phases          : 0 
| | | Max # of I/O read ops per rank          : 27 
| | | Aggregated # of I/O read ops            : 27 
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
| | Total read bytes           : 240.00 B
| | Max read bytes per rank    : 240.00 B
| | Max transfersize           : 80.00 B
| |
| | Max # of I/O read phases per rank         : 3 
| | Aggregated # of I/O read phases           : 3 
| | Max # of I/O read ops in phase            : 1 
| | Max # of I/O overlaping phases            : 0 
| | Max # of I/O read ops per rank            : 3 
| | Aggregated # of I/O read ops              : 3 
| | Weighted harmonic mean                    : 1.789 MB/s
| | Harmonic mean                             : 1.789 MB/s
| | Arithmetic mean                           : 1.801 MB/s
| | Median                                    : 1.751 MB/s
| | Max                                       : 2.003 MB/s
| | Min                                       : 1.649 MB/s
| | Harmonic mean x ranks                     : 1.789 MB/s
| | Aritmetic mean x ranks                    : 1.801 MB/s



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
| | Total written bytes        : 80.00 B
| | Max written bytes per rank : 80.00 B
| | Max transfersize           : 80.00 B
| |
| | Max # of I/O write phases per rank        : 1 
| | Aggregated # of I/O write phases          : 1 
| | Max # of I/O write ops in phase           : 1 
| | Max # of I/O overlaping phases            : 0 
| | Max # of I/O write ops per rank           : 1 
| | Aggregated # of I/O write ops             : 1 
| | Weighted harmonic mean                    : 1.202 MB/s
| | Harmonic mean                             : 1.202 MB/s
| | Arithmetic mean                           : 1.202 MB/s
| | Median                                    : 1.202 MB/s
| | Max                                       : 1.202 MB/s
| | Min                                       : 1.202 MB/s
| | Harmonic mean x ranks                     : 1.202 MB/s
| | Aritmetic mean x ranks                    : 1.202 MB/s

ellapsed time (rank 0)             = 90.606192 sec
|-> application time               = 90.605195 sec      -> from ellapsed time 100.00 %
|-> overhead during runtime        = 0.000571 sec       -> from ellapsed time 0.00 %
'-> overhead post runtime          = 0.000426 sec       -> from ellapsed time 0.00 %

total run time                     = 90.606184 sec
|-> lib overhead time              = 0.000990 sec       -> from run time 0.00 %
|     |-> during runtime           = 0.000571 sec       -> from overhead 57.68 %
|     '-> post runtime             = 0.000419 sec       -> from overhead 42.32 %
|
'-> app time                       = 90.605195 sec      -> from run time 100.00 %
    |-> total compute/comm. time   = 90.604461 sec      -> from app time 100.00 %
    '-> total I/O time             = 0.000734 sec       -> from app time 0.00 %
        |-> sync read time         = 0.000134 sec       -> from app time 0.00 %
        |-> sync write time        = 0.000067 sec       -> from app time 0.00 %
        |
        |-> req. async read time   = 64.785858 sec      -> from app time 71.50 %
        |-> async read time        = 64.786391 sec      -> from app time 71.50 %
        |    |-> approx. wait time = 0.000534 sec       -> from app time 0.00 %
        |    '-> real wait time    = 0.000534 sec       -> from app time 0.00 %
        |
        |-> req. async write time  = 0.000000 sec       -> from app time 0.00 %
        '-> async write time       = 0.000000 sec       -> from app time 0.00 %
             |-> approx. wait time = 0.000000 sec       -> from app time 0.00 %
             '-> real wait time    = 0.000000 sec       -> from app time 0.00 %

```
