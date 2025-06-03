#include "voxels_less_than_1.h"

float voxels_less_than_1(const Matrix<float> &gamma, const Matrix<float> &dose, float thresh)
{
  float nvoxels = 0;
  
  if (
      gamma.M.size() != dose.M.size() ||
      gamma.nx       != dose.nx || 
      gamma.ny       != dose.ny || 
      gamma.nz       != dose.nz 
      )
    {
      logfile_error("voxels_less_than_1.cpp : voxels_less_than_1 : matrix improper format\n");
      size(gamma);
      size(dose);
      return nvoxels;
    }

  unsigned long int n1 = 0;
  unsigned long int n2 = 0;
  //  unsigned long int n3 = 0;
//  float mmax = 0;
  for (unsigned long int ii=0; ii<gamma.M.size(); ++ii) {
    if (fabs(gamma.M[ii])<=1 && dose.M[ii] > thresh  && !isnan(gamma.M[ii])) n1+=1;
    if (dose.M[ii] > thresh && !isnan(gamma.M[ii])) n2+=1;
    //if (dose.M[ii] > thresh) n2+=1;
    //  if(isnan(dose.M[ii])) n3++;
  }
  if (n2>0)
    nvoxels = float(n1)/ float(n2);
  //  logfile_printf("voxels %i / %i; size: %i\n", n1, n2, gamma.M.size());
  return nvoxels;  

}
  
