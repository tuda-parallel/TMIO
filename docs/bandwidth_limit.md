# Bandwidth Limiting
## Prerequisite
### Download and install BW-Limit MPI
To install the MPI for bandwidth limitation execute the following commands in the directory you want to install MPI in: 
```sh
git clone https://github.com/nGreen27/MPICH-IOBandwidth-Limitation.git
cd MPICH-IOBandwidth-Limitation/
cp compilar.sh mpich-4.0.3_BW-limit/
mkdir mpi-bin
mpich-4.0.3_BW-limit/compilar.sh $PWD/mpi-bin
```
### Build TMIO with bandwidth limit capabilities
For most traces generated for the thesis installation of TMIO is required. To install TMIO in the current directory execute the following commands:
```sh
git clone https://github.com/tuda-parallel/TMIO.git
cd build
make library CXX_DEBUG+="-DBW_LIMIT" MPICXX=/path/to/mpi-bin/bin/mpicxx
```
This install utilizes the bandwidth limiting capabilities offered by the custom MPI installed [above](#download-and-install-bw-limit-mpi). Download and install the BW-Limit MPI.
### Bandwidth options
The following flags can be set to determined bandwidth limit behavior. Set them within `TMIO/include/ioflags.h` or append them to `CXX_DEBUG` when building TMIO.
| Flag Name | Default | Description |
| :--- | :---: | :--- |
| `BW_LIMIT_STRATEGY`| `2` | `0`: Always limit. `1`: Increase only. `2`: Limit downside. |
| `BW_LIMIT_GRANULARITY`| `1` | `0`: Deactivate limit. `1`: Phase based limit. `2`: Request based limit. `3`: File based limit. `4`: File based limit with scaling. |
| `BW_LIMIT_FREQ` | `0` | `0`: Off. `1`: Utilize frequency information for prefetching. |
## Example
Clone the following modified version of HACC-IO
```
git clone https://github.com/A-Tarraf/hacc-io.git
cd hacc-io
```
Modify the included Makefile to include the paths to the modified MPI and TMIO:

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
Run the example with the following command:
```
make run_limit CXX_DEBUG+="-DBW_LIMIT_STRATEGY=2 -DBW_LIMIT_GRANULARITY=1"
```
