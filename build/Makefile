
# $@: The file name of the target of the rule. 
# $%: The target member name, when the target is an archive member.
# $<: The name of the first prerequisite. 
# $?: The names of all the prerequisites that are newer than the target, with spaces between them. 
# $^: The names of all the prerequisites, with spaces between them. 
# $+: This is like ‘$^’, but prerequisites listed more than once are duplicated in the order they were listed in the makefile.
# $|: The names of all the order-only prerequisites, with spaces between them.
# $?: is useful even in explicit rules when you wish to operate on only the prerequisites that have changed.

# recursive  =
# simple :=
# appending +=
# conditional ?=

# target specific varibale assignment: 
# foo: CXX_FLAG +=  ..
# foo: all

#! variables
#************
SHELL      := /bin/bash
VERSION    := 1.0.0

PROCS         := 8  #set number of proccesses (2 --> 8)
OPENMPTHREADS := 4

MPICXX = mpicxx
MPIRUN = mpirun
CXX_DEBUG  := 
MPI_RUN_FLAGS =

# MPICXX   = $(TMIO_REPO)/bw_limit/mpich-4.0.3/mpich-bin/bin/mpicxx
# MPIRUN   = $(TMIO_REPO)/bw_limit/mpich-4.0.3/mpich-bin/bin/mpirun
# $(info $(shell tput setaf 1)MPICXX:${MPICXX}$(shell tput sgr0))
# $(info $(shell tput setaf 1)MPIRUN:${MPIRUN} $(shell tput sgr0))
# CXX_DEBUG = -DBW_LIMIT


HOST := $(shell hostname)
ifeq ($(HOST),electric)
	# openMPI
	#MPICXX = /usr/bin/mpic++.openmpi
	#MPIRUN = /usr/bin/mpirun.openmpi
	# MPICH
	#MPICXX = /usr/bin/mpicxx.mpich
	#MPIRUN = /usr/bin/mpirun.mpich
endif

WSL :=$(shell grep -i microsoft /proc/version || uname -r | grep -i wsl || true  )
ifdef WSL
	# "Ubuntu on Windows"
	PYTHON := python3.exe
	PWD := $(subst /d,,$(PWD))
else
	# "native Linux"
	ifeq ($(shell hostname),electric)
		PYTHON := ~/venv/bin/python3
	else
		PYTHON := python3
	endif
endif

SCOREP     := scorep --mpp=mpi --thread=pthread --io=posix --user $(MPICXX)
PREFIX     :=
CXX_FLAGS  :=
CXX        := g++
CXX_INC    := 
CXX_LIB_FLAGS:= 
#CXX_DEBUG  := -DDEBUG -DIOTRACE_VERBOSE=1 -DIODATA_VERBOSE=1
# see ./include/ioflags.h
# CXX_DEBUG  := -DCOLOR_OUTPUT 


TMIO_REPO    = $(shell readlink -f ..)
BIN        := bin
SRC_DIR    := $(TMIO_REPO)/src
INC_DIR    := $(TMIO_REPO)/include
OBJ_DIR    := tmp
DEP_DIR    := $(TMIO_REPO)/dep


# SRC_FILES  := $(wildcard $(SRC_DIR)/*.cxx)
# HED_FILES  := $(wildcard $(INC_DIR)/*.h)
# OBJ_FILES  := $(SRC_FILES:$(SRC_DIR)/%.cxx=$(OBJ_DIR)/%.o)

# Recursively find all source and header files
SRC_FILES  := $(shell find $(SRC_DIR) -name '*.cxx')
HED_FILES  := $(shell find $(INC_DIR) -name '*.h')
# Map source files to object files, preserving subdirectory structure
OBJ_FILES  := $(patsubst $(SRC_DIR)/%.cxx, $(OBJ_DIR)/%.o, $(SRC_FILES))

LIBRARIES  :=
OBJ_FILES_FOR_LIBRARY := $(filter-out $(OBJ_DIR)/test.o, $(OBJ_FILES))
EXECUTABLE := test_run


HOST := $(shell hostname)
ifeq ($(HOST),electric)
	#CXX_DEBUG  += -DCOLOR_OUTPUT  -DDFT=1
endif

#**************************************
#*  Normal build                      *
#**************************************

all: clean_build build run


build: CXX_DEBUG += -DTMIO=1
build: pre $(EXECUTABLE)
# build object files  
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cxx $(HED_FILES)
	$(MPICXX) $(CXX_FLAGS) -c  $< -o  $@ -I$(INC_DIR) $(CXX_DEBUG) $(CXX_INC)

# $(EXECUTABLE): CXX_DEBUG += -DTMIO=1
# link objects
$(EXECUTABLE): $(OBJ_FILES)
	$(MPICXX) $(CXX_FLAGS) -o $@ $^ $(CXX_LIB_FLAGS)


pre: 
	@ mkdir -p $(OBJ_DIR) 
	@ mkdir -p $(dir $(OBJ_FILES)) 
	@ ls $(OBJ_DIR) &>  /dev/null || ( echo "creating Folder  $(OBJ_DIR)" && mkdir $(OBJ_DIR) )
	@ ls $(OBJ_DIR)/test &> /dev/null || ( echo "creating Folder $(OBJ_DIR)/test" && mkdir -p $(OBJ_DIR)/test ) 
	@ ls $(OBJ_DIR)/test/MPI &> /dev/null || ( echo "creating Folder $(OBJ_DIR)/test/MPI" && mkdir -p $(OBJ_DIR)/test/MPI )
	@ ls $(OBJ_DIR)/test/libc &> /dev/null || ( echo "creating Folder $(OBJ_DIR)/test/libc" && mkdir -p $(OBJ_DIR)/test/libc )


run:
	echo $()
	$(PREFIX) $(MPIRUN) -np $(PROCS) $(MPI_RUN_FLAGS) ./$(EXECUTABLE)  


# create Libary
library: clean pre libtmio.so

libtmio.so: CXX_FLAGS += -fPIC -Wall -shared
libtmio.so:  $(OBJ_FILES_FOR_LIBRARY) 
	$(MPICXX) $(CXX_FLAGS) -o $@ $^ $(CXX_LIB_FLAGS)

#**************************************
#*  ScoreP                            *
#**************************************
scorep: clean pre scorep_build scorep_run #scorep_result

scorep_build: MPICXX = $(SCOREP)
scorep_build: CXX_DEBUG += -DSCOREP -DTMIO=0
scorep_build:  $(OBJ_FILES)
	$(MPICXX) $(CXX_FLAGS) -o $(EXECUTABLE) $(OBJ_DIR)/test.o  $(OBJ_DIR)/ioflush.o $(CXX_DEBUG)

scorep_run:
	@ source scoreP.sh && \
	$(MPIRUN) -np $(PROCS) ./$(EXECUTABLE)

scorep_result:
	scorep-score -r ScoreP_Result/profile.cubex  



#**************************************
#*  openMP support                    *
#**************************************

openmp: CXX_FLAGS+=-fopenmp
openmp: CXX_DEBUG += -DOPENMP
openmp: PREFIX+= export OMP_NUM_THREADS=$(OPENMPTHREADS) &&
openmp: all


openmp_library:: CXX_FLAGS+=-fopenmp
openmp_library:: CXX_DEBUG += -DOPENMP
openmp_library: clean pre libtmio.so


#**************************************
#*  ZMQ support                    *
#**************************************
zmq: zmq_pre zmq_build run 

zmq_pre: zmq_git_build msgpack_git_build

zmq_build: CXX_INC += -I$(TMIO_REPO)/dep/msgpack/msgpack-c/include 
zmq_build: CXX_LIB_FLAGS += -lzmq
zmq_build: CXX_DEBUG += -DFILE_FORMAT=3
zmq_build: clean_build build

zmq_library: CXX_INC += -I$(TMIO_REPO)/dep/msgpack/msgpack-c/include 
# zmq_library: CXX_LIB_FLAGS += -lzmq
zmq_library: override CXX_DEBUG := -DFILE_FORMAT=3
zmq_library: clean pre zmq_git_build libtmio.so

#**************************************
#*  MSGPACK support                    *
#**************************************
msgpack: msgpack_build run

msgpack_build: CXX_INC += -I$(TMIO_REPO)/dep/msgpack/msgpack-c/include
msgpack_build: CXX_DEBUG += -DFILE_FORMAT=2
msgpack_build: msgpack_git_build clean_build build

msgpack_library: CXX_INC += -I$(TMIO_REPO)/dep/msgpack/msgpack-c/include
msgpack_library: override CXX_DEBUG := -DFILE_FORMAT=2
msgpack_library: clean pre msgpack_git_build libtmio.so

#**************************************
#* GDB                                *
#**************************************
debug: CXX_FLAGS = -g 
debug: clean build

gdb_run: debug
	$(MPIRUN) -np 1  gdb --args ./test 100000 test/mpi	

gdb_help:
	echo "----------------------------------------------"
	echo "use: "
	echo "    catch syscall"
	echo "    break compute"
	echo "    break verify"
	echo " then use -step- or -next- or -continue- to step through the code and -where- or -backtrace- to see current position"
	echo " and -info breakpoint- to see placed breakpoints"
	echo "----------------------------------------------"



#**************************************
#* Memory Check                       *
#**************************************
memory_check: CXX_FLAGS += -g
memory_check: clean build
	$(MPIRUN) -np 1 valgrind --leak-check=yes --leak-check=full --track-origins=yes ./$(EXECUTABLE)  


#**************************************
#* Memory Overhead    MASSIF          *
#**********************************run
	$(MPICXX) $(CXX_FLAGS) -c   $(SRC_DIR)/test.cxx -o  $(OBJ_DIR)/test.o -I$(INC_DIR) $(CXX_DEBUG)
	$(MPICXX) $(CXX_FLAGS) -o $(EXECUTABLE) $(OBJ_DIR)/test.o $(OBJ_DIR)/ioflush.o

massif_no_tmio: overhead_no_tmio
	$(MPIRUN) -np 1 valgrind --tool=massif --stacks=yes ./$(EXECUTABLE)  
	mv $$(ls -lart | grep massif | tail -1 | awk {'print $$NF'}) massif.out.no_tmio
	# for i in massif.out.*; do ms_print $$i |  head -n9 | tail -1 | grep -oP '.*?(?=\^)' >> massif.no_tmio.out ; rm $$i; done


overhead_tmio: CXX_FLAGS += -g  
overhead_tmio: CXX_DEBUG += -DTMIO=1
overhead_tmio: $(OBJ_FILES) 
	$(MPICXX) $(CXX_FLAGS) -c   $(SRC_DIR)/test.cxx -o  $(OBJ_DIR)/test.o -I$(INC_DIR) $(CXX_DEBUG)
	$(MPICXX) $(CXX_FLAGS) -o $(EXECUTABLE)  $^

overhead_no_tmio: CXX_FLAGS += -g  
overhead_no_tmio: CXX_DEBUG += -DTMIO=0
overhead_no_tmio: $(OBJ_FILES) 
	$(MPICXX) $(CXX_FLAGS) -c   $(SRC_DIR)/test.cxx -o  $(OBJ_DIR)/test.o -I$(INC_DIR) $(CXX_DEBUG)
	$(MPICXX) $(CXX_FLAGS) -o $(EXECUTABLE)  $^

massif_tmio: overhead_tmio
	$(MPIRUN) -np 1 valgrind --tool=massif --stacks=yes ./$(EXECUTABLE)  
	mv $$(ls -lart | grep massif | tail -1 | awk {'print $$NF'}) massif.out.tmio
	# for i in massif.out.*; do ms_print $$i |  head -n9 | tail -1 | grep -oP '.*?(?=\^)' >> massif.tmio.out ; rm $$i; done

memory_overhead: clean pre massif_tmio massif_no_tmio
	# ms_print $$(ls -lart | grep massif | tail -2| awk {'print $$NF'})
	for i in massif*; do ms_print $$i |  head -n9 ; done
	
	
	

massif: memory_overhead

#**************************************
#* Memory Overhead    DHAT            *
#**************************************
#.SILENT: overhead_no_tmio overhead_tmio
dhat_no_tmio:  overhead_no_tmio
	@echo -e "\033[1;31mWithout TMIO:\033[0m";
	@$(MPIRUN) -np $(PROCS) valgrind --tool=dhat  ./$(EXECUTABLE)  2>&1 | grep -A 4 '= Total:'
	

dhat_tmio: overhead_tmio
	@echo -e "\033[1;32mWith TMIO:\033[0m";
	@$(MPIRUN) -np $(PROCS) valgrind --tool=dhat  ./$(EXECUTABLE)  2>&1 | grep -A 4 '= Total:'
	
	

dhat: 	clean pre dhat_tmio dhat_no_tmio


#**************************************
#* DEBUG                              *
#**************************************
debug_code: CXX_DEBUG += -DFUNCTION_INFO=0 -DIOTRACE_VERBOSE=0 -DIODATA_VERBOSE=1
debug_code: all run


#**************************************
#*  Tests build                      *
#**************************************

# Filter out test.h and test.cxx from SRC_FILES and HED_FILES
SRC_FILES_FOR_CATCH2 := $(filter-out $(SRC_DIR)/test.cxx, $(SRC_FILES))
HED_FILES_FOR_CATCH2 := $(filter-out $(INC_DIR)/test.h, $(HED_FILES))
OBJ_FILES_FOR_CATCH2 := $(filter-out $(OBJ_DIR)/test.o, $(OBJ_FILES))

# Add a variable for the test executable
TEST_EXECUTABLE_MPI := run_test_mpi
TEST_EXECUTABLE_LIBC := run_test_libc

# Add a variable for the test source directories
TEST_SRC_DIR_MPI := $(TMIO_REPO)/test/MPI
TEST_SRC_DIR_LIBC := $(TMIO_REPO)/test/libc

# Add a variable for the test source files
TEST_SRC_FILES_MPI := $(wildcard $(TEST_SRC_DIR_MPI)/*.cxx)
TEST_SRC_FILES_LIBC := $(wildcard $(TEST_SRC_DIR_LIBC)/*.cxx)

# Add a variable for the test object files
TEST_OBJ_FILES_MPI := $(TEST_SRC_FILES_MPI:$(TMIO_REPO)/test/%.cxx=$(OBJ_DIR)/test/%.o)
TEST_OBJ_FILES_LIBC := $(TEST_SRC_FILES_LIBC:$(TMIO_REPO)/test/%.cxx=$(OBJ_DIR)/test/%.o)

# Add the include path for Catch2
CXX_INC += -I$(TMIO_REPO)/lib/catch2 -I$(TMIO_REPO)/test/utils

# Add a target for compiling the MPI test executable
$(TEST_EXECUTABLE_MPI): $(TEST_OBJ_FILES_MPI) $(OBJ_FILES_FOR_CATCH2)
	$(MPICXX) $(CXX_FLAGS) -o $@ $^ $(CXX_LIB_FLAGS)

# Add a target for compiling the libc test executable
$(TEST_EXECUTABLE_LIBC): $(TEST_OBJ_FILES_LIBC) $(OBJ_FILES_FOR_CATCH2)
	$(MPICXX) $(CXX_FLAGS) -o $@ $^ $(CXX_LIB_FLAGS)

# Add a target for running the MPI tests
.PHONY: test_MPI
test_MPI: $(TEST_EXECUTABLE_MPI)
	$(MPIRUN) -np $(PROCS) ./$(TEST_EXECUTABLE_MPI) 

# Add a target for running the libc tests
.PHONY: test_libc
test_libc: $(TEST_EXECUTABLE_LIBC) 
	./$(TEST_EXECUTABLE_LIBC)

# Add a general test target that calls both test_MPI and test_libc, would first build the project
.PHONY: test
test: build
test: test_MPI test_libc

# Add a rule for compiling test object files
$(OBJ_DIR)/test/%.o: $(TMIO_REPO)/test/%.cxx $(TMIO_REPO)/lib/catch2/catch.hpp $(SRC_FILES_FOR_CATCH2) $(HED_FILES_FOR_CATCH2)
	@ mkdir -p $(OBJ_DIR)/test 
	$(MPICXX) $(CXX_FLAGS) -c $< -o $@ -I$(INC_DIR) $(CXX_DEBUG) $(CXX_INC)



#**************************************
#*  Clean                             *
#**************************************
clean_build:  
	@ rm -f  file*  *.txt *.err 
	@ rm -rf ScoreP_Result* 
	@ rm -f   *.o 
	@ rm -f   *.out 
	@ rm -f   dhat.out.*
	@ rm -f   massif.out.*

clean: clean_build
	@ rm -f $(EXECUTABLE)
	@ rm -rf $(OBJ_DIR)
	@ rm -f $(TEST_EXECUTABLE_MPI)
	@ rm -f $(TEST_EXECUTABLE_LIBC)
	@ rm -f *.json *.jsonl *.msgpack *.bin*
	@ rm -f libtmio.so

#************************  SCRIPTS ************************

#? Roofline             
roofline:
	@echo -e "creating roofline in  ~/.local/"
	@echo -e "#! /bin/bash \n\n\
#************************\n\
# author : Ahmad Tarraf\n\
# version: $(VERSION)\n\
# date   : $$(date)\n\
#************************\n\n\
# usage:\n\
#************************\n\
#1) ./roofline.sh name.json                -> creates graphs for a single json file\n\
#2) ./roofline.sh name.json save           -> + promts to save\n\
#3) ./roofline.sh folder_no_sub_dir        -> creates grpah for a single folder if it has no subdirectories. No post statistics \n\
#3) ./roofline.sh *.json                   -> same as above\n\
#3) ./roofline.sh single_folder/*.json     -> same as above\n\
#3) ./roofline.sh ./*/*.json               -> same as above\n\
#4) ./roofline.sh *.json save              -> + promts to save \n\
#5) ./roofline.sh folder                   -> if folder has subdirectories, statistics are created that merge the results \n\
#6) ./roofline.sh folder save              -> + promts to save \n\
\n\n\
python3 $(TMIO_REPO)/python/roofline/roofline.py \$$@ " > ~/.local/roofline
	@chmod u+x ~/.local/roofline
	@echo -e "----- done -----\n"


#? Tools                
tools: parse freq_plot ftio ioplot roofline predictor

clean_tools:
	rm -f ~/.local/bin/parse
	rm -f ~/.local/bin/ioplot
	rm -f ~/.local/bin/freq_plot
	rm -f ~/.local/bin/ftio
	rm -f ~/.local/bin/predictor

#? Parse                
parse:
	@echo -e "creating parse in  ~/.local/bin/"
	@echo -e "#! /bin/bash \n\n\
#************************\n\
# author : Ahmad Tarraf\n\
# version: $(VERSION)\n\
# date   : $$(date)\n\
#************************\n\n\
$(PYTHON) $(TMIO_REPO)/python/parse.py \$$@ " > ~/.local/bin/parse
	@chmod u+x ~/.local/bin/parse
	@echo -e "----- done -----\n"

remove:
	rm -f  ~/.local/bin/parse ~/.local/bin/roofline 


#? Ioplot               
ioplot: 
	@echo -e "creating ioplot in  ~/.local/bin/"
	@echo -e "#! /bin/bash \n\n\
#************************\n\
# author : Ahmad Tarraf\n\
# version: $(VERSION)\n\
# date   : $$(date)\n\
#************************\n\n\
$(PYTHON) $(TMIO_REPO)/python/ioplot.py \$$@ " > ~/.local/bin/ioplot
	@chmod u+x ~/.local/bin/ioplot
	@echo -e "----- done -----\n"
	
	@$(PYTHON) $(TMIO_REPO)/python/install/install_packages.py 




#?---------------------------------------------------------------------------------------------
#? freq_plot            
freq_plot:
	@echo -e "creating freq_plot in  ~/.local/bin/"
	@echo -e "#! /bin/bash \n\n\
#************************\n\
# author : Ahmad Tarraf\n\
# version: $(VERSION)\n\
# date   : $$(date)\n\
#************************\n\n\
$(PYTHON) $(TMIO_REPO)/python/freq_plot.py \$$@ " > ~/.local/bin/freq_plot
	@chmod u+x ~/.local/bin/freq_plot
	@echo -e "----- done -----\n"

#?---------------------------------------------------------------------------------------------
#? ftio             
ftio:
	@echo -e "creating ftio in  ~/.local/bin/"
	@echo -e "#! /bin/bash \n\n\
#************************\n\
# author : Ahmad Tarraf\n\
# version: $(VERSION)\n\
# date   : $$(date)\n\
#************************\n\n\
$(PYTHON) $(TMIO_REPO)/python/ftio.py \$$@ " > ~/.local/bin/ftio
	@chmod u+x ~/.local/bin/ftio
	@echo -e "----- done -----\n"
 
	@$(PYTHON) $(PWD)/ftio.py 2>&1 |  grep -i "No module" | awk {'print $NF'} 

#? predictor            
predictor.sh:
	@echo -e "creating predictor in  ~/.local/bin/"
	ln -s $$(readlink -f src/bash/predictor.sh) ~/.local/bin/predictor.sh
	@echo -e "----- done -----\n"

#? playback            
playback:
	@echo -e "creating $@ in  ~/.local/bin/"
	@echo -e "#! /bin/bash \n\n\
#************************\n\
# author : Ahmad Tarraf\n\
# version: $(VERSION)\n\
# date   : $$(date)\n\
#************************\n\n\
$(PYTHON) $(TMIO_REPO)/python/$@.py \$$@ " > ~/.local/bin/$@
	@chmod u+x ~/.local/bin/$@
	@echo -e "----- done -----\n"


#? predictor_new        
predictor: ftio
	@echo -e "creating $@ in  ~/.local/bin/$@"
	@echo -e "#! /bin/bash \n\n\
#************************\n\
# author : Ahmad Tarraf\n\
# version: $(VERSION)\n\
# date   : $$(date)\n\
#************************\n\n\
$(PYTHON) $(TMIO_REPO)/python/predictor.py \$$@ " > ~/.local/bin/$@
	@chmod u+x ~/.local/bin/$@
	@echo -e "----- done -----\n"


docker:
	docker build -t tmio:1.0 .


docker_run:
	docker run -v "$$PWD/8.jsonl:/tmio/8.jsonl" -t tmio:1.0 python ftio.py 8.jsonl -e no 


docker_interactive:
	docker run -ti   tmio:1.0


msgpack_git_build:
	chmod u+x $(TMIO_REPO)/dep/create_lib.sh
	$(TMIO_REPO)/dep/create_lib.sh "msgpack"

zmq_git_build:
	chmod u+x $(TMIO_REPO)/dep/create_lib.sh
	$(TMIO_REPO)/dep/create_lib.sh "zmq" || echo "Failed to build ZMQ"
