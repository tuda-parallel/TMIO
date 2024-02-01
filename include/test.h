#ifndef HEADER
#define HEADER
#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <iostream>
#include <fstream>
#include <cmath>
#include <chrono>
#include <vector>
#include <fstream>
#include <algorithm>
#include <cstring>
#endif

#ifdef SCOREP 
#include <scorep/SCOREP_User.h>
//#include </opt/score-p/include/scorep/SCOREP_User.h>
#endif

#if TMIO == 1
#include "tmio.h"
#endif
#include "ioflush.h"


// #define DEBUG 0
#define NODE_DEBUG 1