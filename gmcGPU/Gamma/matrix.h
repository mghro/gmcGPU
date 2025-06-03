#ifndef MATRIX_H
#define MATRIX_H

#include <cmath>
#include <vector>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <cstring>
#include "vector_util.h"

using namespace std;


template <typename T>
struct Matrix
{
  //Fill vector M as follow:
  // M.at(idx.idy,idz) is actually M.at(idz*ny*nx + idy*nx + idx)
  //where 0 <= idx < nx , 
  //      0 <= idy < ny , and, 
  //      0 <= idz < nz .
  std::vector<T> M;
  unsigned long int nx, ny, nz;
};


template <typename T>
struct Mesh_Grid
{
  Matrix<T> x, y, z;
};

//(D,X,Y,Z) are vectors where (D(ii,jj,kk),X(ii),Y(jj),Z(kk)) specifies a grid_point
//X,Y [mm] are in IEC coordinate system
//Z [mm] points upstream
//D is the dose deposited at that voxel (same units as the Astroid database, GyRBE)
template <typename T>
struct Grid_Points
{
  std::vector<T> D; //,x,y,z; //These vectors are the same size, each index represents a voxel
  std::vector<T> xg,yg,zg; //Axis vectors for the grid
};



/* //return with X indeces at the matrix locations where the boolean is true */
/* std::vector<unsigned long int> matrix_ix(Matrix<bool> &matrix) { */
/*   std::vector<unsigned long int> result; */
/*   for (unsigned long int idz=0; idz<matrix.nz;++idz) { */
/*     for (unsigned long int idy=0; idy<matrix.ny;++idy) { */
/*       for (unsigned long int idx=0; idx<matrix.nx;++idx) { */
/* 	if (matrix.M.at(idz*matrix.ny*matrix.nx+idy*matrix.nx+idx)) result.push_back(idx); */
/*       }}}       */
/*   return result; */
/* } */

/* //return with Y indeces at the matrix locations where the boolean is true */
/* std::vector<unsigned long int> matrix_iy(Matrix<bool> &matrix) { */
/*   std::vector<unsigned long int> result; */
/*   for (unsigned long int idz=0; idz<matrix.nz;++idz) { */
/*     for (unsigned long int idy=0; idy<matrix.ny;++idy) { */
/*       for (unsigned long int idx=0; idx<matrix.nx;++idx) { */
/* 	if (matrix.M.at(idz*matrix.ny*matrix.nx+idy*matrix.nx+idx)) result.push_back(idy); */
/*       }}}	 */
/*   return result; */
/* } */

/* //return with Z indeces at the matrix locations where the boolean is true */
/* std::vector<unsigned long int> matrix_iz(Matrix<bool> &matrix) { */
/*   std::vector<unsigned long int> result; */
/*   for (unsigned long int idz=0; idz<matrix.nz;++idz) { */
/*     for (unsigned long int idy=0; idy<matrix.ny;++idy) { */
/*       for (unsigned long int idx=0; idx<matrix.nx;++idx) { */
/* 	if (matrix.M.at(idz*matrix.ny*matrix.nx+idy*matrix.nx+idx)) result.push_back(idz); */
/*       }}}	 */
/*   return result; */
/* } */



template <typename T>
void matrix_create(Matrix<T> &matrix, unsigned long int n_x, unsigned long int n_y, unsigned long int n_z) {  //allocates memory but doesn't initialize
  matrix.M.clear();
  matrix.M.reserve(n_x*n_y*n_z);
  matrix.nx = n_x;
  matrix.ny = n_y;
  matrix.nz = n_z;
}


template <typename T, typename T2>
void matrix_create(Matrix<T> &matrix, unsigned long int n_x, unsigned long int n_y, unsigned long int n_z, T2 val) { //allocates memory and initializes
  matrix.M.clear();
  matrix.M.resize(n_x*n_y*n_z, (T) val);
  matrix.nx = n_x;
  matrix.ny = n_y;
  matrix.nz = n_z;
}
//faster 
template <typename T>
Matrix<float> matrix_create(const unsigned long int n_x, const unsigned long int n_y, const unsigned long int n_z, T val) { //allocates memory and initializes
  Matrix<float> matrix;  matrix.M.resize(n_x*n_y*n_z,(float) val);
  matrix.nx = n_x;  matrix.ny = n_y;  matrix.nz = n_z;  return matrix; }

template <typename T>
Matrix<T> transpose(const Matrix<T> &matrix) { 
  Matrix<T> result;
  if (matrix.nz !=1) {
    logfile_error("matrix.h : transpose : cannot transpose a 3d matrix\n"); 
    size(matrix);
    return result;
  }
  result.M.reserve(matrix.nx*matrix.ny);
  result.ny = matrix.nx;
  result.nx = matrix.ny;
  result.nz = 1;
  
  for (unsigned long int idx=0; idx<matrix.nx; ++idx) {
    for (unsigned long int idy=0; idy<matrix.ny; ++idy) {
      result.M.push_back(matrix.M[idy* matrix.nx+ idx]); 
    } }  
  return result;
}

template <typename T>
Mesh_Grid<T> meshgrid(const std::vector<T> &X, const std::vector<T> &Y, const std::vector<T> &Z) { 
  Mesh_Grid<T> result;
  result.x.M.reserve(X.size()*Y.size()*Z.size());
  result.y.M.reserve(X.size()*Y.size()*Z.size());
  result.z.M.reserve(X.size()*Y.size()*Z.size());
  for (unsigned long int idz=0; idz<Z.size();++idz) {
    for (unsigned long int idy=0; idy<Y.size();++idy) {
      for (unsigned long int idx=0; idx<X.size();++idx) {
	result.x.M.push_back(X[idx]);
	result.y.M.push_back(Y[idy]);
	result.z.M.push_back(Z[idz]);
      }}}
  result.x.nx=X.size();
  result.y.nx=X.size();
  result.z.nx=X.size();
  result.x.ny=Y.size();
  result.y.ny=Y.size();
  result.z.ny=Y.size();
  result.x.nz=Z.size();
  result.y.nz=Z.size();
  result.z.nz=Z.size();
  return result;
}

template <typename T>
unsigned long int numel(const Matrix<T> &matrix) { 
  return matrix.M.size();
}

template <typename T>
void matrix_init(Matrix<T> &matrix) { //sets all values to zero
  for (unsigned long int ii=0; ii<matrix.M.size(); ++ii) matrix.M[ii]=0;
}

template <typename T, typename T2>
void matrix_set(Matrix<T> &matrix, unsigned long int idx, unsigned long int idy, unsigned long int idz, T2 val) {
  unsigned long int ind = idz*matrix.ny*matrix.nx+idy*matrix.nx+idx;
  if (idx >= matrix.nx || idy >= matrix.ny || idz >= matrix.nz ||  ind>= matrix.M.size()) {
    logfile_error("matrix.h : matrix_set : index %u %u %u %u exceeds matrix dimensions %u %u %u %u\n", 
		  idx, idy,idz,idz*matrix.ny*matrix.nx+idy*matrix.nx+idx,
		  matrix.nx, matrix.ny, matrix.nz, matrix.M.size()); 
    return ;
  }
  matrix.M.at(ind)= (T) val;
}

template <typename T, typename T2>
void matrix_set(Matrix<T> &matrix, Matrix<bool> &mask, T2 val) {
  if (mask.nx != matrix.nx || mask.ny != matrix.ny || mask.nz != matrix.nz ||  mask.M.size()!= matrix.M.size()) {
    logfile_error("matrix.h : matrix_set : matrix dimension mismatch\n", 
		  mask.nx, mask.ny, mask.nz, mask.M.size(),
		  matrix.nx, matrix.ny, matrix.nz, matrix.M.size()); 
    return ;
  }
  vector_util_set(matrix.M, mask.M, val);
}

template <typename T, typename T2>
void matrix_sum_elem(Matrix<T> &matrix, unsigned long int idx, unsigned long int idy, unsigned long int idz, T2 val) {
  matrix.M.at(idz*matrix.ny*matrix.nx+idy*matrix.nx+idx) += (T) val;
}

template <typename T> 
T matrix_sum_elem(const Matrix<T> &matrix) {
  T s = 0;
  for (unsigned long int ii=0; ii<matrix.M.size();++ii)
    s += matrix.M[ii];
  return s;
}

template <typename T>
T matrix_get(const Matrix<T> &matrix, unsigned long int idx, unsigned long int idy, unsigned long int idz) {
  unsigned long int ind = idz*matrix.ny*matrix.nx+idy*matrix.nx+idx;
  if (idx >= matrix.nx || idy >= matrix.ny || idz >= matrix.nz ||  ind>= matrix.M.size()) {
    logfile_error("matrix.h : matrix_get : index %u %u %u %u exceeds matrix dimensions %u %u %u %u\n", 
		  idx, idy,idz,idz*matrix.ny*matrix.nx+idy*matrix.nx+idx,
		  matrix.nx, matrix.ny, matrix.nz, matrix.M.size()); 
    return 0;
  }
  return matrix.M.at(ind);
}

template <typename T>
std::vector<T> matrix_get_column(const Matrix<T> &matrix, unsigned long int idy, unsigned long int idz) {
  if (idy >= matrix.ny || idz >= matrix.nz ) {
    logfile_error("matrix.h : matrix_get_column : column index %u %u exceeds matrix dimensions %u %u\n", 
		  idy,idz, matrix.ny, matrix.nz); 
    std::vector<T> r;
    return r;
  }

  std::vector<T> result(matrix.M.begin()+idz*matrix.nx*matrix.ny+idy*matrix.nx,
			matrix.M.begin()+idz*matrix.nx*matrix.ny+idy*matrix.nx +matrix.nx);
  return result;
}

template <typename T>
std::vector<T> matrix_get_column(const Matrix<T> &matrix, unsigned long int idy) {

  if (matrix.nz>1) {
    logfile_error("matrix.h : matrix_get_column : incomplete function arguments for 3d matrix\n"); 
    std::vector<T> r;
    return r;
  }
  if (idy >= matrix.ny  ) {
    logfile_error("matrix.h : matrix_get_column : column index %u exceeds matrix dimensions %u\n", 
		  idy,matrix.ny); 
    std::vector<T> r;
    return r;
  }
  std::vector<T> result(matrix.M.begin()+idy*matrix.nx,
			matrix.M.begin()+idy*matrix.nx +matrix.nx);
  return result;
}



template <typename T>
Matrix<T> matrix_get(const Matrix<T> &matrix, const std::vector<unsigned long int> &idx, 
		     const std::vector<unsigned long int> &idy, const std::vector<unsigned long int> &idz) {
  Matrix<T> res;
  if (idx.size()==0 || idy.size() == 0 || idz.size() ==0) return res;
  if (vector_util_max(idx)>=matrix.nx || vector_util_max(idy)>=matrix.ny || vector_util_max(idz)>=matrix.nz ||
      vector_util_min(idx)<0 || vector_util_min(idy)<0 || vector_util_min(idz)<0 ) {
    logfile_error("matrix.h : matrix_get : index %u %u %u exceeds matrix dimensions %u %u %u\n", 
		  vector_util_max(idx), vector_util_max(idy),vector_util_max(idz),
		  matrix.nx, matrix.ny, matrix.nz); 
    return res;
  }
  res.nx = idx.size();
  res.ny = idy.size();
  res.nz = idz.size();
  matrix_create(res, idx.size(), idy.size(), idz.size());
  for (unsigned long int kk=0; kk<idz.size(); ++kk) {
    unsigned long int tmpz = idz[kk]*matrix.ny*matrix.nx;
    for (unsigned long int jj=0; jj<idy.size(); ++jj) { 
      unsigned long int tmpy = idy[jj]*matrix.nx;
      for (unsigned long int ii=0; ii<idx.size(); ++ii) { 
	res.M.push_back(matrix.M[tmpz+tmpy+idx[ii]]);
      }}}
  return res;
}



template <typename T>
std::vector<T> matrix_get(const Matrix<T> &a, const std::vector<bool>& b) {
  if (a.M.size()!= b.size()) 
    logfile_error("matrix.h : matrix_get : matrix dimension mismatch\n"); 
  return vector_util_get(a.M, b);
}


template <typename T>
void matrix_print(const Matrix<T> &matrix) {  
  if (matrix.M.empty()) {
    return ;
  }
  if (matrix.M.size()!= matrix.nx*matrix.ny*matrix.nz) {
    logfile_error("matrix.h : matrix_print : incorrectly formed matrix\n"); 
    std::vector<T> matrix;
    return;
  }
  for (unsigned long int idz=0; idz<matrix.nz;++idz) {
    for (unsigned long int idx=0; idx<matrix.nx;++idx) {
      for (unsigned long int idy=0; idy<matrix.ny;++idy) {
	logfile_cout(matrix.M.at(idz*matrix.ny*matrix.nx+idy*matrix.nx+idx));
	if (idy <matrix.ny-1)
	  logfile_cout(',');
      }
      logfile_cout('\n');
    }
    if (idz <matrix.nz-1)
      logfile_cout('\n');
  }
  logfile_cout("=======================\n");
  return ;
}

template <typename T>
Matrix<T> sqrt(const Matrix<T> &matrix) {
  Matrix<T> result=matrix;
  for (unsigned long int ii=0; ii<matrix.M.size(); ++ii) result.M[ii]=sqrt(matrix.M[ii]);
  return result;
}

template <typename T>
Matrix<T> abs(const Matrix<T> &matrix) {
  Matrix<T> result=matrix;
  for (unsigned long int ii=0; ii<matrix.M.size(); ++ii) {
    if (result.M[ii]<0) result.M[ii] *= -1; 
  }
  return result;
}

template <typename T>
Matrix<T> sign(const Matrix<T> &matrix) {
  Matrix<T> result;
  matrix_create(result, matrix.nx, matrix.ny, matrix.nz, 0);
  for (unsigned long int ii=0; ii<matrix.M.size(); ++ii) {
    if (matrix.M[ii]<0) result.M[ii] = -1;
    else if (matrix.M[ii]>0) result.M[ii] = 1;
  }
  
  return result;
}


template <typename T>
T matrixMax(const Matrix<T> &matrix) {
  if (matrix.M.size()==0) return 0;
  T maxval = matrix.M[0];
  for (unsigned long int ii=1; ii<matrix.M.size(); ++ii) {if (maxval<matrix.M[ii])  maxval = matrix.M[ii];}
  return maxval;
}

template <typename T>
T matrixMin(const Matrix<T> &matrix) {
  if (matrix.M.size()==0) return 0;
  T minval = matrix.M[0];
  for (unsigned long int ii=1; ii<matrix.M.size(); ++ii) {if (minval>matrix.M[ii])  minval = matrix.M[ii];}
  return minval;
}


template <typename T, typename T2>
Matrix<T> operator+(const Matrix<T> &matrix, const T2 &a) 
{ 
  Matrix<T> result= matrix; 
  for (unsigned long int i=0; i<result.M.size(); ++i) result.M[i] += (T) a; 
  return result;
}

template <typename T, typename T2>
Matrix<T> operator-(const Matrix<T> &matrix, const T2 &a) 
{ 
  Matrix<T> result= matrix; 
  for (unsigned long int i=0; i<result.M.size(); ++i) result.M[i] -= (T) a; 
  return result;
}

template <typename T, typename T2>
Matrix<T> operator*(const Matrix<T> &matrix, const T2 &a) 
{ 
  Matrix<T> result= matrix; 
  for (unsigned long int i=0; i<result.M.size(); ++i) result.M[i] *= (T) a; 
  return result;
}

template <typename T, typename T2>
Matrix<T> operator/(const Matrix<T> &matrix, const T2 &a) 
{ 
  Matrix<T> result= matrix; 
  for (unsigned long int i=0; i<result.M.size(); ++i) result.M[i] /= (T) a; 
  return result;
}


template <typename T, typename T2>
Matrix<T> operator+(const T2 &a, const Matrix<T> &matrix) 
{ 
  Matrix<T> result= matrix; 
  for (unsigned long int i=0; i<result.M.size(); ++i) result.M[i] += (T) a; 
  return result;
}

template <typename T, typename T2>
Matrix<T> operator-(const T2 &a, const Matrix<T> &matrix) 
{ 
  Matrix<T> result= matrix; 
  for (unsigned long int i=0; i<result.M.size(); ++i) result.M[i] = (T) a - result.M[i]; 
  return result;
}

template <typename T, typename T2>
Matrix<T> operator*(const T2 &a, const Matrix<T> &matrix) 
{ 
  Matrix<T> result= matrix; 
  for (unsigned long int i=0; i<result.M.size(); ++i) result.M[i] *= (T) a; 
  return result;
}

template <typename T, typename T2>
Matrix<T> operator/(const T2 &a, const Matrix<T> &matrix) 
{ 
  Matrix<T> result= matrix; 
  for (unsigned long int i=0; i<result.M.size(); ++i) result.M[i] = (T) a / result.M[i]; 
  return result;
}



template <typename T>
Matrix<T> operator+(const Matrix<T> &matrix, const Matrix<T> &a) 
{ 
  Matrix<T> result;
  if (matrix.M.size() != a.M.size()) {
    logfile_error("matrix.h : operator+ : matrix size mismatch\n"); 
    return result;
  }
  result = matrix;
  for (unsigned long int i=0; i<result.M.size(); ++i) result.M[i] += a.M[i]; 
  return result;
}

template <typename T>
Matrix<T> operator-(const Matrix<T> &matrix, const Matrix<T> &a) 
{ 
  Matrix<T> result;
  if (matrix.M.size() != a.M.size()) {
    logfile_error("matrix.h : operator- : matrix size mismatch\n"); 
    return result;
  }
  result = matrix;
  for (unsigned long int i=0; i<result.M.size(); ++i) result.M[i] -= a.M[i]; 
  return result;
}



template <typename T>
Matrix<T> matrix_mult(const Matrix<T> &a, const Matrix<T> &b) 
{ 
  if (b.nz==1 && a.nz==1) {  //do proper 2D matrix multiplication
    Matrix<T> result;
    if (a.ny != b.nx) {
      logfile_error("matrix.h : matrix_mult : matrix size mismatch\na.size() = %u %u\nb.size() = %u %u\n",
		    a.nx,a.ny,b.nx,b.ny);
      return result;
    }
    result= matrix_create(a.nx,b.ny,1,0); 

    for (unsigned long int adx=0; adx<a.nx; ++adx) {
      for (unsigned long int ady=0; ady<a.ny; ++ady) {
	for (unsigned long int bdy=0; bdy<b.ny; ++bdy) {
	  result.M[bdy*a.nx+adx] += a.M[ady*a.nx+ adx] * b.M[bdy*b.nx+ ady]; 
	} } }
    return result;
  }
  else { //do element by element multiplication
    Matrix<T> result;
    if (b.M.size() != a.M.size() || a.nx != b.nx || a.ny != b.ny || a.nz != b.nz) {
      logfile_error("matrix.h : matrix_mult : matrix size mismatch\na.size() = %u %u %u\nb.size() = %u %u %u\n",
		    a.nx,a.ny,a.nz,b.nx,b.ny,b.nz);
      return result;
    }
    result= b;
    for (unsigned long int i=0; i<result.M.size(); ++i) result.M[i] *= a.M[i];
    return result;
  }
}

template <typename T>
Matrix<T> operator*(const Matrix<T> &a, const Matrix<T> &b) 
{ 
  Matrix<T> result;
  if (b.M.size() != a.M.size() || b.nz!=a.nz || a.ny != b.ny || a.nx != b.nx) {
    logfile_error("matrix.h : operator* : matrix size mismatch\na.size() = %u %u %u\nb.size() = %u %u %u\n",
		  a.nx,a.ny,a.nz,b.nx,b.ny,b.nz);
    return result;
  }
  result= matrix_create(b.nx,b.ny,b.nz,0); 
  //do element by element multiplication
  for (unsigned long int i=0; i<result.M.size(); ++i) result.M[i]=b.M[i]*a.M[i];
  return result;
}


template <typename T>
Matrix<T> matrix_dot(const Matrix<T> &a, const Matrix<T> &b) 
{ 
  return a*b;
}


template <typename T>
Matrix<T> operator/(const Matrix<T> &a, const Matrix<T> &b) 
{ 
  Matrix<T> result;
  if (b.M.size() != a.M.size() || b.nz!=a.nz || a.ny != b.ny || a.nx != b.nx) {
    logfile_error("matrix.h : operator/ : matrix size mismatch\na.size() = %u %u %u\nb.size() = %u %u %u\n",
		  a.nx,a.ny,a.nz,b.nx,b.ny,b.nz);
    return result;
  }
  result = a;
  for (unsigned long int i=0; i<result.M.size(); ++i) result.M[i] /= b.M[i];
  return result;
}


template <typename T>
std::vector<bool> operator<(const Matrix<T>& a, const Matrix<T>& b)
{    return a.M<b.M; }
template <typename T, typename T2>
std::vector<bool> operator<(const Matrix<T>& a, T2 b)
{    return a.M<b; }
template <typename T, typename T2>
std::vector<bool> operator<(T2 a, const Matrix<T>& b)
{    return a<b.M; }
template <typename T>
std::vector<bool> operator>(const Matrix<T>& a, const Matrix<T>& b)
{    return a.M>b.M; }
template <typename T, typename T2>
std::vector<bool> operator>(const Matrix<T>& a, T2 b)
{    return a.M>b; }
template <typename T, typename T2>
std::vector<bool> operator>(T2 a, const Matrix<T>& b)
{    return a>b.M; }

template <typename T>
unsigned long int size(const Matrix<T> f) {
  if (f.M.empty()) {
    logfile_printf("numel(x,y,z,f) = 0 0 0 0\n");
    return 0;
  }
  logfile_printf("numel(x,y,z,f) = %u %u %u %u\n",f.nx,f.ny,f.nz,numel(f.M));
  return f.M.size();
}



#endif  //MATRIX_H
