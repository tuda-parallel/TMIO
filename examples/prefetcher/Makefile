TMIO_BASE = ../../

.PHONY: library run

library:
		@cd $(TMIO_BASE)/build && make library CXX_DEBUG+="-DPREFETCH -DPREFETCH_VERBOSE=1"
		@cp $(TMIO_BASE)/build/libtmio.so . 

Prefetcher_example.o: Prefetcher_example.cxx
		mpicxx -c Prefetcher_example.cxx -I$(TMIO_BASE)/include

Prefetcher_example: Prefetcher_example.o library
		mpicxx Prefetcher_example.o -o $@ -I$(TMIO_BASE)/include -L. -ltmio -Wl,-rpath,$(PWD)

run: Prefetcher_example
		mpirun -np 1 ./Prefetcher_example