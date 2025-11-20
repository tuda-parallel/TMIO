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

if [[ ${TARGETS} == *"msgpack"* ]]; then
	echo "building: ${TARGETS}"
	msgpack
	msgpack_build
elif [[ ${TARGETS} == *"zmq"* ]]; then
	echo "building: ${TARGETS}"
	zmq
	zmq_build
else
	echo -e "${RED}No target specified${BLACK}"
fi

echo -e "${GREEN}    -------------- Ready to go ----------------\n ${BLACK}"
