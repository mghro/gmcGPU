#include "tgif.h"

//Tricubic Gamma Index Function
//Matrix<float> tgif(vector<float> x0, vector<float> y0, vector<float> z0,  string unit0, 
//		   Matrix<float> &D0, string unitD0, 
//		   vector<float> x1, vector<float> y1, vector<float> z1,  string unit1, 
//		   Matrix<float> &D1, string unitD1,
//		   float d_gamma, string unit_d, float D_gamma_const, string unit_D, 
//		   float thresh, string unit_th, int Nmax) 
//{
//  Matrix<float> D_gamma;
//  matrix_create(D_gamma, D0.nx, D0.ny, D0.nz, D_gamma_const);
//  return tgif(x0, y0, z0, unit0, D0, unitD0,
//	      x1, y1, z1, unit1, D1, unitD1,
//	      d_gamma, unit_d, D_gamma, unit_D, 
//	      thresh, unit_th, Nmax);
//  
//}

void tgif(vector<float> x0, vector<float> y0, vector<float> z0,  string unit0, 
		   Matrix<float> D0, string unitD0, 
		   vector<float> x1, vector<float> y1, vector<float> z1,  string unit1, 
		   Matrix<float> D1, string unitD1,
		   float d_gamma, string unit_d, Matrix<float> D_gamma, string unit_D, 
		   float thresh, string unit_th, int Nmax, Matrix<float>& signed_gamma, SGammaResults* gammaResults)
{

  double t_start=double(clock())/(double)CLOCKS_PER_SEC;
 
  bool flag_1st_deriv;

  if (x1.size()!=D1.nx || y1.size()!=D1.ny || z1.size() != D1.nz 
      || D1.M.size() != D1.nx*D1.ny*D1.nz || D1.M.size()==0){
    logfile_error("tgif.cpp : tgif : incorrect format for x1,y1,z1,D1 dose cube\n");
    size(x1);
    size(y1);
    size(z1);
    size(D1);
    return;
  }
  if (x0.size()!=D0.nx || y0.size()!=D0.ny || z0.size() != D0.nz 
      || D0.M.size() != D0.nx*D0.ny*D0.nz || D0.M.size()==0){
    logfile_error("tgif.cpp : tgif : incorrect format for x0,y0,z0,D0 dose cube\n");
    size(x0);
    size(y0);
    size(z0);
    size(D0);
    return;
  }


  //convert spatial units to [mm]
  if (unit0=="cm") {
    x0=10.0*x0; y0=10.0*y0;  z0=10.0*z0;}
  else if (unit0 == "m"){
    x0=1000.0*x0; y0=1000.0*y0;  z0=1000.0*z0; }
  else if (unit0 == "in"){
    x0=25.4*x0; y0=25.4*y0;  z0=25.4*z0; }
  else if (unit0 != "mm") {
    logfile_error("tgif.cpp : tgif : unit0 units \"%s\" are not supported\n", unit0.c_str());
    logfile_printf("'mm', 'cm', 'm', or 'in'\n");
    return;}
  
  if (unit1 == "cm"){
    x1=10.0*x1; y1=10.0*y1;  z1=10.0*z1;}
  else if (unit1 == "m"){
    x1=1000.0*x1; y1=1000.0*y1;  z1=1000.0*z1; }
  else if (unit1 == "in"){
    x1=25.4*x1; y1=25.4*y1;  z1=25.4*z1; }
  else if (unit1 != "mm"){
    logfile_error("tgif.cpp : tgif : unit1 units \"%s\" are not supported\n", unit1.c_str());
    logfile_printf("'mm', 'cm', 'm', or 'in'\n");
    return;}
  
  if (unit_d == "cm")
    d_gamma =10.0*d_gamma;
  else if (unit_d == "m")
    d_gamma =1000.0*d_gamma;
  else if (unit_d == "in")
    d_gamma =25.4*d_gamma;
  else if (unit_d != "mm"){
    logfile_error("tgif.cpp : tgif : unit_d units \"%s\" are not supported\n", unit_d.c_str());
    logfile_printf("'mm', 'cm', 'm', or 'in'\n");
    return;}

  //convert dose units to [Gy]
  if (unitD0 == "cGy")
    D0 = D0/100.0;
  else if (unitD0 == "cGyRBE")
    D0 = D0/110.0;
  else if (unitD0 == "GyRBE")
    D0 = D0/1.1;
  else if (unitD0 != "Gy"){
    logfile_error("tgif.cpp : tgif : unitD0 units \"%s\" are not supported\n", unitD0.c_str());
    logfile_printf("'cGy', 'cGyRBE', 'Gy' 'GyRBE'\n");
    return;}

  if (unitD1 == "cGy")
    D1 = D1/100.0;
  else if (unitD1 == "cGyRBE")
    D1 = D1/110.0;
  else if (unitD1 == "GyRBE")
    D1 = D1/1.1;
  else if (unitD1 != "Gy"){
    logfile_error("tgif.cpp : tgif : unitD1 units \"%s\" are not supported\n", unitD1.c_str());
    logfile_printf("'cGy', 'cGyRBE', 'Gy' 'GyRBE'\n");
    return;}

  if (unit_D == "cGy")
    D_gamma = D_gamma/100.0;
  else if (unit_D == "cGyRBE")
    D_gamma = D_gamma/110.0;
  else if (unit_D == "GyRBE")
    D_gamma = D_gamma/1.1;
  else if (unit_D == "%D0")
    D_gamma = D_gamma*D0/100.0;
  else if (unit_D == "%D1max")
    D_gamma = D_gamma*vector_util_max(D1.M)/100.0;
  else if (unit_D != "Gy"){
    logfile_error("tgif.cpp : tgif : unit_D units \"%s\" are not supported\n", unit_D.c_str());
    logfile_printf("'cGy', 'cGyRBE', 'Gy' 'GyRBE', '%D0', or '%D1max'\n");
    return;}

  if (unit_th == "cGy")
    thresh = thresh/100.0;
  else if (unit_th == "cGyRBE")
    thresh = thresh/110.0;
  else if (unit_th == "GyRBE")
    thresh = thresh/1.1;
  else if (unit_th == "%D1max")
    thresh = thresh*vector_util_max(D1.M)/100.0;
  else if (unit_th != "Gy"){
    logfile_error("tgif.cpp : tgif : unit_th units \"%s\" are not supported\n", unit_th.c_str());
    logfile_printf("'cGy', 'cGyRBE', 'Gy' 'GyRBE', or '%D1max'\n");
    return;}

  //crop the (x1,y1,z1) grid to save computation time for thse values that
  //   (a) far exceeds the data in the (x0,y0,z0) grid, or,
  //   (b) far exceeds the mask (which is specified by the (x1,y1,z1) grid)
  //the bounding box function modifies x1,y1,z1,D1
  if(Nmax>0)
    bounding_box(x0,y0,z0, x1,y1,z1, D1, thresh, d_gamma); //d_gamma used for grid buffer
  else
    bounding_box(x0,y0,z0, x1,y1,z1, D1, thresh, 0);  //first order method (no buffer needed)


  if (x1.size()==0 || y1.size()==0 || z1.size() ==0 || D1.M.size()==0){
    logfile_error("tgif.cpp : tgif : x1,y1,z1,D1 dose cube is empty\n");
    return;
  }
  if (x0.size()==0 || y0.size()==0 || z0.size() ==0 || D0.M.size()==0){
    logfile_error("tgif.cpp : tgif : x0,y0,z0,D0 dose cube is empty\n");
    return;
  }


  float accuracy_factor = 1.2;  //1 = nominal, 10 = better accuracy

  //Let's get down to business
  x0 = x0/d_gamma;
  y0 = y0/d_gamma;
  z0 = z0/d_gamma;
  x1 = x1/d_gamma;
  y1 = y1/d_gamma;
  z1 = z1/d_gamma;
  float nvoxels =0;
  Mesh_Grid<float> mesh0 = meshgrid(x0, y0, z0);
  Search_Matrices<float> sm;
  float *alpha;
  unsigned long int size_alpha = 0;
  // Next section is to get D1 and 1st-order gradients interpolated on x0,y0,z0 grid
  // The results are called D1_interp and fx, fy, fz
  size_alpha = D1.M.size()*64;
  try {
    alpha = new float[size_alpha]; //this can be VERY large
  }  catch (...) {
    logfile_error("tgif.cpp : tgif : Cannot allocate space for an alpha matrix of size %u\n", size_alpha);
    return; }
  //tricubic spline interpolation
  //cout<<"Begin alpha, time =  " << double(clock())/(double)CLOCKS_PER_SEC-t_start <<" s"<<endl;
  //This function modifies alpha
  tricubic_alpha(alpha,size_alpha,x1,y1,z1,D1,t_start);  //Perform this calculation for alpha once only
  //     for (unsigned long int tmp =0; tmp<size_alpha; ++tmp)
  //       cout<<alpha[tmp]<<' '<<endl;
  if (logfile_success() == false) {
    logfile_error("tgif.cpp : tgif : alpha was not successfully created\n");
    return; 
  }
  //cout<<"End alpha, time =  " << double(clock())/(double)CLOCKS_PER_SEC-t_start <<" s"<<endl;
  flag_1st_deriv=true;
  sm =LM_cubicspline(x1, y1, z1, mesh0, alpha, size_alpha, flag_1st_deriv, t_start);
  //cout<<"End tricubic interpolation, time =  " << double(clock())/(double)CLOCKS_PER_SEC-t_start <<" s"<<endl;
  if (sm.D1_interp.M.size() <1) { 
    logfile_error("tgif.cpp : tgif : interpolated matrices have zero size\n"); 
    return; }


  if (logfile_success() == false) { 
    logfile_error("tgif.cpp : tgif : failure encountered after interpolation\n"); 
    return; }
  Matrix<float> D1_interp = sm.D1_interp;
  Matrix<float> f0 = (D1_interp-D0)/D_gamma;
  bool flag_zeroth_order = false;
  if (logfile_success() == false) { 
    logfile_error("tgif.cpp : tgif : failure encountered after creating the normalized function f0\n"); 
    return; }

  if (flag_zeroth_order ) {
    signed_gamma=f0;
    //calculate fraction of voxels with |gamma| <= 1 within the region of interest
    nvoxels = voxels_less_than_1(signed_gamma, D1_interp, thresh);
    logfile_printf("gamma_criterion_0th_order = %f\n", nvoxels);
#ifdef TIME_COUT
    logfile_printf("time =  %f s\n", 
		   double(clock())/(double)CLOCKS_PER_SEC-t_start);
#endif
  }
  std::vector<float> fx = sm.fx.M/D_gamma.M;
  std::vector<float> fy = sm.fy.M/D_gamma.M;
  std::vector<float> fz = sm.fz.M/D_gamma.M;
  std::vector<bool> mask = D1_interp>thresh;

  //First order solution
  //%%%%%%%%%%%%%%%%%%%%%%
  //calculate gamma using taylor series for f
  signed_gamma.M = f0.M/vector_util_sqrt(1+fx*fx+fy*fy+fz*fz);
  signed_gamma.nx = f0.nx;
  signed_gamma.ny = f0.ny;
  signed_gamma.nz = f0.nz;
  unsigned long int numel_all_ind =vector_util_count_true( mask);
// calculate fraction of voxels with |gamma| <= 1 within the region of interest

  nvoxels = voxels_less_than_1(signed_gamma, D1_interp, thresh);
  logfile_printf("gamma_criterion_1st_order = %f \n", nvoxels);
  if (Nmax <=0) {  //first order
    if (size_alpha>0)
      delete [] alpha;
    return;
  }

  //Iteration method (apply 1st order method multiple times)
  //%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  if (Nmax>0){
    signed_gamma = abs(signed_gamma);
    std::vector<bool> ind =  mask;  //indeces of voxels that need the iteration method.  numel(ind)=numel(D0)
    std::vector<float> c1= signed_gamma.M*signed_gamma.M/f0.M;  //numel(c1)=numel(D0)
    vector_util_set(c1,f0.M==0,0);
    //x,y,z contain info for only those voxels that need to be iterated
    std::vector<float> x=-1*fx*c1; // numel(x) = numel(D0)
    std::vector<float> y=-1*fy*c1;
    std::vector<float> z=-1*fz*c1;
    float convergence = 1-float(vector_util_count_true(ind))/float(numel_all_ind);

    std::vector<bool> lt_ind=ind; //numel(lt_index)=numel(D0)
    //maintain the search volume to a reasonable size
    //if x>2 then this voxel automatically has gamma>2 
    x=vector_util_max(x,-2 );
    x=vector_util_min(x,2 );
    y=vector_util_max(y,-2 );
    y=vector_util_min(y,2 );
    z=vector_util_max(z,-2 );
    z=vector_util_min(z,2 );

    std::vector<float>  prev_x = x;
    std::vector<float>  prev_y = y;
    std::vector<float>  prev_z = z;

    int N=0;
    while( convergence<(1-1/(100*accuracy_factor)) && N<Nmax) {
      N += 1;
      //always tricubic spline interpolation for iteration method
      flag_1st_deriv=true;
      //use the existing alpha (poly coefficients and extract values on the surface at the new grid points)
#ifdef TIME_COUT
      logfile_printf(" time tgif 1 =  %f", double(clock())/(double)CLOCKS_PER_SEC-t_start );
#endif
      sm =LM_cubicspline(x1, y1, z1, 
			  vector_util_get(mesh0.x.M,ind)+vector_util_get(x,ind), 
			  vector_util_get(mesh0.y.M,ind)+vector_util_get(y,ind), 
			  vector_util_get(mesh0.z.M,ind)+vector_util_get(z,ind), 
			  alpha, size_alpha, flag_1st_deriv, t_start);
      if (sm.D1_interp.M.size() <1) {
	logfile_error("tgif.cpp : tgif : interpolated matrices have zero size during iterations\n");
	return;
      }
#ifdef TIME_COUT
      logfile_printf(" time tgif 2 =  %f", double(clock())/(double)CLOCKS_PER_SEC-t_start );
#endif
      Matrix<float> prev_gamma=signed_gamma;
#ifdef TIME_COUT
      logfile_printf(" time tgif 3 =  %f", double(clock())/(double)CLOCKS_PER_SEC-t_start );
#endif
       // numel(f) is less than numel(D0)
      std::vector<float>  f = (sm.D1_interp.M-vector_util_get(D0.M,ind))/vector_util_get(D_gamma.M,ind);
      std::vector<bool> prev_ind = ind;
      lt_ind = ind;
      unsigned long int jj=0;  //of the subset of voxels that are iterated, this is the jj'th iterated voxel
      for (unsigned long int ii=0; ii<signed_gamma.M.size(); ++ii) { //in the set of all voxels, this is the ii'th voxel
	//numel(ind)=numel(D0)
	if (ind[ii]) {
	  //New gamma in the iteration.  Note that it is based on the spline
	  //interpolated function value at that point
	  //Also note that this is not the signed gamma yet
	  signed_gamma.M[ii] = sqrt(f[jj]*f[jj] + x[ii]*x[ii] + y[ii]*y[ii] + z[ii]*z[ii]);
	  //numel(ind) = numel(lt_ind) = numel(D0)
	  if(N>1) {
	    //update "ind" the list of voxels that need to be iterated
	    ind[ii] = lt_ind[ii]==false ||abs(signed_gamma.M[ii]/prev_gamma.M[ii]-1)>0.0025/accuracy_factor;
	    //update "lt_ind" the list of voxels that had a good move the last time through and still need iterations
	    lt_ind[ii] = signed_gamma.M[ii]<=prev_gamma.M[ii];
	  }

	  //	cout<<(signed_gamma.M[ii]/prev_gamma.M[ii]-1)<<',';
	  if (lt_ind[ii]) {  //we had a good move the last time through, calculate the next step
	    //numel(sm.fx) is less than numel(D0) it was just recalculated for only iterated voxels
	    fx[ii] = sm.fx.M[jj]/D_gamma.M[ii]; //numel(fx) = numel(D0)
	    fy[ii] = sm.fy.M[jj]/D_gamma.M[ii];
	    fz[ii] = sm.fz.M[jj]/D_gamma.M[ii];
	    // numel(f) is less than numel(D0)
	    // numel(x), numel(y) and numel(z), numel(fx), numel(fy) and numel(fz) equal numel(D0)
	    float t1=-(f[jj]-x[ii]*fx[ii]-y[ii]*fy[ii]-z[ii]*fz[ii])
	      /(1+fx[ii]*fx[ii]+fy[ii]*fy[ii]+fz[ii]*fz[ii]);
	    //numel(prev_x) = numel(x)
    prev_x[ii] = x[ii];
	    prev_y[ii] = y[ii];
	    prev_z[ii] = z[ii];
	    x[ii]=0.25*prev_x[ii]+0.75*fx[ii]*t1;   //new coordinates in the iteration
	    y[ii]=0.25*prev_y[ii]+0.75*fy[ii]*t1;
	    z[ii]=0.25*prev_z[ii]+0.75*fz[ii]*t1;
	  }
	  else { // we had a bad move, go back half a step
	    x[ii]= (prev_x[ii]+x[ii])/2;
	    y[ii]= (prev_y[ii]+y[ii])/2;
	    z[ii]= (prev_z[ii]+z[ii])/2;
	    signed_gamma.M[ii] = prev_gamma.M[ii];
	  }
	  if(isnan(x[ii])) x[ii] =prev_x[ii];  //we did a really bad move , go back
	  if(isnan(y[ii])) y[ii] =prev_y[ii];
	  if(isnan(z[ii])) z[ii] =prev_z[ii];
	  jj +=1; //the jj'th voxel in the subset of iterated voxels
	} //end of if(ind[ii])
      } //end looping over all voxels (ii index)
      convergence = 1-float(vector_util_count_true(ind))/float(numel_all_ind);
      //cout<<float(vector_util_count_true(ind))<<' '<<float(numel_all_ind)<<endl;
      //calculate fraction of voxels with |gamma| <= 1 within the region of interest
      nvoxels = voxels_less_than_1(signed_gamma, D1_interp, thresh);
      logfile_printf("gamma_criterion_iter %i = %f   fraction converged = %f\n", 
		     N, nvoxels, convergence);
#ifdef TIME_COUT
    logfile_printf("time =  %f s\n", 
		   double(clock())/(double)CLOCKS_PER_SEC-t_start);
#endif
      if (N==Nmax-1) {
	logfile_warn("tgif.cpp : tgif : Gamma computation reached maximum number of iterations (Nmax=%i)\n", Nmax);
      }
    } //end looping over iterations
    //signed_gamma = sign(D1_interp - D0)*abs(signed_gamma);
    signed_gamma = abs(signed_gamma);

    gammaResults->numIterations     = Nmax;
    gammaResults->fractionConverged = nvoxels;
    gammaResults->convergenceFactor = convergence;
  } // if (Nmax>1)

  // cout<<"Gamma pass rate = "<<100*nvoxels<<"%"<<endl;
  if (size_alpha>0)
    delete [] alpha;

   vector_util_set(signed_gamma.M, vector_util_not(mask), 0.0);

   //Added for Joost's Gamma Gui test
   vector_util_set(signed_gamma.M, vector_util_isnan(signed_gamma.M)&& mask, 2);
   
   return;

}
