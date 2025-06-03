#ifndef DERIVATIVES_H
#define DERIVATIVES_H

// function [fx fy fz fxy fxz fyz fxyz]=derivatives(f,x,y,z)
//WARNING, assumes constant grid spacing!!
//Returns with the second partial derivatives of f by the finite
//difference method.  
//  f has dimension [I,J,K]
//  x has dimension [I]
//  y has dimension [J]
//  z has dimension [K]




#include "vector_util.h"
#include "Matrix.h"
#include "gamma_structs.h"

//Return with first and second derivatives
Derivatives<float> derivatives(const std::vector<float> &x, const std::vector<float> &y, 
			       const std::vector<float> &z, const Matrix<float> &f);

//Return with first derivatives only
First_Derivatives<float> derivatives_first(const Matrix<float> &f, const std::vector<float> &x, 
					   const std::vector<float> &y,  const std::vector<float> &z);

//Differentiate in 1 variable
Matrix<float> differentiate_1d(const Matrix<float> &f, float h, int dim);
#endif 
