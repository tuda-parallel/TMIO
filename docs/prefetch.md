# Prefetcher Documentation
## Prerequisites
### Setup application and TMIO
Define the prefetch flag when building TMIO to enable prefetching:
```
make library "CXX_DEBUG+="-DPREFETCH"
```
Additionally, TMIO has to be included into the application directly. Add `#include <tmio.h>` into the appropriate file.
### Download and install FTIO
IO frequency information is required to utilize the prefetcher. FTIO can analyze traces from TMIO to generate a frequency. To install FTIO in the current folder execute the following commands:
```sh
git clone https://github.com/tuda-parallel/FTIO.git
cd FTIO
make install
```
After installation source the python virtual environment made available by FTIO. For more information on installing FTIO visit the [FTIO github repository](https://github.com/tuda-parallel/FTIO).
## General Usage
### Direct inclusion
Directly call the prefetcher init function from within the code. 
```
tmio::init_prefetcher(io_frequency, max_cache_bytes, max_file_bytes);
```
### Retrieve data from FTIO
Build TMIO with ZMQ support and set the FETCH_FTIO_FREQ flag to 1 in `TMIO/include/ioflags.h`. Start the `predictor`tool included with the following command:
```
predictor --zmq --zmq_port 5555 --zmq_port_reply 5556
```
Call the following function inside the code to send data to FTIO:
```
tmio::io_summary();
```
Frequency information can then either be retrieved by calling
```
tmio::retrieve_FTIO_frequency();
```
Or is automatically retrieved with the next summary call.
## Example
A simple example for the prefetcher can be found in the examples folder