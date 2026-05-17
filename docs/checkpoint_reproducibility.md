Data can be found here: [DOI](https://doi.org/10.5281/zenodo.20122316)

This document describes how to reproduce the results achieved within the thesis "Adaptive Checkpointing With Bandwidth-Aware Optimization in HPC" 
# Extracting the trace data
Content inside `Data.tar` is structured in the following way:
```
Data
├── VELOC_BANDWIDTH
│     └── ... // Contains traces for VELOC checkpoint interval by bandwidth      
├── WACOMM_FREQUENCY
│     └── ... // Contains traces for frequency as I/O phase substitute      
├── HACC_IO_RESULTS    
│     └── ... // Contains traces for fine-grained bandwidth limit approaches      
├── HACC_IO_PREFETCH
│     └── ... // Contains traces for I/O frequency based prefetching
├── HACC_IO_GROWING_PHASES
│     └── ... // Contains traces for fine-grained bandwidth limit 
│             // approaches with growing bandwidth requirements 
├── HACC_IO_VARYING_PHASES
│     └── ... // Contains traces for fine-grained bandwidth limit 
│             // approaches with varying bandwidth requirements during iterations  
├── HETDIS_VELOC
│     └── ... // Contains traces for asynchronous checkpoint evaluation
└── HEATDIS_ITERATIONS
      └── ... // Contains traces for asynchronous slowdown factor examination
```

This command can be used to extract the data:
```
tar -xf Data.tar
```
# Figures
All graphes within the evaluation chapter of the thesis can be generated utilizing the trace data inside `Data.tar`.
## Prerequisites
### Download and install FTIO
Most figures in the evaluation chapter have been generated with the `ioplot` tool packaged with FTIO. To install FTIO in the current folder execute the following commands:
```sh
git clone https://github.com/tuda-parallel/FTIO.git
cd FTIO
make install
```
After installation source the python virtual environment made available by FTIO. For more information on installing FTIO visit the [FTIO github repository](https://github.com/tuda-parallel/FTIO).
### Plotting overhead
Overhead graphes utilize the following template file `plot_overhead.py`. The example measurements inside the file should be replaced with the respective measurements for the plotted overhead.
```python
import plotly.graph_objects as go
import numpy as np
def main():
    x_coords = [96, 384, 768, 1536, 3072, 6144]

    measurements = [
            [0.074803, 0.090228, 0.074937], # Measurements for 96 ranks
            [0.088876, 0.126739, 0.050842], # Measurements for 384 ranks
            [0.079192, 0.047360, 0.070189], # Measurements for 768 ranks
            [0.088253, 0.062377, 0.133458], # Measurements for 1536 ranks
            [0.071425, 0.052577, 0.108888], # Measurements for 3072 ranks
            [0.063913, 0.069863, 0.063913], # Meansuremnts for 6144 ranks
    ]

    means = [np.mean(m) for m in measurements]
    stds  = [np.std(m, ddof=1) for m in measurements]

    scatter = go.Scatter(
        x=x_coords,
        y=means,
        mode='lines+markers',
        marker=dict(
            symbol="square",
            line=dict(width=1, color="DarkSlateGrey"),
        ),  
        error_y=dict(
            type='data',
            array=stds,
            visible=True
        ),
        textposition='top center',
        name='Overhead'
    )

    fig = go.Figure(data=[scatter])

    fig.update_layout(
        title='Title',
        xaxis_title='Ranks',
        yaxis_title='Overhead in s',
        plot_bgcolor='white',
        width=600,
        height=400,
    )

    fig.update_xaxes(
        type="log",
        tickmode="array",
        tickvals=x_coords,
        ticktext=[str(p) for p in x_coords],    
        ticks="outside",
        ticklen=10,
        showgrid=True,
        mirror=True,
        showline=True,
        linecolor="black",
        gridcolor="lightgrey",
        minor=None,
    )

    fig.update_yaxes(
        ticks="outside",
        ticklen=10,
        showgrid=True,
        mirror=True,
        showline=True,
        linecolor="black",
        gridcolor="lightgrey",
        minor_ticks="outside",
        minor=dict(ticklen=2),
        rangemode="tozero",
    )
    fig.show()

if __name__ == "__main__":
    main()
```
To utilize this file install numpy and plotly.
```sh
pip install numpy plotly
```
And execute the file with:
```
python plot_overhead.py
```
## Fine-Grained Bandwidth Limit
This section utilizes traces within the folder HACC_IO_RESULTS, HACC_IO_GROWING_PHASES and HACC_IO_VARYING_PHASES. The subfolders each represent one of the applied bandwidth limiting strategies within the thesis:

* always represents *direct phase*
* nolimit represents *no limit*
* request_size represents *request size*
* file_history represents *file history*
* file_scale represents *file scale*

Additionally traces for *up-only* in folder uponly and *adaptive* in folder adaptive can be found in `Data.tar`.
### Bandwidth Behavior
Figures within this section can be recreated with the traces inside the *run_median* folder present inside the strategy sub-directories. From 
```
cd HACC_IO_RESULTS/strategy_folder/run_median
ioplot 1536_MPI.json
cd io_results
```
This generates multiple html files containing utilized graphes. Figure 5.2 can be found inside `write_async.html`. Open this file with your preferred html viewer. Alternatively, a folder named 1536 on the same level as run_median contains the already generated plots utilized for the thesis.

To generate Figure 5.3 call these commands inside the respective strategy subfolder:
```
cd run_median
ioplot 96_MPI.json 385_MPI.json 768_MPI.json 1536_MPI.json 3072_MPI.json 6144_MPI.json
cd io_results
```
The graph utilized for Figure 5.3 can be found inside `time.html`. A folder named scale on the same level as run_median contains the generated plots utilized for Figure 5.3.
### Bandwidth Behavior with Growing Phases
Figure 5.4 is reproducible by executing these commands in the Data directory. Here only traces for *direct phase*, *request size*, *file history* and *file scale* were made.
```
cd HACC_IO_GROWING_PHASES/strategy_folder/
ioplot 768_MPI.json
cd io_results
```
The graphs utilized for Figure 5.4 can then be found inside `write_async.html`. The strategy folders also contain already generated plots inside the preexisting io_results folders.
### Bandwidth Behavior with Varying Phases
Figure 5.5 is reproducible by executing these commands in the Data directory. Here only traces for *direct phase*, *request size*, *file history* and *file scale* were made.
```
cd HACC_IO_VARYING_PHASES/strategy_folder/
ioplot 768_MPI.json
cd io_results
```
The graphs utilized for Figure 5.5 can then be found inside `write_async.html`. The strategy folders also contain already generated plots inside the preexisting io_results folders.
### Bandwidth Limit Overhead
For Figure 5.6 data from HACC_IO_RESULTS was utilized. Inside each strategy directory folders called run1 to run3 can be found. These contain multiple textfiles with `.txt` ending containing TMIO's run summaries inside. Information about overhead during runtime can be found in the following format:
```
|-> overhead during runtime        = 0.002490 sec       -> from ellapsed time 0.01 %
```
This information was collected for all runs and different rank counts and can be inserted into the python file introduced [above](#plotting-overhead) to plot Figure 5.6.
## Phase-Based Prediction
Traces within this section can be found inside WACOMM_FREQUENCY and HACC_IO_PREFETCHING.
### Bandwidth Requirement Prediction
WACOMM_FREQUENCY is divided into three subdirectories:
* _always_ contains traces for the strategy *direct phase*
* *get_freq* contains traces for *no_limit*
* *with_freq* contains traces for *phase_frequency*

To generate Figures 5.7 and 5.8 execute the following commands for each subdirectory:
```
cd WACOMM_FREQUENCY/strategy_folder/
ioplot 1536_MPI.json
cd io_result
```
The graphes utilized for Figure 5.7 can now be found inside `write_async.html`. Figure 5.8 can be found within `time.html`.
### Prefetching with Phase Prediction
HACC_IO_PREFETCH is divided into three subdirectories:
* _limit_
* *nolimit*
* *get_freq*

To generate Figure 5.10 execute the following commands for the subdirectories *limit* and *get_freq*:
```
cd HACC_IO_PREFETCH/subdirectory/
ioplot 1536_MPI.json
cd io_result
```
Both subfigures for synchronous transactions can be found within `read_sync.html`. The subfigure for asynchronous transaction is within the subdirectory `limit/io_results/read_async.html`.

Figure 5.11 can be built with the overhead script from [above](#plotting-overhead). Overhead information can gathered from the subfolder *nolimit*. Within this subfolder are multiple run folders run1 to run3. Inside these are folders for the different rank counts. Inside are `HACC_ASYNC_IO.out` files, the following information can be found:
```
Average prefetching overhead: 0.194600.2s,
```
Disregard the .2s and insert the overhead information into the [overhead script](#plotting-overhead) to generate Figure 5.11.

To plot figure 5.12, modify the [overhead script](#plotting-overhead) by replacing measurements with the following:
```python
measurements_nolimit = [
    [9.048162, 9.038754, 11.168678],        # Measurements for 96 ranks
    [15.148793, 15.836647, 14.955177],      # Measurements for 384 ranks
    [25.19743, 23.655919, 23.449984],       # Measurements for 768 ranks
    [42.402541, 35.990965, 42.555818],      # Measurements for 1536 ranks
    [68.693763, 67.633996, 69.549229],      # Measurements for 3072 ranks
    [135.667605, 127.723278, 130.182473]]   # Measurements for 6144 ranks

measurements_get_freq = [
    [12.400412],     # Measurement for 96 ranks
    [19.822684],     # Measurement for 384 ranks
    [31.961761],     # Measurement for 768 ranks
    [47.458255],     # Measurement for 1536 ranks
    [87.459588],     # Measurement for 3072 ranks
    [162.199193]]    # Measurement for 6144 ranks

measurements = [
    [get_freq[0] / val for val in nolimit] 
    for nolimit, get_freq in zip(measurements_nolimit, measurements_get_freq)
]
```
Values for measurements_nolimit can be retrieved from the `nolimit` subfolder. Within this subfolder are multiple run folders run1 to run3. Inside these are folders for the different rank counts. Inside are `HACC_ASYNC_IO.out` files, the following information can be found:
```
|-> application time               = 127.723278 sec     -> from ellapsed time 97.36 %
```
Values for measurements_nolimit can be retrieved from the `get_freq` subfolder. Within this subfolder are `NNN_MPI.txt` files. These contain application time in the following way:
```
|-> application time               = 47.458255 sec      -> from ellapsed time 97.40 %
```
## Checkpointing
This section utilizes traces from within the folders HEATDIS_VELOC, HEATDIS_ITERATIONS and VELOC_MEMORY.
### Bandwidth Behavior
To generate Figure 5.13 execute the following commands:
```
cd HEATDIS_VELOC/mpi/run1
ioplot 1535_MPI.json
cd io_result
```
Subfigure 5.13 (a) can be found within `write_async.html`.
```
cd HEATDIS_VELOC/sync/run1
ioplot 1535_MPI.json
cd io_result
```
Subfigure 5.13 (b) can be found within `write_sync.html`.
### Checkpointing Time
For Figure 5.14, the [overhead script](#plotting-overhead) can be modified to allow multiple measurements to be visualized by adding:
```python
...
measurements_sync = [
    [0.917224,0.925756,0.908066],
    [1.641060,2.214320,2.146630],
    [2.815060,3.764730,3.674970],
    [6.376020,6.713270,6.928930],
    [15.214900,17.369200,14.369500],
]

means_sync = [np.mean(m) for m in measurements_sync]
stds_sync  = [np.std(m, ddof=1) for m in measurements_sync]

scatter_async = go.Scatter(
    x=x_coords,
    y=means_sync,
    mode='lines+markers',
    marker=dict(
        symbol="square",
        line=dict(width=1, color="DarkSlateGrey"),
    ),  
    error_y=dict(
        type='data',
        array=stds_sync,
        visible=True
    ),
    textposition='top center',
    name='Asynchronous, No Limit'
)
...
fig.add_trace(scatter_sync)
...
```
Figure data can be found inside HEATDIS_VELOC. Navigate to `HEATDIS_VELOC/async/`. Inside are traces for multiple runs. For each of these runs, text files for multiple rank counts exist. These contain the required information in the following line: 
```
Average checkpoint time: 0.0945153s
```
Synchronous data can be found inside `HEATDIS_VELOC/sync/`.

Runtime for Figure 5.15 can be extracted from the various subfolders inside HEATDIS_VELOC. The textfiles at the lowest directory level contain the data in this line:
```
Execution finished in 175.834178 seconds.
```
### Slowdown Factor
Simulation iterations can be retrieved from HEATDIS_ITERATIONS. The subfolders inside represent the different interfaces. For each interface multiple overlap factors were measured. To retrieve data visualized in Figure 5.16 search for the following lines inside the `VELOC.out` files:
```
Checkpoint conducted at iteration 2479
```
Subtract the first instance of this line from the last one to remove the first checkpoint free 60s. For the no checkpoint measurements navigate inside `nocheck/20`. The required data can be found in two different lines:
```
60s Iteration 493
...
Execution ended at iteration 2462
```
Subtract the 60s Iteration count from the overall execution iterations to ensure comparability between results. This code-snippet can be used to print the iteration data for disabled checkpointing. 
```python
go.Scatter(
    x=x2_coords,
    y=means_nocheck,
    mode="lines",
    name="No checkpoint",
    line=dict(dash="dash")
)
```
For Figure 5.17 this code-snippet can be utilized to calculate the slowdown factor from the iteration measurements from Figure 5.16:
```python
for x in range(len(x_coords)):
    for i, n in enumerate(measurements_posix[x]):
        t = 1/x_coords[x]
        waste=(1-(n/(np.mean(measurements_nocheck[0]))))
        measurements_posix[x][i] = 1 - (waste * t)
```

This calculation can be modified to calculate Figure 5.18:
```python
Tphase = 60
TCsync = 6.6726

for x in range(len(x_coords)):
    for i, n in enumerate(measurements_posix[x]):
        t = Tphase/TCsync
        waste=(1-(n/(np.mean(measurements_nocheck[0]))))
        measurements_posix[x][i] = 1 - (waste * t)
```
### Checkpointing by Bandwidth
Figure 5.19 is reproducible by executing these commands in the Data directory.
```
cd VELOC_BANDWIDTH/
ioplot 96_MPI.json
cd io_results
```
The graphs utilized for Figure 5.19 can then be found inside `write_async.html`.
### Main Memory Usage
The trace-data for Figure 5.20 can be extracted from rank0_memory.txt with this code snippet.
```python
with open('rank0_memory.txt', 'r') as file:
    for line in file:
        if not line.startswith('#') and line.strip():
            parts = line.split()
            data.append((float(parts[0]), float(parts[2])))

data.sort(key=lambda x: x[0]) # This added sort is reqiured due to multithreading mixing up time data

times = [d[0] for d in data]
memory = [d[1] for d in data]
```
# Traces
Within this section generation of the traces within `Data.tar` is explained.
## Prerequisites
### Download and install BW-Limit MPI
To install the MPI for bandwidth limitation execute the following commands in the directory you want to install MPI in: 
```sh
git clone https://github.com/nGreen27/MPICH-IOBandwidth-Limitation.git
cd MPICH-IOBandwidth-Limitation/
cp compilar.sh mpich-4.0.3_BW-limit/
mkdir mpi-bin
mpich-4.0.3_BW-limit/compilar.sh $PWD/mpi-bin
```
### Download and install TMIO
For most traces generated for the thesis installation of TMIO is required. To install TMIO in the current directory execute the following commands:
```sh
git clone https://github.com/tuda-parallel/TMIO.git
cd build
make library CXX_DEBUG+="-DBW_LIMIT" MPICXX=/path/to/mpi-bin/bin/mpicxx
```
This install utilizes the bandwidth limiting capabilities offered by the custom MPI installed [above](#download-and-install-bw-limit-mpi). Download and install the BW-Limit MPI. To install without bandwidth limiting simply replace the make command with the following:
```
make library CXX_DEBUG+="-DCUSTOM_MPI" MPICXX=/path/to/mpi-bin/bin/mpicxx
```
For all measurements within the thesis, only MPI tracing is activated. Modify the file `TMIO/include/ioflags.h` in the following way:
```C++
...
#ifndef ENABLE_MPI_TRACE
#define ENABLE_MPI_TRACE 1 // Edit: set to 1 to enable tracing
// 0: disable tracing
// 1: enable tracing
#endif

#ifndef ENABLE_LIBC_TRACE
#define ENABLE_LIBC_TRACE 0 //Edit: set to 0 to disable tracing
// 0: disable tracing
// 1: enable tracing
#endif

#ifndef ENABLE_IOURING_TRACE
#define ENABLE_IOURING_TRACE 0 //Edit: set to 0 to disable tracing 
// 0: disable tracing
// 1: enable tracing
#endif
...
```
### Download and install FTIO
IO frequency information is required to utilize the prefetcher. FTIO can analyze traces from TMIO to generate a frequency. To install FTIO in the current folder execute the following commands:
```sh
git clone https://github.com/tuda-parallel/FTIO.git
cd FTIO
make install
```
After installation source the python virtual environment made available by FTIO. For more information on installing FTIO visit the [FTIO github repository](https://github.com/tuda-parallel/FTIO).
## Heatdis
This subsection details the generation of all traces associated with the heatdis simulation.
### Download and install VELOC
To install VELOC with TMIO, setup the environment variable `TMIO_ROOT` to reference the base directory of TMIO:
```sh
export TMIO_ROOT=/path/to/TMIO
```
For a successfull install of all of VELOC's prerequisites make sure to include the BW-Limit MPI bin into your path and add the lib into the LD libraries. Additionally installed MPI libraries may cause conflicts.
```sh
export PATH=/path/to/mpi-bin/bin:$PATH
export LD_LIBRARY_PATH=/path/to/mpi-bin/lib:$LD_LIBRARY_PATH
```
Download VELOC from github:
```sh
git clone https://github.com/nGreen27/VELOC.git
cd VELOC
```
Either use the bootstrap script offered by VELOC:
```sh
./bootstrap.sh
```
Or install the required python packages manually:
```sh
pip install bs4 wget
```
Finally, install VELOC:
```sh
mkdir install
python auto-install.py install/
```
### Replicate results in VELOC_BANDWIDTH
To replicate results for checkpointing by bandwidth create a config file `heatdis.cfg` with the following content:
```sh
scratch = /path/to/store/node_local/checkpoints/
persistent = /path/to/store/persistent/checkpoints/
meta = /path/to/store/meta/checkpoints/
max_versions = 2
scratch_versions = 1
mode = sync
ec_interval=-1
persistent_interval=-1
chksum = false 
checkpoint_strategy = bandwidth
checkpoint_bandwidth = 500
flatten_flush = false
flatten_local = true
local_mpi = true
```
The following file `sbatch.sh` can be utilized, to run the simulation on the Lichtenberg cluster. The paths for scratch, persistent and scratch storage have to be the same as inside the `heatdis.cfg` file.
```sh
#!/bin/bash
#SBATCH -J VELOC
#SBATCH -e %x.err
#SBATCH -o %x.out
#SBATCH -C i01
#SBATCH -n 96
#SBATCH -c 1
#SBATCH -N 1
#SBATCH -A projectXXXX
#SBATCH --mem-per-cpu=3800   
#SBATCH -t 00:15:00

mkdir -p /work/scratch/user_name/scratch/
mkdir -p /work/scratch/user_name/persistent/
mkdir -p /work/scratch/user_name/meta/

/path/to/mpi-bin/bin/mpirun /path/to/VELOC/build/test/heatdis_iter 100 /path/to/heatdis.cfg

rm -r /work/scratch/user_name/scratch/
rm -r /work/scratch/user_name/persistent/
rm -r /work/scratch/user_name/meta/

EXITSTATUS=$?
echo “Job $SLURM_JOB_ID has finished at $(date).”
exit $EXITSTATUS

```
### Replicate results in HEATDIS_VELOC

This section explains how to replicate the results for the general checkpointing comparisons. The `sbatch.sh` file from the [above](#replicate-results-in-veloc_bandwidth) section can be applied here. Additionally, create a config file `heatdis.cfg` with the following content:
```sh
scratch = /path/to/store/node_local/checkpoints/
persistent = /path/to/store/persistent/checkpoints/
meta = /path/to/store/meta/checkpoints/
max_versions = 2
scratch_versions = 1
mode = sync
ec_interval=-1
persistent_interval=-1
chksum = false
checkpoint_strategy = factor
local_concurrency = 0.7
flush_concurrency = 1.0
checkpoint_interval = 60
flatten_flush = false
flatten_local = true
local_mpi = true
```
This file is valid for the results achieved in the mpi folder containing traces for asynchronous traces using the MPI interface. For the results in the posix folder containing results for the POSIX interface set `local_mpi = false` and `flatten_local = true`. For the results in the sync folder for synchronous checkpoints set `local_mpi = true` and `flatten_local = false`. 

Results within the async folder can be achieved by compiling VELOC without TMIO. This requires `unset TMIO_ROOT` to remove the environmental variable and a recompile with `python auto-install.py install/` inside the VELOC root folder. Navigate to the `TMIO/build` folder and rebuild TMIO without bandwidth limit with the following command:
```
make library CXX_DEBUG+="-DCUSTOM_MPI" MPICXX=/path/to/mpi-bin/bin/mpicxx
```
Modify `sbatch.sh` to contain the line `export LD_PRELOAD=/path/to/TMIO/build/libtmio.so` under the `#SBATCH` directives. To get the results in the async folder, set set `local_mpi = true` and `flatten_local = true` in the `heatdis.cfg` file.

The traces are generated using different rank counts. Replace the number in `#SBATCH -n 96` with the desired ranks. Results from this thesis use `#SBATCH -N 1` to set the number of participating nodes to the minimum requirement. Consequently, node count equals the rank count divided by 96.
### Replicate results in HEATDIS_ITERATIONS
Modified versions of `heatdis_iter` can be used to generate the traces for the slowdown calculations. All measurements within this folder were made with `#SBATCH -n 1536` and `#SBATCH -N 16`. For the mpi and posix results replace `heatdis_iter` with `heatdis_slowdown` in the `sbatch.sh` file from [above](#replicate-results-in-veloc_bandwidth). This modification executes exactly 6 checkpoints. The `heatdis.cfg` file in the [above](#replicate-results-in-heatdis_veloc) section can be utilized to recreate the results here. Set `local_mpi = false` and `flatten_local = true` for the results in the posix folder and to `local_mpi = true` and `flatten_local = true` for the results in the mpi folder.

Traces have been generated with different `local_concurrency` factors applied. Each numbered folder inside the posix or mpi folder signifies a percentage of concurrent asynchronous checkpointing in regard to the overall checkpoint interval of 60s. To recreate results inside the 20 folder set `local_concurrency = 0.2` inside the `heatdis.cfg` file. For the contents of the 40 folder set `local_concurrency = 0.4` instead and so on.

Results contained in the nocheck folder were generated using the `heatdis_no_ckpt` application in the same folder as `heatdis_iter` and `heatdis_slowdown`. Simply replace `heatdis_iter` with `heatdis_no_ckpt` in the `sbatch.sh` file [above](#replicate-results-in-veloc_bandwidth) to execute on the Lichtenberg cluster. This application does not initiate checkpoints, therefore the settings for checkpointing inside `heatdis.cfg` do not matter and one of the files described [above](#replicate-results-in-heatdis_veloc) can be passed to this application.
### Replicate results in VELOC_MEMORY_USAGE
Measuring memory allocation requires the python package `psrecord`:
```
pip install psrecord
```
Instead of calling psrecord on all ranks, only rank 0's memory allocation is measured. The following script `mem.sh` only instruments rank 0:
```sh
#!/bin/bash
if [ "${PMI_RANK}" -eq 0 ]; then # Rank 0 start with psrecord
    "$@" &
    APP_PID=$!
    psrecord $APP_PID --log rank0_memory.txt --include-children
    wait $APP_PID
else # Other ranks start default
    exec "$@"
fi
```
The following `heatdis.cfg` file was applied to generate the traces inside the _async_ subfolder:
```sh
scratch = /path/to/store/node_local/checkpoints/
persistent = /path/to/store/persistent/checkpoints/
meta = /path/to/store/meta/checkpoints/
max_versions = 2
scratch_versions = 1
mode = sync
ec_interval=-1
persistent_interval=-1
chksum = false
checkpoint_strategy = factor
local_concurrency = 0.5
flush_concurrency = 1.0
checkpoint_interval = 60
flatten_flush = false
flatten_local = true
local_mpi = true
```
To receive the results inside the _sync_ folder set `flatten_local = false`

This `sbatch.sh` can be utilized on the Lichtenberg cluster to measure memory allocation. The script `mem.sh` must be in the same directory.
```sh
#!/bin/bash
#SBATCH -J VELOC
#SBATCH -e %x.err
#SBATCH -o %x.out
#SBATCH -C i01
#SBATCH -n 96
#SBATCH -c 1
#SBATCH -N 1
#SBATCH -A projectXXXX
#SBATCH --mem-per-cpu=3800   
#SBATCH -t 00:15:00

mkdir -p /work/scratch/user_name/scratch/
mkdir -p /work/scratch/user_name/persistent/
mkdir -p /work/scratch/user_name/meta/

mpirun ./mem.sh /path/to/VELOC/build/test/heatdis_iter 100 /path/to/heatdis.cfg

rm -r /work/scratch/user_name/scratch/
rm -r /work/scratch/user_name/persistent/
rm -r /work/scratch/user_name/meta/

EXITSTATUS=$?
echo “Job $SLURM_JOB_ID has finished at $(date).”
exit $EXITSTATUS
```
## HACC-IO
This section describes how to recreate results created with the HACC-IO simulation.
### Download and install HACC-IO
For the traces generated with HACC-IO slight modifications were applied to HACC-IO. Install the modified HACC-IO as follows:
```
git clone https://github.com/nGreen27/hacc-io.git
cd hacc-io
```
This modified version of HACC-IO contains three additional versions: `testHACC_Async_IO_grow.cxx`, `testHACC_Async_IO_read.cxx` and `testHACC_Async_IO_report.cxx`. To correctly compile the provided Benchmarks edit the `Makefile` to point to the required libraries installed in the [prerequisites](#prerequisites) section:

```make
...
## For bw limit
MODIFED_MPICXX = /path/to/mpi-bin/bin/mpicxx    // Edit
MODIFED_MPIRUN = /path/to/mpi-bin/bin/mpirun    // Edit

...

# TMIO Github location and code
TMIO_REPO = /path/to/TMIO                       // Edit
TMIO_INC  = $(TMIO_REPO)/include
TMIO_BLD  = $(TMIO_REPO)/build
...
```
### Setting up the various tracing strategies
The subfolders represent the applied bandwidth limiting strategies. To configure TMIO to utilize the different strategies either set `BW_LIMIT_STRATEGY` and `BW_LIMIT_GRANULARITY` manually in `TMIO/include/ioflags.h` or pass them as define flags to the make command for compilation:
```sh
make build-command CXX_DEBUG="-DBW_LIMIT_STRATEGY=N -DBW_LIMIT_GRANULARITY=M"
```
The following bandwidth limiting strategies can be configured and are always applied in the respective folders:
* Traces in the subfolder _always_ represent traces for the strategy referred to as _direct phase_ within the thesis. Set `BW_LIMIT_STRATEGY = 0` and `BW_LIMIT_GRANULARITY = 1`
* Traces in the subfolder *request_size* represent traces for the strategy referred to as _direct phase_ within the thesis. Set `BW_LIMIT_STRATEGY = 0` and `BW_LIMIT_GRANULARITY = 2`
* Traces in the subfolder *file_history* represent traces for the strategy referred to as _direct phase_ within the thesis. Set `BW_LIMIT_STRATEGY = 0` and `BW_LIMIT_GRANULARITY = 3`
* Traces in the subfolder *file_scale* represent traces for the strategy referred to as _direct phase_ within the thesis. Set `BW_LIMIT_STRATEGY = 0` and `BW_LIMIT_GRANULARITY = 4`
* Traces in the subfolder *uponly* represent traces for the strategy referred to as _up-only_ within the thesis. Set `BW_LIMIT_STRATEGY = 1` and `BW_LIMIT_GRANULARITY = 1`
* Traces in the subfolder *adaptive* represent traces for the strategy referred to as _adaptive_ within the thesis. Set `BW_LIMIT_STRATEGY = 2` and `BW_LIMIT_GRANULARITY = 1`

### Replicate results in HACC_IO_RESULTS
For traces within this folder set `BW_LIMIT_STRATEGY` and `BW_LIMIT_STRATEGY` as dictated by the subfolder name, as explained [above](#setting-up-the-various-traced-strategies). Execute the following inside the hacc-io folder to generate the `HACC_ASYNC_IO_BWLIMIT` executable:
```
make limit
```
To recreate traces inside the _nolimit_ folder instead generate `HACC_ASYNC_IO_BWLIMIT` with the following command:
```
make nolimit
```
The required `libtmio.so` is automatically copied into the current directory. The following script `sbatch.sh` can be utilized on the Lichtenberg cluster to execute the application.
```sh
#!/bin/bash
#SBATCH -J HACC_ASYNC_IO
#SBATCH -e %x.err
#SBATCH -o %x.out
#SBATCH -C i01
#SBATCH -n 768
#SBATCH -c 1
#SBATCH -N 8
#SBATCH -A projectXXXX
#SBATCH --mem-per-cpu=3800   
#SBATCH -t 00:15:00

export LD_PRELOAD=./libtmio.so
mkdir -p /work/scratch/user_name/test/

/path/to/mpi-bin/bin/mpirun ./HACC_ASYNC_IO_BWLIMIT 1000000 /work/scratch/user_name/test/

EXITSTATUS=$?
echo “Job $SLURM_JOB_ID has finished at $(date).”
exit $EXITSTATUS
```
The traces are generated using different rank counts. Replace the number in `#SBATCH -n 768` with the desired ranks. Results from this thesis use `#SBATCH -N 1` to set the number of participating nodes to the minimum requirement. Consequently, node count equals the rank count divided by 96.
### Replicate results in HACC_IO_GROWING_PHASES
For traces within this folder set `BW_LIMIT_STRATEGY` and `BW_LIMIT_STRATEGY` as [dictated by the subfolder name](#setting-up-the-various-traced-strategies). Execute the following inside the hacc-io folder to generate the `HACC_ASYNC_IO_GROW` executable:
```sh
make limit_grow
```
The `sbatch.sh` file below can be utilized to execute this on the Lichtenberg cluster.
```sh
#!/bin/bash
#SBATCH -J HACC_ASYNC_IO
#SBATCH -e %x.err
#SBATCH -o %x.out
#SBATCH -C i01
#SBATCH -n 768
#SBATCH -c 2
#SBATCH -N 16
#SBATCH -A projectXXXX
#SBATCH --mem-per-cpu=3800   
#SBATCH -t 00:15:00

export LD_PRELOAD=./libtmio.so
mkdir -p /work/scratch/user_name/test_grow/

/path/to/mpi-bin/bin/mpirun ./HACC_ASYNC_IO_GROW 100000 1000000 /work/scratch/user_name/test_grow/

EXITSTATUS=$?
echo “Job $SLURM_JOB_ID has finished at $(date).”
exit $EXITSTATUS
```
### Replicate results in HACC_IO_VARYING_PHASES
For traces within this folder set `BW_LIMIT_STRATEGY` and `BW_LIMIT_STRATEGY` as [dictated by the subfolder name](#setting-up-the-various-traced-strategies). Execute the following inside the hacc-io folder to generate the `HACC_ASYNC_IO_REPORT` executable:
```sh
make limit_report
```
The `sbatch.sh` file below can be utilized to execute this on the Lichtenberg cluster.
```sh
#!/bin/bash
#SBATCH -J HACC_ASYNC_IO
#SBATCH -e %x.err
#SBATCH -o %x.out
#SBATCH -C i01
#SBATCH -n 768
#SBATCH -c 2
#SBATCH -N 16
#SBATCH -A projectXXXX
#SBATCH --mem-per-cpu=3800   
#SBATCH -t 00:15:00

export LD_PRELOAD=./libtmio.so
mkdir -p /work/scratch/user_name/test_report/

/path/to/mpi-bin/bin/mpirun  ./HACC_ASYNC_IO_REPORT 1000000 1000000 /work/scratch/user_name/test_report/

EXITSTATUS=$?
echo “Job $SLURM_JOB_ID has finished at $(date).”
exit $EXITSTATUS
```
### Replicate results in HACC_IO_PREFETCH
The prefetcher requires frequency information about the application. To generate the traces within the subfolder *get_freq* execute the following command within the hacc-io folder:
```sh
make prefetch_freq
```
This generates the executable `HACC_ASYNC_IO_READ`. TMIO is already included so the LD_PRELOAD method does not have to be applied. For execution on the Lichtenberg cluster utilize the following `sbatch.sh` file:
```sh
#!/bin/bash
#SBATCH -J HACC_ASYNC_IO
#SBATCH -e %x.err
#SBATCH -o %x.out
#SBATCH -C i01
#SBATCH -n 96
#SBATCH -c 1
#SBATCH -N 1 
#SBATCH -A projectXXXX
#SBATCH --mem-per-cpu=3800   
#SBATCH -t 00:15:00

mkdir -p /work/scratch/user_name/test_prefetch/

/path/to/mpi-bin/bin/mpirun ./HACC_ASYNC_IO_READ 1000000 /work/scratch/user_name/test_prefetch/

EXITSTATUS=$?
echo “Job $SLURM_JOB_ID has finished at $(date).”
exit $EXITSTATUS
```
The frequency can be derived from calling FTIO on the generated traces. The virtual python environemnt provided by FTIO has to be sourced before evoking the following command:
```
ftio NNN_MPI.json --mode read_sync
```
The output of ftio can be found as in the provided trace data inside files with the naming scheme `NNN_frequency.txt`.
This frequency information has to be applied inside the file `testHACC_Async_IO_read.cxx`. Find the line defining the preprocessor define for `FREQUENCY`and set it to the frequency determined by FTIO.
```C++
...
#ifndef FREQUENCY
#define FREQUENCY 0.4415 // Edit
#endif
...
```
Alternatively, set the frequency when compiling by appending `CXX_DEBUG="-DPREFETCHING_FREQUENCY=0.4415"` to the make command. `HACC_ASYNC_IO_READ` exhibits different frequency behavior for different rank counts. Thus, frequency has to be redetermined when applying a different number of ranks to the application.

The results within the subfolder _limit_ were generated by applying the appropriate frequency and calling:
```
make prefetch_limit
```
After compilation, the same `sbash.sh` file that was utilized for determining frequency infomation can be used.

The results within the subfolder *nolimit* can be generated by compiling with the following command instead:
```
make prefetch_nolimit
```
The numbered names of the subfolders within the different runs respresent the respective ranks utilized. Replace the number in `#SBATCH -n 768` with the desired ranks.
## WACOMM
Within this section, traces in the folder WACOMM_FREQUENCY are recreated. To achieve this, a modified version of the WaComM++ is required. This version is not publically available and can be obtained on demand. Enabling frequency based bandwidth limiting requires further modifications to this version. Modify the file `m_wacomm1_bw.c`:
```C++
...
#include <assert.h>
#include <tmio_c.h> // Add inclusion
...
MPI_Get_processor_name (mpi_name, &len);
    
set_io_freqency(1.5); // Add this function

if (argc < 3)
...
```
Adjust the `Makefile` to include TMIO as a library and include new building rules.
```Make
...
CC = $(GIT_REPO)/bw_limit/mpich-4.0.3/mpich-bin/bin/mpicc
headerdir := $(GIT_REPO)/TMIO/include/ // !Add
sources := m_wacomm1_bw.c 
binaries := m_wacomm1_bw

LDFLAGS  := -L. -Wl,-rpath,. // !Add
LDLIBS   := -lm -ltmio       // !Add

objects := $(sources:.c=.o)
depends := $(sources:.c=.d)

CPPFLAGS := -MMD
CFLAGS := -I$(headerdir) -I. -std=gnu99 -Wall -Wextra -Werror=uninitialized -O2 -g // !Modify
.PHONY: all clean

all: $(binaries)

$(binaries): $(objects)                             // !Add
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LDLIBS)   // !Add

clean:

...

CXX_DEBUG:=  // Modify
PROCS = 8

lib_dir := $(GIT_REPO)/TMIO/build

...

limit_freq: override CXX_DEBUG := "-DBW_LIMIT -DBW_LIMIT_FREQ=1 -DBW_LIMIT_STRATEGY=0 -BW_LIMIT_GRANULARITY=1 $(CXX_DEBUG)"  // !Add
limit_freq: clean lib all                                                                                                    // !Add

limit_always: override CXX_DEBUG := "-DBW_LIMIT -DBW_LIMIT_STRATEGY=0 -BW_LIMIT_GRANULARITY=1 $(CXX_DEBUG)" // !Add
limit_always: clean lib all	                                                                                // !Add

nolimit: override CXX_DEBUG := "-DCUSTOM_MPI $(CXX_DEBUG)"  // !Add
nolimit: clean lib all                                      // !Add
```
The following `sbatch.sh` file can be utilized on the Lichtenberg cluster to generate the resulting traces:
```sh
#!/bin/bash
#SBATCH -J WACOMM
#SBATCH -e %x.err
#SBATCH -o %x.out
#SBATCH -C i01
#SBATCH -n 1536
#SBATCH -c 1
#SBATCH -N 16
#SBATCH -A projectXXXX
#SBATCH --mem-per-cpu=3800   
#SBATCH -t 00:15:00

mkdir -p /work/scratch/user_name/test/

/path/to/mpi_bin/bin/mpirun ./m_wacomm1_bw 10000000 20 /work/scratch/user_name/test/   

EXITSTATUS=$?
echo “Job $SLURM_JOB_ID has finished at $(date).”
exit $EXITSTATUS
```
For the results inside the get_freq subfolder compile the program with:
```
make nolimit
```
Retrieve the frequency from the generated trace by calling:
```
ftio 1536_MPI.json --mode write_async
```
Set the frequency obtained frequency inside the file `m_wacomm1_bw.c`:
```C++
...
MPI_Get_processor_name (mpi_name, &len);
    
set_io_freqency(/*FREQUENCY*/); // Set frequency here

if (argc < 3)
...
```
Compile using `make limit_always` for traces within the _always_ folder or `make limit_freq` for traces within the *with_freq* folder.
