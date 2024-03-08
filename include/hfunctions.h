#include <iostream>
#include <algorithm>
#include <mpi.h>
#include <vector>
#include <fstream>
#include <limits>
#include <math.h> 
#include "ioflags.h"
#include "iometrics.h"
#ifdef OPENMP
#include <omp.h>
#endif

#if FILE_FORMAT > 1
#include "msgpack.hpp"
#endif

#if FILE_FORMAT > 2
#include <zmq.hpp>
#endif

namespace iohf{

double Median(double [],int);
double Arithmetic_Mean(double [],int);
double Harmonic_Mean(double [],int);
double Harmonic_Mean_Non_Zero(double [] ,int);
double Weighted_Harmonic_Mean(long long *, double* , int);
double Min(double [],int,int* ptr=NULL);
double Variance(double [], int n, double mean = NAN);
double Standard_Deviation(double [], int n, double mean = NAN);
double Agg_Over_Ranks(double *, int *, int, int mode = 1);

template <class T>
void   Disp(T ,int , std::string, int mode = 0);

template <class T>
T Sum(T *,int);

template<class T>
T Max(T *, int);

template <class T>
void Phase_Info(T ,T &,T &,MPI_Datatype);

template <class T>
void Phase_Max(T ,T &, MPI_Datatype);

template <class T>
void  Gather_Summary(int, int ,int, T* , std::vector <T>, int*,MPI_Comm IO_WORLD = MPI_COMM_WORLD ,MPI_Datatype type = MPI_DOUBLE);

int  Get_Io_Ranks(int, int*);
std::string Get_File_Format(int);
void Set_Unit(double , std::string &,double &);
void Compute_Statistics(std::vector <double>,int );


// function  for finding phases for the entire job
// find overlap in phases between different ranks
void Overlap(std::vector<std::vector<int>>&, std::vector<double> &, double*, double*, int*, int, int);
// sorts array and returns sorted index array
int* Sort_With_Index(double * , int ); 
int *Sort_With_Index(collect*, int, std::string mode = "t_start");

// add bandwidth where phases of different ranks (see Overlap) overlap
double* Phase_Bandwidth(std::vector<std::vector<int>>&, double*);
// return non-empty 2D count
int Non_Empty(std::vector<std::vector<int>>&);

int* N_Phase(std::vector<std::vector<int>>&);

// find minimum distance between elements in vector
double Sample_Time(double *, int );

// debud functions
void Overlap_Graph(std::vector<std::vector<int>>&, int);

void Function_Debug(std::string);
}

#ifdef COLOR_OUTPUT 
#define BLACK   "\033[0m"
#define GREEN   "\033[1;32m"
#define YELLOW  "\033[1;33m"
#define RED     "\033[1;31m"
#define BLUE    "\033[1;34m"
#define CYAN    "\033[1;36m"
#else
#define BLACK   ""
#define RED     ""
#define GREEN   ""
#define YELLOW  ""
#define BLUE    ""
#define CYAN    ""
#endif



