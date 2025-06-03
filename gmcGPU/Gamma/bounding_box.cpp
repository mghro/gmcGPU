#include "bounding_box.h"

void bounding_box( const vector<float> x0, const vector<float> y0, const vector<float> z0,
		   vector<float> &x1, vector<float> &y1, vector<float> &z1,
		   Matrix<float> &D1, float thresh, float d_gamma) {
  
  //Crop the data on the x1,y1,z1 grid to the extreme values in the
  //x0,y0,z0 grid with a padding of 2 indeces and 2*dd on all sides
  

  float dd=abs(d_gamma);
  
  if (x1.size() != D1.nx || y1.size() != D1.ny || z1.size() != D1.nz || D1.nx*D1.ny*D1.nz != D1.M.size()) {
    logfile_error("bounding_box.cpp : bounding_box : Incorrectly formed x1,y1,z1,D1 dose distribution  %u %u %u %u %u %u %u\n", 
		  x1.size(), y1.size(), z1.size(), 
		  D1.nx, D1.ny, D1.nz, D1.M.size()); 
    x1.clear();
    y1.clear();
    z1.clear();
    D1.M.clear();
    D1.nx=0;
    D1.ny=0;
    D1.nz=0;
    return ;
  }
  if (vector_util_max(D1.M) <= thresh) {
    logfile_error("bounding_box.cpp : bounding_box : Maximum dose in the D1 matrix [%f] does not exceed threshod [%f]\n",
		  vector_util_max(D1.M), thresh); 
    x1.clear();
    y1.clear();
    z1.clear();
    D1.M.clear();
    D1.nx=0;
    D1.ny=0;
    D1.nz=0;
    return ;
    
  }

  float dx = fabs(vector_util_step_size(x1)) ;
  float dy = fabs(vector_util_step_size(y1)) ;
  float dz = fabs(vector_util_step_size(z1)) ;
  
 
  std::vector<bool> mask= D1.M >= thresh;
  std::vector<unsigned long int> ix, iy, iz;
  for (unsigned long int idz=0; idz<D1.nz;++idz) {
    for (unsigned long int idy=0; idy<D1.ny;++idy) {
      for (unsigned long int idx=0; idx<D1.nx;++idx) {
 	if (mask.at(idz*D1.ny*D1.nx+idy*D1.nx+idx)) {
 	  ix.push_back(idx);
 	  iy.push_back(idy);
 	  iz.push_back(idz);
 	}}}}
  
  float min_x = fmax(vector_util_min(x0),vector_util_min(vector_util_get(x1,ix)));
  float max_x = fmin(vector_util_max(x0),vector_util_max(vector_util_get(x1,ix)));
  float min_y = fmax(vector_util_min(y0),vector_util_min(vector_util_get(y1,iy)));
  float max_y = fmin(vector_util_max(y0),vector_util_max(vector_util_get(y1,iy)));
  float min_z = fmax(vector_util_min(z0),vector_util_min(vector_util_get(z1,iz)));
  float max_z = fmin(vector_util_max(z0),vector_util_max(vector_util_get(z1,iz)));
  
  //2015 (pre N. Depauw)
  // std::vector<bool> x_used = x1>=(vector_util_min(x0)-2*dx-2*dd) && x1<=(vector_util_max(x0)+2*dx+2*dd);
  // std::vector<bool> y_used = y1>=(vector_util_min(y0)-2*dy-2*dd) && y1<=(vector_util_max(y0)+2*dy+2*dd);
  // std::vector<bool> z_used = z1>=(vector_util_min(z0)-2*dz-2*dd) && z1<=(vector_util_max(z0)+2*dz+2*dd);

  //2016 (includes N. Depauw request)
  std::vector<bool> x_used = x1>=(min_x-2*dx-2*dd) && x1<=(max_x+2*dx+2*dd);
  std::vector<bool> y_used = y1>=(min_y-2*dy-2*dd) && y1<=(max_y+2*dy+2*dd);
  std::vector<bool> z_used = z1>=(min_z-2*dz-2*dd) && z1<=(max_z+2*dz+2*dd);


  if(vector_util_sum(x_used)*vector_util_sum(y_used)*vector_util_sum(z_used)==0) {
    logfile_error("bounding_box.cpp : bounding_box : the (x1,y0,z0) and (x1,y1,z1) grids do not overlap\n");
    x1.clear();
    y1.clear();
    z1.clear();
    D1.M.clear();
    D1.nx=0;
    D1.ny=0;
    D1.nz=0;
    return ;
  }
  x1=vector_util_get(x1, x_used);
  y1=vector_util_get(y1, y_used);
  z1=vector_util_get(z1, z_used);
  D1=matrix_get(D1,vector_util_find(x_used),vector_util_find(y_used),vector_util_find(z_used));
  
  return ;
}
