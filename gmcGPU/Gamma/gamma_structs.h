#ifndef GAMMA_STRUCTS_H
#define GAMMA_STRUCTS_H

#include <vector>
#include "matrix.h"

//#define TIME_COUT

template <typename T>
struct Dose_Cube
{
  std::vector<T> x,y,z; //axes of the dose cube [mm]
  Matrix<T> D; //Dose [Gy]
};

template <typename T>
struct Derivatives
{
  Matrix<T> fx,fy,fz,fxy,fxz,fyz,fxyz; //partial derivatives of f with respect to x,y,z
};

template <typename T>
struct First_Derivatives
{
  Matrix<T> fx,fy,fz; //partial derivatives of f with respect to x,y,z
};

template <typename T>
struct Search_Matrices
{
  Matrix<T> D1_interp, fx,fy,fz; 
};


#endif
