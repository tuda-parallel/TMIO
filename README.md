<!-- # tmio -->
![license][license.bedge]
![Coveralls branch](https://img.shields.io/coverallsCoverage/github/tuda-parallel/TMIO)
![GitHub Release](https://img.shields.io/github/v/release/tuda-parallel/TMIO)
![issues](https://img.shields.io/github/issues/tuda-parallel/TMIO)
![contributors](https://img.shields.io/github/contributors/tuda-parallel/TMIO)
![c++][c++.bedge]
 

<br />
<div align="center">
  <h1 align="center">TMIO</h1>
  <p align="center">
 <h3 align="center"> Tracing MPI-IO </h2>
    <a href="https://git.rwth-aachen.de/parallel/TMIO">View Demo</a>
    ·
    <a href="https://github.com/tuda-parallel/TMIO/issues">Report Bug</a>
    ·
    <a href="https://github.com/tuda-parallel/TMIO/issues">Request Feature</a>
  </p>
</div>

This repository contains the TMIO source code. TMIO is a C++ tracing library that can be easily 
attached to existing code to monitor MPI-IO online. The tool traces synchronous as well as asynchronous I/O.
TMIO offers a C as well as a c++ interface.
We provide two methods for linking the library to the application, depending on whether the information is used for [offline](#offline-analysis) or [online](#online-analysis) analysis. 

The generated file can be easily examined with all provided tools from the [FTIO](https://github.com/tuda-parallel/FTIO) repo to: 
1. find the period of the I/O phases [offline](https://github.com/tuda-parallel/FTIO#usage) or [online](https://github.com/tuda-parallel/FTIO/blob/main/docs/approach.md#online-prediction)
2. visualize the results with [`ioplot`](https://github.com/tuda-parallel/FTIO/blob/main/docs/tools.md#ioplot)
3. parse using [`ioparse`](https://github.com/tuda-parallel/FTIO/blob/main/docs/tools.md#ioparse) several trace files to a single profile to examine with [Extra-P](https://github.com/extra-p/extrap)



<!-- TABLE OF CONTENTS -->
<details>
  <summary>Table of Contents</summary>
  <ol>
    <li>
      <a href="#getting-started">Getting Started</a>
      <ul>
        <li><a href="#prerequisites">Prerequisites</a></li>
        <li><a href="#installation">Installation</a></li>
		<li><a href="#testing">Testing</a></li>
      </ul>
    <li><a href="#Usage">Usage</a></li>
	<ul>
        <li><a href="#offline-tracing">Offline Tracing</a></li>
        <li><a href="#online-tracing">Online Tracing</a></li>
      </ul>
    <li><a href="#roadmap">Roadmap</a></li>
    <li><a href="#contributing">Contributing</a></li>
    <li><a href="#contact">Contact</a></li>
    <li><a href="#license">License</a></li>
    <li><a href="#acknowledgments">Acknowledgments</a></li>
  </ol>
</details>

see latest updates here: [Latest News](https://github.com/tuda-parallel/TMIO/tree/main/ChangeLog.md)

## Getting Started
### Prerequisites
- MPI
- G++

<p align="right"><a href="#tmio">⬆</a></p>

### Installation
Go to the build directory:
```sh
cd build
```
Build the library using the Makefile.
The library can be built with or without MessagePack:
- Standard :
	``` sh
	make library
	```

- MessagePack support:
	```sh
	make msgpack_library
	```

Options can be passed with flags to the `make` command or through the file [`./include/ioflags.h`](https://github.com/tuda-parallel/TMIO/tree/main/include/ioflags.h).

<p align="right"><a href="#tmio">⬆</a></p>

### Testing
To test the the library works execute:
``` sh
make clean
make 
```

To test the MessagePack support, call:
``` sh
make msgpack
```

<p align="right"><a href="#tmio">⬆</a></p>

## Usage
There are 2 ways to use this library to trace I/O: **offline** or **online** tracing.

<p align="right"><a href="#tmio">⬆</a></p>

### Offline Analysis:

The offline mode uses the LD_PRELOAD mechanism.
The offline mode uses the LD_PRELOAD mechanism. Upon MPI Finalize, the collected data is written to a single file to be analyzed later. 
In the online mode, the application is compiled with the TMIO library and a line is added to indicate when
to flush the results out to a file (JSON Lines or MessagePack). 


For offline tracing, the `LD_PRELOAD` mechanism is used. First, build the library with either MessagePack support or not (see [Installation](#installation)).
After that, just call:
```
LD_PRELOAD=path_to_lib/libtmio.so $(MPIRUN)  -np $(PROCS) $(MPI_RUN_FLAGS) ./your_code variable_1 variable_2
```

### Online Analysis:
The code needs to be compiled with the library. 

An example on how to modify IOR is provided [here](/examples/IOR/README.md#instructions)



## Contributing

If you have a suggestion that would make this better, please fork the repo and create a pull request.

1. Fork the Project
2. Create your Feature Branch (`git checkout -b feature/AmazingFeature`)
3. Commit your Changes (`git commit -m 'Add some AmazingFeature'`)
4. Push to the Branch (`git push origin feature/AmazingFeature`)
5. Open a Pull Request

<p align="right"><a href="#tmio">⬆</a></p>

<!-- CONTACT -->
## Contact

[![][parallel.bedge]][parallel_website]

- Ahmad Tarraf: <ahmad.tarraf@tu-darmstadt.de>

<p align="right"><a href="#tmio">⬆</a></p>

## License

![license][license.bedge]

Distributed under the BSD 3-Clause License. See [LISCENCE](./LICENSE) for more information.
<p align="right"><a href="#tmio">⬆</a></p>

<!-- ACKNOWLEDGMENTS -->
## Acknowledgments

Authors:

- Ahmad Tarraf
- Add your name here


<p align="right"><a href="#tmio">⬆</a></p>

## Citation

```
 @inproceedings{Tarraf_Bandet_Boito_Pallez_Wolf_2024, 
  author={Tarraf, Ahmad and Bandet, Alexis and Boito, Francieli and Pallez, Guillaume and Wolf, Felix},
  title={Capturing Periodic I/O Using Frequency Techniques}, 
  booktitle={2024 IEEE International Parallel and Distributed Processing Symposium (IPDPS)}, 
  address={San Francisco, CA, USA}, 
  year={2024},
  month=may, 
  pages={1–14}, 
  notes = {(accepted)}
 }
```

<p align="right"><a href="#tmio">⬆</a></p>

## Publications

1. A. Tarraf, A. Bandet, F. Boito, G. Pallez, and F. Wolf, “Capturing Periodic I/O Using Frequency Techniques,” in 2024 IEEE International Parallel and Distributed Processing Symposium (IPDPS), San Francisco, CA, USA, May 2024, pp. 1–14.
<p align="right"><a href="#tmio">⬆</a></p>





[pipeline.badge]: https://git.rwth-aachen.de/parallel/tmio/badges/main/pipeline.svg
[coverage.badge]: https://git.rwth-aachen.de/parallel/tmio/badges/main/coverage.svg
[c++.bedge]: https://img.shields.io/badge/C++-00599C?style=flat-square&logo=C%2B%2B&logoColor=white
[c.bedge]: https://img.shields.io/badge/c-%2300599C.svg?style=for-the-badge&logo=c&logoColor=white
[license.bedge]: https://img.shields.io/badge/License-BSD_3--Clause-blue.svg
[linkedin.bedge]: https://img.shields.io/badge/LinkedIn-0077B5?tyle=for-the-badge&logo=linkedin&logoColor=white
[parallel_website]: https://www.parallel.informatik.tu-darmstadt.de/laboratory/team/tarraf/tarraf.html
[parallel.bedge]: https://img.shields.io/badge/Parallel_Programming:-Ahmad_Tarraf-blue