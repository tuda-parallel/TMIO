## Prerequisites
### Optional: Download and install BW-Limit MPI
To install the MPI for bandwidth limitation execute the following commands in the directory you want to install MPI in: 
```sh
git clone https://github.com/nGreen27/MPICH-IOBandwidth-Limitation.git
cd MPICH-IOBandwidth-Limitation/
cp compilar.sh mpich-4.0.3_BW-limit/
mkdir mpi-bin
mpich-4.0.3_BW-limit/compilar.sh $PWD/mpi-bin
```
### Download and install TMIO
Execute the following commands compile tmio
```sh
git clone https://github.com/tuda-parallel/TMIO.git
cd build
make library
```
Download and install the BW-Limit MPI. To install with bandwidth limiting simply replace the make command with the following:
```
make library CXX_DEBUG+="-DBW_LIMIT" MPICXX=/path/to/mpi-bin/bin/mpicxx
```
### Download and install VELOC
To install VELOC with TMIO, setup the environment variable `TMIO_ROOT` to reference the base directory of TMIO:
```sh
export TMIO_ROOT=/path/to/TMIO
```
For a successfull install of all of VELOC's prerequisites with the optional bandwidth limiting MPI make sure to include the BW-Limit MPI bin into your path and add the lib into the LD libraries. Additionally installed MPI libraries may cause conflicts.
```sh
export PATH=/path/to/mpi-bin/bin:$PATH
export LD_LIBRARY_PATH=/path/to/mpi-bin/lib:$LD_LIBRARY_PATH
```
Download the modified VELOC from github:
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
### Example
For an example follow the guide offered by the modified VELOC [readme](https://github.com/nGreen27/VELOC/blob/main/README.md) and the [VELOC Docs](https://veloc.readthedocs.io/en/latest/userguide.html). For direct examples look in [checkpoint_reproducability.md](https://github.com/tuda-parallel/TMIO/blob/development/docs/libc.md).