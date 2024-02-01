
![license][license.bedge] &emsp; ![coverage][coverage.badge] &emsp;![pipline][pipeline.badge] 
<br />

<div align="center">
  <a href="https://git.rwth-aachen.de/parallel/tmio">
    <!-- <img src="images/logo.png" alt="Logo" width="80" height="80"> -->
	TMIO LOGO
  </a>
</div>


This repository contains the TMIO source code.
- For installation, see [Installation](#installation)
- See the list of updates here: [Latest News](#latest-news)


<!-- TABLE OF CONTENTS -->
<details>
  <summary>Table of Contents</summary>
  <ol>
    <li>
      <a href="#about-the-project">About The Project</a>
      <ul>
        <li><a href="#built-with">Built With</a></li>
      </ul>
    </li>
    <li>
      <a href="#getting-started">Getting Started</a>
      <ul>
        <li><a href="#prerequisites">Prerequisites</a></li>
        <li><a href="#installation">Installation</a></li>
      </ul>
    </li>
    <li><a href="#quick-start">Usage</a></li>
    <li><a href="#roadmap">Roadmap</a></li>
    <li><a href="#contributing">Contributing</a></li>
    <li><a href="#contact">Contact</a></li>
    <li><a href="#license">License</a></li>
    <li><a href="#acknowledgments">Acknowledgments</a></li>
  </ol>
</details>

## About The Project

`TMIO` stand for **T**echniques for **M**PI. **I**/**O**.
`TMIO` contains a tracing library that allows to monitor online MPI I/O. 

For any questions, feel free to contact me: <ahmad.tarraf@tu-darmstadt.de>

### Built With
The project was build with C and C++.

- ![c++][c++.bedge]
- ![c][c.bedge]

## Latest News

- no update yet

## Getting Started
### Prerequisites
- MPI

### Installation
Go to the build directory:
```
cd build
```
Build the library using the Makefile.
The library can be build with or without msgpack:
- Standard :
	```
	make library
	```

- Msgpack support:
	```
	make msgpack_library
	```

Options can be passed with flags or through the file `./include/iofalgs.h` or through the Makefile variables. 

### Testing
To test the library, run:
```
make
make clean
make msgpack
```

## Usage
There are 2 ways to use this library to trace I/O: **offline** or **online** tracing.
### Offline Tracing:
For offline tracing, the `LD_PRELOAD` mechanism is used. First, build the library with either msgpack support or not (see [Installation](#installation)).
After that, just call:
```
LD_PRELOAD=path_to_lib/libtmio.so $(MPIRUN)  -np $(PROCS) $(MPI_RUN_FLAGS) ./your_code variable_1 variable_2
```

### Online Tracing:
The code needs to be compiled with the library. Let's take IOR as an example. 

#### IOR Example:
clone ior:
``` bash
git clone git@github.com:hpc/ior.git
```
In `src/` modify the Makefile as following:

1. Add this line:
	``` make
	TMIO_DIR = /d/git/tarraf/tmio
	```
2. Add to LIBS the TMIO lib:
	``` make
	LIBS = ... -ltmio
	```
3. Add a call for building the library:
	``` make
	shared_lib: 
		cd $(TMIO_DIR)/build && make library ; cd -
		cp $(TMIO_DIR)/build/libtmio.so libtmio.so
	``` 
4. Add include dir:
	``` make
	DEFAULT_INCLUDES = -I.  -I$(TMIO_DIR)/include
	DEFS = ... -DINCLUDE
	```
5. Add LDFLAGS:
	``` make
	LDFLAGS =  -L. -Wl,-rpath,$(PWD)
	```
6. Add to the all target:
	``` make
	all: config.h shared_lib
	```

Next modify either ior.c or aiori-MPIIO.c to include the lib. For ior.c:

``` c
#ifdef INCLUDE
#include "tmio_c.h"
#endif 

// (Somewhere, for example at the end of the file add:)
#ifdef INCLUDE
if (access == READ){
		iotrace_summary();}
#endif
return (dataMoved);
```

Finally, in ior root directory, call:
``` bash
make
```

If everything is right, libtmio.so should be created in the ./src folder of ior. 
Moreover, git diff on ior.c should show:
``` diff 
+#ifdef INCLUDE
+#include "tmio_c.h"
+#endif 
+
 enum {
         IOR_TIMER_OPEN_START,
         IOR_TIMER_OPEN_STOP,
@@ -1869,5 +1873,9 @@ static IOR_offset_t WriteOrRead(IOR_param_t *test, IOR_results_t *results,
           aligned_buffer_free(randomPrefillBuffer, test->gpuMemoryFlags);
         }
 
+               #ifdef INCLUDE
+               if (access == READ){
+                               iotrace_summary();}
+               #endif
         return (dataMoved);

```

Git diff on the Makefile should show:
``` diff
+TMIO_DIR = /d/git/tarraf/tmio
 
@@ -501,7 +505,7 @@ AM_V_at = $(am__v_at_$(V))
 am__v_at_ = $(am__v_at_$(AM_DEFAULT_VERBOSITY))
 am__v_at_0 = @
 am__v_at_1 = 
-DEFAULT_INCLUDES = -I.
+DEFAULT_INCLUDES = -I. -I$(TMIO_DIR)/include
 depcomp = $(SHELL) $(top_srcdir)/config/depcomp
 am__maybe_remake_depfiles = depfiles
 am__depfiles_remade = ./$(DEPDIR)/IOR-aiori-CEPHFS.Po \

@@ -735,7 +739,7 @@ CPPFLAGS =  -Icheck/include -Icheck/include
 CSCOPE = cscope
 CTAGS = ctags
 CYGPATH_W = echo
-DEFS = -DHAVE_CONFIG_H
+DEFS = -DHAVE_CONFIG_H -DINCLUDE
 DEPDIR = .deps
 ECHO_C = 
 ECHO_N = -n

@@ -747,12 +751,12 @@ INSTALL_DATA = ${INSTALL} -m 644
 INSTALL_PROGRAM = ${INSTALL}
 INSTALL_SCRIPT = ${INSTALL}
 INSTALL_STRIP_PROGRAM = $(install_sh) -c -s
-LDFLAGS =  -Lcheck/lib64 -Lcheck/lib -Wl,--enable-new-dtags -Wl,-rpath=check/lib64:check/lib -Lcheck/lib64 -Lcheck/lib -Wl,--enable-new-dtags -Wl,-rpath=check/lib64:check/lib
+LDFLAGS =  -Lcheck/lib64 -Lcheck/lib -Wl,--enable-new-dtags -Wl,-rpath=check/lib64:check/lib -Lcheck/lib64 -Lcheck/lib -Wl,--enable-new-dtags -Wl,-rpath=check/lib64:check/lib -L. -Wl,-rpath,$(PWD)
LIBOBJS = 
-LIBS = -lm 
+LIBS = -lm -ltmio
 LTLIBOBJS = 
 MAINT = #

@@ -880,7 +884,7 @@ MDTEST_LDFLAGS = $(mdtest_LDFLAGS)
 MDTEST_LDADD = $(mdtest_LDADD)
 MDTEST_CPPFLAGS = $(mdtest_CPPFLAGS)
 libaiori_a_CPPFLAGS = $(extraCPPFLAGS)
-all: config.h
+all: config.h shared_lib
        $(MAKE) $(AM_MAKEFLAGS) all-recursive

@@ -3721,3 +3725,8 @@ build.conf:
 # Tell versions [3.59,3.63) of GNU make to not export all variables.
 # Otherwise a system limit (for SysV at least) may be exceeded.
 .NOEXPORT:
+
+
+shared_lib: 
+               cd $(TMIO_DIR)/build && make library ; cd -
+               cp $(TMIO_DIR)/build/libtmio.so libtmio.so
+               
```

Now you can run it. On the cluster call for example:
``` bash
srun ./ior  -N ${SLURM_NPROCS} -t 2m -b 10m -s 2 -i 8 -a MPIIO
```
## Contributing

Contributions are what make the open source community such an amazing place to learn, inspire, and create. Any contributions you make are **greatly appreciated**.

If you have a suggestion that would make this better, please fork the repo and create a pull request. You can also simply open an issue with the tag "enhancement".
Don't forget to give the project a star! Thanks again!

1. Fork the Project
2. Create your Feature Branch (`git checkout -b feature/AmazingFeature`)
3. Commit your Changes (`git commit -m 'Add some AmazingFeature'`)
4. Push to the Branch (`git push origin feature/AmazingFeature`)
5. Open a Pull Request




## Contact
Ahmad Tarraf
- ahmad.tarraf@tu-darmstadt.de 
- [![][parallel.bedge]][parallel_website]

Project Link: [https://git.rwth-aachen.de/parallel/tmio](https://git.rwth-aachen.de/parallel/tmio)



## License

Distributed under the BSD 3-Clause License. See `LICENSE` for more information.


## Acknowledgments
Authors: 
  - Ahmad Tarraf

Publication:

[pipeline.badge]: https://git.rwth-aachen.de/parallel/tmio/badges/main/pipeline.svg
[coverage.badge]: https://git.rwth-aachen.de/parallel/tmio/badges/main/coverage.svg
[c++.bedge]: https://img.shields.io/badge/C++-00599C?style=flat-square&logo=C%2B%2B&logoColor=white
[c.bedge]: https://img.shields.io/badge/c-%2300599C.svg?style=for-the-badge&logo=c&logoColor=white
[license.bedge]: https://img.shields.io/badge/License-BSD_3--Clause-blue.svg
[linkedin.bedge]: https://img.shields.io/badge/LinkedIn-0077B5?tyle=for-the-badge&logo=linkedin&logoColor=white
[parallel_website]: https://www.parallel.informatik.tu-darmstadt.de/laboratory/team/tarraf/tarraf.html
[parallel.bedge]: https://img.shields.io/badge/Parallel_Programming:-Ahmad_Tarraf-blue
