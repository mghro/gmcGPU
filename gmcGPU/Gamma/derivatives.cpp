#include "derivatives.h"

First_Derivatives<float>  derivatives_first(const Matrix<float> &f, const std::vector<float> &x, const std::vector<float> &y, 
					       const std::vector<float> &z)
{
  First_Derivatives<float>  deriv;
  unsigned long int sf1 = f.nx;
  unsigned long int sf2 = f.ny;
  unsigned long int sf3 = f.nz;
  unsigned long int sx = numel(x);
  unsigned long int sy = numel(y);
  unsigned long int sz = numel(z);
  if (sf1 != sx || sf2 != sy || sf3 != sz) {
    logfile_error("derivatives.cpp : derivatives_first : x,y,z need to be vectors of length given by size(f)\n");
    logfile_printf("numel(x,y,z,f) = %i %i %i %i\n",x.size(),y.size(),z.size(),numel(f.M));
    return deriv;
  }
  
  float hx = vector_util_step_size(x);
  float hy = vector_util_step_size(y);
  float hz = vector_util_step_size(z);

  if(sf1>=2)
    deriv.fx=differentiate_1d(f,hx,1);
  else
    matrix_create(deriv.fx,  f.nx,f.ny,f.nz,0);
  if(sf2>=2)
    deriv.fy=differentiate_1d(f,hy,2);
  else
    matrix_create(deriv.fy,  f.nx,f.ny,f.nz,0);
  if(sf3>=2)
    deriv.fz=differentiate_1d(f,hz,3);
  else
    matrix_create(deriv.fz,  f.nx,f.ny,f.nz,0);
    
  return deriv;  
}

Derivatives<float>  derivatives(const std::vector<float> &x, const std::vector<float> &y, 
				const std::vector<float> &z, const Matrix<float> &f)
{
  Derivatives<float>  deriv;

  unsigned long int sf1 = f.nx;
  unsigned long int sf2 = f.ny;
  unsigned long int sf3 = f.nz;
  unsigned long int sx = numel(x);
  unsigned long int sy = numel(y);
  unsigned long int sz = numel(z);
  if (sf1 != sx || sf2 != sy || sf3 != sz) {
    logfile_error("derivatives.cpp : derivatives : x,y,z need to be vectors of length given by size(f)\n");
    logfile_printf("numel(x,y,z,f) = %i %i %i %i\n",x.size(),y.size(),z.size(),numel(f.M));
    return deriv;
  }
  
  float hx = vector_util_step_size(x);
  float hy = vector_util_step_size(y);
  float hz = vector_util_step_size(z);

  if(sf1>=2)
    deriv.fx=differentiate_1d(f,hx,1);
  else
    matrix_create(deriv.fx,  f.nx,f.ny,f.nz,0);
  if(sf2>=2)
    deriv.fy=differentiate_1d(f,hy,2);
  else
    matrix_create(deriv.fy,  f.nx,f.ny,f.nz,0);
  if(sf3>=2)
    deriv.fz=differentiate_1d(f,hz,3);
  else
    matrix_create(deriv.fz,  f.nx,f.ny,f.nz,0);
  if(sf1>=2 && sf2>=2)
    deriv.fxy=differentiate_1d(deriv.fy,hx,1);
  else
    matrix_create(deriv.fxy,  f.nx,f.ny,f.nz,0);
   
  if(sf1>=2 && sf3>=2)
    deriv.fxz=differentiate_1d(deriv.fz,hx,1);    
  else
    matrix_create(deriv.fxz,  f.nx,f.ny,f.nz,0);
  if(sf2>=2 && sf3>=2)
    deriv.fyz=differentiate_1d(deriv.fy,hz,3);
  else
    matrix_create(deriv.fyz,  f.nx,f.ny,f.nz,0);
  if(sf1>=2 && sf2>=2 && sf3>=2)
    deriv.fxyz=differentiate_1d(deriv.fxy,hz,3);
  else
    matrix_create(deriv.fxyz,  f.nx,f.ny,f.nz,0);
  return deriv;
}


Matrix<float> differentiate_1d(const Matrix<float> &f, float h, int dim) 
{
  Matrix<float> dfdh;
  matrix_create(dfdh,f.nx,f.ny,f.nz,0);
  if (f.nx*f.ny*f.nz != f.M.size()) {
    logfile_error("derivatives.cpp : differentiate_1d : matrix improper format\n");
    logfile_printf("numel(x,y,z,f) = %i %i %i %i\n",f.nx,f.ny,f.nz,numel(f.M));
    return dfdh;
  }
  for (unsigned long int kk=0; kk<f.nz; ++kk) {
    for (unsigned long int jj=0; jj<f.ny; ++jj) {
      for (unsigned long int ii=0; ii<f.nx; ++ii) {
	unsigned long int ctr = kk*f.ny*f.nx+jj*f.nx+ii;
	if (dim==1) {
	  if (f.nx ==1)        dfdh.M[ctr] = 0;
	  else if (ii==0)      dfdh.M[ctr] = (f.M[ctr+1]-f.M[ctr])/h;
	  else if (ii==f.nx-1) dfdh.M[ctr] = (f.M[ctr]-f.M[ctr-1])/h;
	  else                 dfdh.M[ctr] = (f.M[ctr+1]-f.M[ctr-1])/2/h;
	}
	if (dim==2) {
	  if (f.ny ==1)        dfdh.M[ctr] = 0;
	  else if (jj==0)      dfdh.M[ctr] = (f.M[ctr+f.nx]-f.M[ctr])/h;
	  else if (jj==f.ny-1) dfdh.M[ctr] = (f.M[ctr]-f.M[ctr-f.nx])/h;
	  else                 dfdh.M[ctr] = (f.M[ctr+f.nx]-f.M[ctr-f.nx])/2/h;
	}
	if (dim==3) {
	  if (f.nz ==1)        dfdh.M[ctr] = 0;
	  else if (kk==0)      dfdh.M[ctr] = (f.M[ctr+f.nx*f.ny]-f.M[ctr])/h;
	  else if (kk==f.nz-1) dfdh.M[ctr] = (f.M[ctr]-f.M[ctr-f.nx*f.ny])/h;
	  else                 dfdh.M[ctr] = (f.M[ctr+f.nx*f.ny]-f.M[ctr-f.nx*f.ny])/2/h;
	}
	ctr +=1;
      }}}
  return dfdh;
}
