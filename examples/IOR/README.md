# IOR Example:

Below are instructions on how to link TMIO to the IOR benchmark. The total changes are shown at the [result](#result) section. 

## Instructions:


1. Clone IOR:

	``` sh
	git clone git@github.com:hpc/ior.git
	```

2. In `src/` folder of IOR, modify the Makefile as following:

   1. Add this line anywhere (e.g., at the end of the file):
		``` make
		TMIO_DIR = absolute_path_to_tmio_dir #e.g., /d/github/TMIO
		```

   2. Add to LIBS the TMIO lib:
		```make
		LIBS = ... -ltmio
		```

   3. Add a call for building the library:
	   	```make
		shared_lib: 
			cd $(TMIO_DIR)/build && make library ; cd -
			cp $(TMIO_DIR)/build/libtmio.so libtmio.so
		``` 

   4. Add include dir:
		```make
		DEFAULT_INCLUDES = -I.  -I$(TMIO_DIR)/include
		DEFS = ... -DINCLUDE
		```

   5. Add LDFLAGS:
		```make
		LDFLAGS =  -L. -Wl,-rpath,$(PWD)
		```

   6. Add to the all target:
		```make
		all: config.h shared_lib
		```

3. Next modify either ior.c or aiori-MPIIO.c to include the lib. For ior.c add:

	``` c
	#ifdef INCLUDE
	#include "tmio_c.h"
	#endif 

	// (Somewhere, for example at the end of the file)
	#ifdef INCLUDE
	if (access == READ){
			iotrace_summary();}
	#endif
	return (dataMoved);
	```

4. Finally, in the IOR root directory, call:
	``` sh
	make
	```

</br>
</br>

<p align="right"><a href="#ior-example">⬆</a></p>

## Result

If everything is right, libtmio.so should be created in the ./src folder of IOR. 
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

</br>

Git diff on the Makefile should show:
``` diff
+TMIO_DIR = /d/github/TMIO
 
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

</br>
Now you can run IOR as usual. For example, on a cluster with SLURM installed, call :

```sh
srun ./ior  -N ${SLURM_NPROCS} -t 2m -b 10m -s 2 -i 8 -a MPIIO
```

<p align="right"><a href="#ior-example">⬆</a></p>