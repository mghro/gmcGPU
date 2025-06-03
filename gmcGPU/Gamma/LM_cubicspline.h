#ifndef BEN_CUBICSPLINE_H
#define BEN_CUBICSPLINE_H


//
//
// INPUT
// xi = initial grid [mm], a vector for each axis.  Must be equally spaced!
// x  = a matrix of x positions [mm] indicating X position to be interpolated 
//   x, y, z matrices must have the same size and can be 1d, 2d or 3d and
//   are not necessarily equally spaced
// alpha = the alpha matrix from tricubic_alpha(xi,yi,zi,f)
// flag_1st_deriv = flag to return with 1st-order derivatives (true=yes)
// 
// OUTPUT
// g = f at interpolated coordinates,
// gx = df/dx at interpolated coordinates,
//   "g" and "g derivatives" have the same number of elements and size as x 
//
// Calculation method can be found in:
//   "Tricubic interpolation in three dimensions"
//   F. Lekien and J. Marsden
//   INTERNATIONAL JOURNAL FOR NUMERICAL METHODS IN ENGINEERING
//   Int. J. Numer. Meth. Eng 2005; 63:455-471

#include "vector_util.h"
#include "matrix.h"
#include "gamma_structs.h"
#include <algorithm>    // std::max

Search_Matrices<float> LM_cubicspline(const std::vector<float> &xi, const std::vector<float> &yi,
		const std::vector<float> &zi, const Mesh_Grid<float> &mesh0,
		const float *alpha, unsigned long int size_alpha, bool flag_1st_deriv, double t_start);

Search_Matrices<float> LM_cubicspline(const std::vector<float> &xi, const std::vector<float> &yi, const std::vector<float> &zi, 
				       const std::vector<float> &xf, const std::vector<float> &yf, const std::vector<float> &zf, 
				       const float *alpha, unsigned long int size_alpha, bool flag_1st_deriv, double t_start);

#endif 
