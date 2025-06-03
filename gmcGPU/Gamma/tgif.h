#ifndef TGIF_H
#define TGIF_H

#include "vector_util.h"
#include "Matrix.h"
#include "bounding_box.h"
#include "gamma_structs.h"
#include "tricubic_alpha.h"
#include "LM_cubicspline.h"
#include "voxels_less_than_1.h"
#include "GammaIO.h"
#include <ctime>
#include <string.h>

using namespace std;

/*
Tricubic Gamma Index Function
signed_gamma=tgif
          (x0, y0, z0, unit0, D0, 
           x1, y1, z1, unit1, D1, 
           d_gamma, D_gamma,
           thresh, analysis_type)
  
INPUT:
   x0,y0,z0: vectors containing the reference grid,  units are [init0]
   x1,y1,z1: vectors containing the evaluated (searched) grid, units are [init1]
   unit0 and unit1 are spatial units either 'mm', 'cm', 'm', or 'in'
   D0: reference dose Matrix< D0(x0,y0,z0), units must be the same as D1 and D_gamma
   D1: evaluated dose Matrix< D1(x1,y1,z1)         
   unitD0 and unitD1 dose units either 'cGy', 'cGyRBE', 'Gy' 'GyRBE' 
   d_gamma: Gamma distance tolerance,  typically 3 mm
   unit_d: d_gamma units.  either 'mm', 'cm', 'm', or 'in'
   D_gamma: Gamma dose tolerance. It can be either a scaler or a Matrix< with dimensions the same as D0.
   unit_D: D_gamma units.  either 'cGy', 'cGyRBE', 'Gy' 'GyRBE', '%D0', or '%D1max' 
   thresh: a scalar with the dose level that defines the region of
           interest.  
   unit_th: D_gamma units.  either 'cGy', 'cGyRBE', 'Gy' 'GyRBE', or '%D1max' 
   Nmax: the maximum number of iterations when searching for the closest agreement
        Use 0 for no iterations (Fast)

OUTPUT:
   signed_gamma: signed_gamma index Matrix< with the same dimensions as D0.
*/

//Gamma calc with dose tolerance D_gamma varying with position (units are not percent, 
//they are the same as the units as D0 and D1)
//Matrix<float> tgif(vector<float> x0, vector<float> y0, vector<float> z0,  string unit0, 
//		   Matrix<float> &D0, string unitD0, 
//		   vector<float> x1, vector<float> y1, vector<float> z1,  string unit1, 
//		   Matrix<float> &D1, string unitD1,
//		   float d_gamma, string unit_d, float D_gamma_const, string unit_D, 
//		   float thresh, string unit_th, int Nmax) ;

//Gamma calc with constant dose tolerance D_gamma_const (units are not percent, 
//they are the same as the units as D0 and D1)
void tgif(vector<float> x0, vector<float> y0, vector<float> z0,  string unit0, 
		   Matrix<float> D0, string unitD0, 
		   vector<float> x1, vector<float> y1, vector<float> z1,  string unit1, 
		   Matrix<float> D1, string unitD1,
		   float d_gamma, string unit_d, Matrix<float> D_gamma, string unit_D, 
		   float thresh, string unit_th, int Nmax, Matrix<float>& signed_gamma, SGammaResults* gammaResults) ;


#endif  //TGIF_H
