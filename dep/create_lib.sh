#!/usr/bin/env bash

BLACK='\033[0m'
GREEN='\033[1;32m'
YELLOW='\033[1;33m'
RED='\033[1;31m'
BLUE='\033[1;34m'
CYAN='\033[1;36m'
echo "The script you are running has:"
echo "basename: [$(basename "$0")]"
echo "dirname : [$(dirname "$0")]"
echo "pwd     : [$(pwd)]"

DIR=$(dirname "$0")
TARGETS="${1:="msgpack"}"

echo -e "${BLUE}    --------------- Building -----------------\n ${BLACK}"
echo -e "${BLUE}Building: ${TARGETS} ${BLACK}"




############# msgpack ######################
# create git
msgpack () {
	if [ ! -d "${DIR}/${FUNCNAME[0]}" ]; then
		mkdir -p ${DIR}/${FUNCNAME[0]}
		git clone "https://github.com/msgpack/msgpack-c.git" "${DIR}/${FUNCNAME[0]}/msgpack-c"
	else
		echo "git already exists:"${DIR}/${FUNCNAME[0]}/msgpack-c""
	fi
}


msgpack_build (){
	cd ${DIR}/msgpack/msgpack-c
	git checkout cpp_master
	cmake . ; 	make 
	echo "Successfully created ${FUNCNAME[0]}"
}
#######################################

############# zmq ######################
# create git
zmq () {
	if [ ! -d "${DIR}/${FUNCNAME[0]}" ]; then
		mkdir -p ${DIR}/${FUNCNAME[0]}
		git clone "https://github.com/zeromq/libzmq.git" "${DIR}/${FUNCNAME[0]}/libzmq"
		git clone "https://github.com/zeromq/cppzmq.git" "${DIR}/${FUNCNAME[0]}/cppzmq"
		
	else
		echo "git already exists:"${DIR}/${FUNCNAME[0]}/cppzmq""
	fi
}


zmq_build (){
	cd ${DIR}/zmq/libzmq
	mkdir build && cd build && cmake .. ; make 
	cd ${DIR}/zmq/cppzmq
	cmake . ; make 
	echo "Successfully created ${FUNCNAME[0]}"
}
#######################################

############# liburing ######################
# create git
liburing () {
	if [ ! -d "${DIR}/${FUNCNAME[0]}" ]; then
		mkdir -p ${DIR}/${FUNCNAME[0]}
		git clone "https://github.com/axboe/liburing.git" "${DIR}/${FUNCNAME[0]}/liburing"
	else
		echo "git already exists:"${DIR}/${FUNCNAME[0]}/liburing""
	fi
}


liburing_build (){
	cd ${DIR}/liburing/liburing
	./configure ; make
	# `-luring` needs an unversioned `liburing.so` to link against, and the
	# runtime loader looks for the file named after the embedded SONAME
	# (liburing.so.2) inside our RUNPATH -- a bare build only produces the
	# fully-versioned file (liburing.so.2.15). Normally `make install` adds
	# both symlinks, but that installs system-wide, which we don't want for
	# a vendored dependency. Without the SONAME symlink specifically, the
	# loader silently falls back to any system-installed liburing instead.
	( cd src && \
		full_so=$(ls liburing.so.*.* | head -1) && \
		soname=$(readelf -d "$full_so" | sed -n 's/.*Library soname: \[\(.*\)\]/\1/p') && \
		ln -sf "$full_so" liburing.so && \
		ln -sf "$full_so" "$soname" )
	echo "Successfully created ${FUNCNAME[0]}"
}
#######################################

############# bw_limit_mpich ######################
# Custom MPICH 4.0.3 with an I/O bandwidth-limiting patch in its ROMIO ufs
# ADIO driver (chunked usleep() pacing driven by the same EMPI_DESIRED_BW_*
# globals Bw_limit writes to -- see include/bw_limit.h). Needed to build with
# -DBW_LIMIT / -DCUSTOM_MPI (see docs/bandwidth_limit.md).
bw_limit_mpich () {
	if [ ! -d "${DIR}/${FUNCNAME[0]}" ]; then
		mkdir -p ${DIR}/${FUNCNAME[0]}
		git clone "https://github.com/jfmunoz00/MPICH-IOBandwidth-Limitation.git" "${DIR}/${FUNCNAME[0]}/MPICH-IOBandwidth-Limitation"
	else
		echo "git already exists:"${DIR}/${FUNCNAME[0]}/MPICH-IOBandwidth-Limitation""
	fi
}

bw_limit_mpich_build (){
	cd ${DIR}/bw_limit_mpich/MPICH-IOBandwidth-Limitation
	cp compilar.sh mpich-4.0.3_BW-limit/
	chmod +x mpich-4.0.3_BW-limit/compilar.sh
	mkdir -p ${DIR}/bw_limit_mpich/mpich-bin
	mpich-4.0.3_BW-limit/compilar.sh $(readlink -f ${DIR}/bw_limit_mpich/mpich-bin)
	echo "Successfully created ${FUNCNAME[0]}"
}
#######################################

if [[ ${TARGETS} == *"msgpack"* ]]; then
	echo "building: ${TARGETS}"
	msgpack
	msgpack_build
elif [[ ${TARGETS} == *"zmq"* ]]; then
	echo "building: ${TARGETS}"
	zmq
	zmq_build
elif [[ ${TARGETS} == *"bw_limit_mpich"* ]]; then
	echo "building: ${TARGETS}"
	bw_limit_mpich
	bw_limit_mpich_build
elif [[ ${TARGETS} == *"liburing"* ]]; then
	echo "building: ${TARGETS}"
	liburing
	liburing_build
else
	echo -e "${RED}No target specified${BLACK}"
fi

echo -e "${GREEN}    -------------- Ready to go ----------------\n ${BLACK}"
