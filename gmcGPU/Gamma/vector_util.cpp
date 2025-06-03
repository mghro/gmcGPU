#include "vector_util.h"





//linear interpolation of (xi,yi) on the xf data point
//The xi vector must be equally spaced 
double linear_interp(const std::vector<double> &xi, const std::vector<double> &yi,double xf) {
  std::vector<double> vE(1,xf); //a vector with only 1 element
  vE=linear_interp(xi, yi,vE);
  if (vE.size() == 0)
    logfile_error("vector_util.cpp : linear_interp : size of vector is zero\n"); 
  return vE.at(0);
}

//linear interpolation of (xi,yi) on the xf data points
//The xi vector must be monotonic increasing or decreasing 
//xi spacing need not be uniformly spaced
std::vector<double> linear_interp(const std::vector<double> &xi, const std::vector<double> &yi,const std::vector<double> &xf) {

  std::vector<double> yf; 
  if (xi.size() != yi.size()  || xf.size() == 0 || xi.size() == 0) {
    logfile_error("vector_util.cpp : linear_interp : vector size incorrect %u %u %u\n", xi.size(), yi.size(),  xf.size()); 
    return yf;
  }

  if (xi.size() ==1) {
    for (unsigned long int jj=0; jj<xf.size(); ++jj)
      yf.push_back(xi.at(0));
    return yf;
  }
  double del = (xi.at(xi.size()-1)-xi[0])/double(xi.size()-1);
  for (unsigned long int ii=1; ii<xi.size(); ++ii) {
    if (del * (xi[ii]-xi[ii-1])<= 0) {
      logfile_error("vector_util.cpp : linear_interp : xi must be monotonically increasing or decreasing\n");
      vector_util_print(xi);
      return yf;
    }
  }
  if (del>0) {
    for (unsigned long int jj=0; jj<xf.size(); ++jj) {
      bool success = false;
      for (unsigned long int ii=1; ii<xi.size() && success == false; ++ii) {
	if (xf[jj]< xi[ii] || ii == (xi.size()-1) ){
	  yf.push_back(yi[ii-1] + (xf[jj]-xi[ii-1])*(yi[ii]-yi[ii-1])/(xi[ii]-xi[ii-1])  );
	  success = true;	
	}
      }
      if (success == false) {
	yf.clear();
	logfile_error("vector_util.cpp : linear_interp : failed to interpolate %f\n",  xf[jj]);
 	return yf;
      }
    }
  } //end of checking if del is greater than zero
  else {  //del is less than zero
    del = -del;
    for (unsigned long int jj=0; jj<xf.size(); ++jj) {
      bool success = false;
      for (unsigned long int ii=1; ii<xi.size() && success == false; ++ii) {
	if (xf[jj]> xi[ii]  || ii == xi.size()-1 ){
	  yf.push_back(yi[ii-1] + (xf[jj]-xi[ii-1])*(yi[ii]-yi[ii-1])/(xi[ii]-xi[ii-1])  );
	  success = true;	
	}
      }
      if (success == false) {//failed to linear interpolate this xf[jj]
	yf.clear();
	logfile_error("vector_util.cpp : linear_interp : failed to interpolate %f\n",  xf[jj]); 	
	return yf;
      }
    }
  } 
  return yf;

}





// //Recursive BSPLINE from GSHARP 2012
// //The xi vector must be equally spaced
// std::vector<double> bspline(std::vector<double> xi, std::vector<double> yi,std::vector<double> xf) {
//   if (xi.size() != yi.size()  || xf.size() == 0) { 
//     std::cout<<"ERROR: vector_util_bspline "<<xi.size()<<" "<< yi.size()<<std::endl;
//     std::vector<double> yf; 
//     return yf;}

//   // Some setup
//   int intK = int(yi.size());
//   double K = double(intK);
//   double alpha = -0.2679;
//   std::vector<double> dplus = yi*0;
//   std::vector<double> c = yi*0;

//   // Compute coefficient values
//   dplus.at(0)=fsum(pow(alpha,intK-1)*yi);
//   for (int k=1; k<intK; ++k) 
//     dplus.at(k) = 6*yi.at(k) + alpha * dplus.at(k-1);

//   c[intK-1] = - alpha * (1 - alpha*alpha) * (2 * dplus.at(intK-1) - 6*yi.at(intK-1));
  
//   for (int k=intK-2; k>=0; k=k-1)
//     c[k] = alpha * (c.at(k+1) - dplus.at(k+1));

//   double del = (fmax(xi)-fmin(xi))/(K-1);
//   std::vector<double> xf_unitless = 1+(xf-fmin(xi))/del;
//   std::vector<double> yf=xf*0;
//   for (int i=0; i<int(xf_unitless.size());++i) {
//     int kmin = int(fmax(1,floor(xf_unitless.at(i)) - 2))-1;

//     if (kmin>intK-4) 
//       kmin=intK-4;
    
//     double xoff = xf_unitless.at(i) - floor(xf_unitless.at(i));
//     if (xf_unitless.at(i)<3)
//       xoff=xf_unitless.at(i)-3;
//     cout<<xf.at(i)<< "  "<<xoff<<endl;
//     yf[i] = (1/6)*(
// 		   (-pow(xoff,3)+3*pow(xoff,2)-3*xoff+1)*c.at(kmin)+(3*pow(xoff,3)-6*pow(xoff,2)+4)*c.at(kmin+1)+
// 		   (-3*pow(xoff,3)+3*pow(xoff,2)+3*xoff+1)*c.at(kmin+2)+pow(xoff,3)*c.at(kmin+3)  );
//   }

//   for (int ii=0; ii<int(xi.size()); ++ii)  
//     std::cout<<xi.at(ii)<<','<<yi.at(ii)<<std::endl;
//   std::cout<<std::endl;
//   for (int ii=0; ii<int(xf.size()); ++ii)  
//     std::cout<<xf.at(ii)<<','<<yf.at(ii)<<std::endl;

//   return yf;

// }



//Ben Clasie 11/19/09
//
//Calculates the weights of a 1d gaussian function on a grid
//weights= gauss_grid(grid,A,sigma,x0)
//
//grid is a 1d array with values grid points representing the 
//centers of the grid. It does not need to be regularly spaced
//
//The Gaussian is A*1/sqrt(2*pi*sigma^2)*exp(-grid^2/2/sigma^2)
//The weights are calculated using the integral of the Gaussian over the
//grid area, resulting in error functions in the calculation.
std::vector<double> gauss_grid(const std::vector<double> &grid, double A, double sigma, double x0)
{
  std::vector<double> weights;
  if (grid.size()<3) {
    logfile_error("vector_util.cpp : gauss_grid : fewer than 3 grid points %u\n",  grid.size()); 	
    return weights;
  }
  weights.reserve(grid.size());
  double xlo, xhi;
  double sqrt_two_sigma_sq = sqrt(2*sigma*sigma);
  double area = 0;
  //grid points represent the centers of grid
  for (unsigned long int ii=0; ii<grid.size(); ++ii) {
    if (ii==0)
      xlo = grid[ii]-(grid[ii+1]-grid[ii])/2;
    else
      xlo = (grid[ii]+grid[ii-1])/2;
    if (ii==grid.size()-1)
      xhi = grid[ii]+(grid[ii]-grid[ii-1])/2;
    else
      xhi = (grid[ii]+grid[ii+1])/2;
    double tlo = (xlo-x0)/sqrt_two_sigma_sq;
    double thi = (xhi-x0)/sqrt_two_sigma_sq;

    if ( fabs(tlo)<6 || fabs(thi)<6) {
      //Use this for A = maximum of gaussian
      //weights.push_back(A*sqrt(pi/2.0)*sigma*(erf(thi)-erf(tlo)));
      
      //use this for A*normalized_gaussian
      weights.push_back(A/2*fabs(erf(thi)-erf(tlo)));
      area = area+weights[ii];
    }
    else
      weights.push_back(0);
  }
  return weights;
}

std::vector<unsigned long int> vector_util_find(const std::vector<bool>& a)
{
  std::vector<unsigned long int> result;
  for (unsigned long int ii=0; ii<a.size(); ++ii) {
    if (a[ii]) result.push_back(ii);
  }
  return result;
}

unsigned long int vector_util_count_true(const std::vector<bool> &a)
{
  unsigned long int ctr=0;
  for (unsigned long int ii=0; ii<a.size(); ++ii) {
    if(a[ii]) ctr +=1;
  }
  return ctr;
}


std::vector<bool> vector_util_not(const std::vector<bool> &a)
{
    std::vector<bool> result;
    result.reserve(a.size());
    for (unsigned long int ii=0; ii<a.size(); ++ii) result.push_back(!a[ii]);
    return result;
}
