#!/bin/bash

GIT_REPO=$(readlink -f ..)
BLACK="\033[0m"
GREEN="\033[1;32m"
YELLOW="\033[1;33m"
RED="\033[1;31m"
BLUE="\033[1;34m"
CYAN="\033[1;36m"

cd ${GIT_REPO}/build


# standard test
(make clean 1>/dev/null && echo -e "${GREEN}passed test (0/10): clean${BLACK}") || echo -e "${RED}failed test (0/10): clean${BLACK}"
(make build 1>/dev/null && echo -e "${GREEN}passed test (1/10): build${BLACK}") || echo -e "${RED}failed test (1/10): build${BLACK}"
(make run 1>/dev/null && echo -e "${GREEN}passed test (2/10): run${BLACK}")     || echo -e "${RED}failed test (2/10): run${BLACK}"
(make msgpack 1>/dev/null && echo -e "${GREEN}passed test (3/10): msgpack${BLACK}") || echo -e "${RED}failed test (3/10): msgpack${BLACK}"

# library test
make clean
(make library 1>/dev/null && echo -e "${GREEN}passed test (4/10): library${BLACK}") || echo -e "${RED}failed test (4/10): library${BLACK}"
(make msgpack_library 1>/dev/null && echo -e "${GREEN}passed test (5/10): msgpack_library${BLACK}") || echo -e "${RED}failed test (5/10): msgpack_library${BLACK}"

# special tetst
(make debug 1>/dev/null  && echo -e "${GREEN}passed test (6/10): debug${BLACK}")   ||  echo -e "${RED}failed test (6/10): debug${BLACK}"
(make dhat 1>/dev/null && echo -e "${GREEN}passed test (7/10): dhat${BLACK}")      || echo -e "${RED}failed test (7/10): dhat${BLACK}"
(make memory_overhead 1>/dev/null && echo -e "${GREEN}passed test (8/10): memory_overhead${BLACK}") || echo -e "${RED}failed test (8/10): memory_overhead${BLACK}"
(make openmp 1>/dev/null && echo -e "${GREEN}passed test (9/10): openmp${BLACK}") ||  echo -e "${RED}failed test (9/10): openmp${BLACK}"

# python tools
(make tools 1>/dev/null && echo -e "${GREEN}passed test (10/10): tools${BLACK}")   || echo -e "${RED}failed test (10/10): tools${BLACK}"

make clean