#include "dose_grid_compare.h"

void dose_grid_compare(DOSE_GRID &dose_grid_0, string unit0, string unitD0,
		       DOSE_GRID &dose_grid_1, string unit1, string unitD1,
		       float d_gamma, string unit_d, float D_gamma, string unit_D, 
		       float thresh, string unit_th) 

{
  
  if (dose_grid_0.D.size() != dose_grid_0.xg.size()*dose_grid_0.yg.size()*dose_grid_0.zg.size() ) {
    logfile_error("dose_grid_compare.cpp : dose_grid_compare : Input dose_grid_0 has incorrect format %i %i %i %i\n",dose_grid_0.D.size(),dose_grid_0.xg.size(),dose_grid_0.yg.size(),dose_grid_0.zg.size() );
    return;
  }
  if (dose_grid_1.D.size() != dose_grid_1.xg.size()*dose_grid_1.yg.size()*dose_grid_1.zg.size() ) {
    logfile_error("dose_grid_compare.cpp : dose_grid_compare : Input dose_grid_1 has incorrect format %i %i %i %i\n",dose_grid_1.D.size(),dose_grid_1.xg.size(),dose_grid_1.yg.size(),dose_grid_1.zg.size() );
    return;
  }

   GMatrix<float> D0, D1;
   D0.M= vector_util_float(dose_grid_0.D); 
   D1.M= vector_util_float(dose_grid_1.D);
   D0.nx =dose_grid_0.xg.size();
   D0.ny =dose_grid_0.yg.size();
   D0.nz =dose_grid_0.zg.size();
   D1.nx =dose_grid_1.xg.size();
   D1.ny =dose_grid_1.yg.size();
   D1.nz =dose_grid_1.zg.size();

   GMatrix<float> gamma = tgif(
	vector_util_float(dose_grid_0.xg), vector_util_float(dose_grid_0.yg), vector_util_float(dose_grid_0.zg), unit0, 
	D0, unitD0,
	vector_util_float(dose_grid_1.xg), vector_util_float(dose_grid_1.yg), vector_util_float(dose_grid_1.zg), unit1,
	D1, unitD1,
	d_gamma, unit_d, D_gamma, unit_D, 
	thresh, unit_th, db_Nmax);
   if (logfile_success() == false) {
     logfile_error("dose_grid_compare.cpp : dose_grid_compare : exiting due to previous errors\n");
     return;
   }
     
   unsigned long int nslices = 11;
   std::vector<unsigned long int> numerator, denominator;
   vector_util_create(numerator,nslices+1,0);
   vector_util_create(denominator,nslices+1,0);
   std::vector<float> zslice, pass_rate;

   //calculate slice pass rates
   float dz = (vector_util_max(dose_grid_0.zg)-vector_util_min(dose_grid_0.zg)) /float(nslices + 1);
   for (unsigned long int slice_ctr=0; slice_ctr<nslices; ++slice_ctr) {
     unsigned long int kk = find_closest(dose_grid_0.zg, vector_util_min(dose_grid_0.zg)+float(slice_ctr+1)*dz);  //index of slice
     zslice.push_back(dose_grid_0.zg.at(kk));
     for (unsigned long int ii=0; ii<gamma.nx; ++ii) {
       for (unsigned long int jj=0; jj<gamma.ny; ++jj) {
	 float g= matrix_get(gamma, ii, jj, kk);
	 if (g != -10000) denominator.at(slice_ctr) +=1; {
	   if ( fabs(g)<=1) numerator.at(slice_ctr) +=1;
	 }
       } 
     }
   }

   //calculate total pass rate
   for (unsigned long int ii=0; ii<gamma.M.size(); ++ii) {
     float g = gamma.M.at(ii);
     if (g != -10000) denominator.at(nslices) +=1; {
       if ( fabs(g)<=1) numerator.at(nslices) +=1;
     }
   }
   pass_rate = 100*vector_util_float(numerator)/vector_util_float(denominator);
//     vector_print(numerator);
//     vector_print(denominator);
//     vector_print(pass_rate);

   logfile_printf("Gamma( %.2f %s %.2f %s )\n", d_gamma, unit_d.c_str(), D_gamma, unit_D.c_str());

   logfile_printf("Z=\t");
   for (unsigned int slice_ctr=0; slice_ctr<nslices; ++slice_ctr) 
     logfile_printf("%.2f,\t", zslice.at(slice_ctr));
   logfile_printf("ALL VOXELS\n");

//    logfile_printf("N=\t");
//    for (unsigned long int slice_ctr=0; slice_ctr<nslices; ++slice_ctr) 
//      logfile_printf("%d,\t", denominator.at(slice_ctr));
//    logfile_printf("%d\n",denominator.at(nslices));

   logfile_printf("%%=\t");
   bool gamma_success = true;
   for (unsigned long int slice_ctr=0; slice_ctr<nslices; ++slice_ctr) {
     logfile_printf("%.2f,\t", pass_rate.at(slice_ctr));
     if (pass_rate.at(slice_ctr) < db_gamma_pass_rate) gamma_success = false;
   }
   logfile_printf("%.2f\n",pass_rate.at(nslices));
   if (pass_rate.at(nslices) < db_gamma_pass_rate) gamma_success = false;
   if (gamma_success == false)
     logfile_error("dose_grid_compare.cpp : dose_grid_compare : Gamma index pass rate below database specified value of %.1f %%D1max\n", 
		   db_gamma_pass_rate);
 
  //calculate dose difference between the two dose grids
   if (dose_grid_0.xg.size() != dose_grid_1.xg.size()
       || dose_grid_0.yg.size() != dose_grid_1.yg.size()
       || dose_grid_0.zg.size() != dose_grid_1.zg.size()
       || vector_util_count_true(
          vector_util_abs(dose_grid_0.xg-dose_grid_1.xg)<=db_small_distance
                             ) != dose_grid_0.xg.size()
       || vector_util_count_true(
           vector_util_abs(dose_grid_0.yg-dose_grid_1.yg)<=db_small_distance
                             ) != dose_grid_0.yg.size()
       || vector_util_count_true(
           vector_util_abs(dose_grid_0.zg-dose_grid_1.zg)<=db_small_distance
                             ) != dose_grid_0.zg.size()) {
     logfile_error("dose_grid_compare.cpp : dose_grid_compare : dose difference calculation requires input dose cubes to be on the same grid (gamma calc, however, supports different grids).\n");
     return;
   }

  double Dmax = vector_util_max(dose_grid_1.D);
  double maximum_dose_diff = 100.0*vector_util_max(vector_util_abs(dose_grid_0.D - dose_grid_1.D))/Dmax;
  logfile_printf("Largest dose difference [%%D1max] = %.1f\n", maximum_dose_diff);
  if (maximum_dose_diff > db_tolerated_dose_diff) 
    logfile_error("Dose difference exceeds the database specified value of %.1f %%D1max\n", db_tolerated_dose_diff);
}
