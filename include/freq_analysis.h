
#include "hfunctions.h"
#include <cmath>

#if DFT >= 1
#include <complex>
#endif


/**
 *  DFT transformation and outlier detection DURING file execution
 * @file   freq_analysis.h
 * @author Ahmad Tarraf
 * @date   23.10.2023
 */

namespace freq_analysis
{
#if DFT >= 1

    std::complex<double>* Dft(double *, double *, int , bool , bool , int , double&, double freq = 0);
    std::string DFT_Create_String(std::string, double* p, int, char*, int, bool end = false);
    void DFT_Sample(int, double, double*, double* t, double*, int n, double t_short = -1);
    void DFT_Core(int, double*, std::complex<double>* ,double*, double *, double&);
    //void DFT_Confidence_Check(std::vector<double>, double, int, double*);
    double DFT_Confidence_Check(double, double, int, double*);
#endif
    
}