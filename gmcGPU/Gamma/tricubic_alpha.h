#ifndef TRICUBIC_ALPHA_H
#define TRICUBIC_ALPHA_H

//
//This function returns with alpha that is used in the tricubic interpolation
//Alpha depends on the initial function "f" and initial grid "xi,yi,zi"
//Alpha does NOT depend on the coordinates to be interpolated 
//
// xi = initial grid [mm], a vector for each axis.  Must be equally spaced!
// f = initial function at (xi,yi,zi).  Size of f is numel(xi),numel(yi),numel(zi)
// Size of Alpha is numel(f),64 
//
//  Alpha = M * fvect, where M is a 64*64 matrix of constants and fvect = [f(:), df/dx(:), df/dy(:), ...]
//  The units of all elements of "Alpha" are the same as the elements of
//  "f".  Those elements of Alpha with df/dx are normalized per grid spacing.
// 
// 
// Calculation of derivatives at the grid points by finite difference


#include "vector_util.h"
#include "matrix.h"
#include "derivatives.h"


void tricubic_alpha(float *alpha, unsigned long int size_alpha, const std::vector<float> &xi, const std::vector<float> &yi, 
		    const std::vector<float> &zi, const Matrix<float> &fmat, double t_start);
#endif 
