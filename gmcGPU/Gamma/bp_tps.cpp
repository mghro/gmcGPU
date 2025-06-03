#include "bp_tps.h"

BP_TPS  bp_tps_read(const char steps_file_name[],const char optics_file_name[],
	       const char tbl_file_name[], const char pp_dir_name[]) {

  BP_TPS bp_tps;
  std::ifstream infile;
  float r1, r2, r3, r4,r5, r6, r7, r8;  
  char buff[5000];

  //read in the energy steps file
  infile.open( steps_file_name );   
  if (!infile) 
    logfile_error("bp_tps.cpp : bp_tps_read : Failed to open %s\n", steps_file_name);
  if (logfile_success()==true) {
    while (infile.getline(buff,5000,'\n')) {
      int nread=sscanf(buff,"%f %f %f %f",
		       &r1,&r2,&r3,&r4); //read numbers from buff    
      if (nread == 3 ) {
	bp_tps.steps.r90.push_back( double(r1));
	bp_tps.steps.r80.push_back( double(r2));
	bp_tps.steps.E.push_back( double(r3));
      }
      else {
	logfile_error("bp_tps.cpp : bp_tps_read : the following line of the pbs.steps file was not recognized\n");
	logfile_printf("%s\n", buff);
      }
    }
  }
  infile.close();
  infile.clear();
  
  //read in the optics file
  infile.open( optics_file_name );   
  if (!infile) 
    logfile_error("bp_tps.cpp : bp_tps_read : Failed to open %s\n", optics_file_name);
  if (logfile_success()==true) {
    while (infile.getline(buff,5000,'\n')) {
      int nread=sscanf(buff,"%f %f %f %f %f %f %f %f",
		       &r1,&r2,&r3,&r4,&r5,&r6,&r7,&r8); //read numbers from buff    
      if (nread == 7 ) {
	bp_tps.optics.Ax.push_back( double(r1));
	bp_tps.optics.Ay.push_back( double(r2));
	bp_tps.optics.Az.push_back( double(r3));
	bp_tps.optics.Bx.push_back( double(r4));
	bp_tps.optics.By.push_back( double(r5));
	bp_tps.optics.Bz.push_back( double(r6));
	bp_tps.optics.wfit.push_back( double(r7));
      }
      else {
	logfile_error("bp_tps.cpp : bp_tps_read : the following line of the pbs.optics file was not recognized\n");
	logfile_printf("%s\n", buff);
      }
    }
  }
  infile.close();
  infile.clear();
  
  //read in the tbl file
  infile.open( tbl_file_name );   
  if (!infile) 
    logfile_error("bp_tps.cpp : bp_tps_read : Failed to open %s\n", tbl_file_name);
  if (logfile_success()==true) {
    while (infile.getline(buff,5000,'\n')) {
      int nread=sscanf(buff,"%f %f %f %f %f",
		       &r1,&r2,&r3,&r4,&r5); //read numbers from buff    
      if (nread == 4 ) {
	bp_tps.tbl.r100.push_back( double(r1));
	bp_tps.tbl.r90.push_back( double(r2));
	bp_tps.tbl.w80.push_back( double(r3));
	bp_tps.tbl.E.push_back( double(r4));
      }
      else {
	logfile_error("bp_tps.cpp : bp_tps_read : the following line of the pbs.tbl file was not recognized\n");
	logfile_printf("%s\n", buff);
      }
    }
  }
  infile.close();
  infile.clear();

  //read in the pp files
  unsigned long int pp_numb = 0;
  unsigned long int pp_len = 0;
  for (unsigned long int ii=0; ii<bp_tps.tbl.r90.size(); ++ii) {
    int r90_int = int(floor(bp_tps.tbl.r90.at(ii)));
    std::stringstream r90_str;
    r90_str<<pp_dir_name;
    r90_str<<"pp";
    if (r90_int<10)
      r90_str<<"0";
    if (r90_int<100)
      r90_str<<"0";
    r90_str<<((r90_int));
    r90_str<<".pbs";
    infile.open( r90_str.str().c_str() );   
    if (!infile) 
      logfile_error("bp_tps.cpp : bp_tps_read : Failed to open %s\n", r90_str.str().c_str());
    unsigned long int line_numb = 0;
    if (logfile_success()==true) {
      while (infile.getline(buff,5000,'\n')) {
	line_numb += 1;
	int nread=sscanf(buff,"%f %f %f",
			 &r1,&r2,&r3); //read numbers from buff    
	if (line_numb==1 && nread == 1) {
	  bp_tps.pp.pp_numb.push_back(pp_numb);
	} 
	else if (line_numb==2 && nread == 1 ) {
	  bp_tps.pp.r90.push_back( double(r1));
	}
	else if (line_numb==3 && nread == 1){
	  pp_len = (unsigned long int) r1;
	  bp_tps.pp.pp_len.push_back( pp_len );
	}
	else if (line_numb>3 && nread == 2 ) {
	  bp_tps.pp.depth.push_back( double(r1));
	  bp_tps.pp.dose.push_back( double(r2));
	  pp_numb+=1;
	}
	else {
	  logfile_error("bp_tps.cpp : bp_tps_read : the following line of %s was not recognized\n",r90_str.str().c_str());
	  logfile_printf("%s\n", buff);
	}
      }
      if ((line_numb-3) != pp_len ) {
	logfile_error("bp_tps.cpp : bp_tps_read : the length of the file %s=%i does not match the third row of the file=%i\n",
		      r90_str.str().c_str(), line_numb-3, pp_len);
	logfile_printf("%s\n", buff);
      }
	
    }
    infile.close();
    infile.clear();
  }
  

  return bp_tps;
}



void bp_tps_print(BP_TPS &bp_tps) {
  for (unsigned long int ii=0; ii<bp_tps.steps.r90.size() ; ++ii)
    logfile_printf("%f,%f,%f\n",bp_tps.steps.r90.at(ii),bp_tps.steps.r80.at(ii),bp_tps.steps.E.at(ii));
  for (unsigned long int ii=0; ii<bp_tps.optics.Ax.size() ; ++ii)
    logfile_printf("%f,%f,%f,%f,%f,%f,%f\n",
		   bp_tps.optics.Ax.at(ii),
		   bp_tps.optics.Ay.at(ii),
		   bp_tps.optics.Az.at(ii),
		   bp_tps.optics.Bx.at(ii),
		   bp_tps.optics.By.at(ii),
		   bp_tps.optics.Bz.at(ii),
		   bp_tps.optics.wfit.at(ii));
  for (unsigned long int ii=0; ii<bp_tps.tbl.r100.size() ; ++ii)
    logfile_printf("%f,%f,%f,%f\n", 
		   bp_tps.tbl.r100.at(ii),bp_tps.tbl.r90.at(ii),
		   bp_tps.tbl.w80.at(ii),bp_tps.tbl.E.at(ii));
  for (unsigned long int ii=0; ii<bp_tps.pp.pp_numb.size() ; ++ii) {
    logfile_printf("Pristine peak R90= %f\n", bp_tps.pp.r90.at(ii));
    for (unsigned long int jj=bp_tps.pp.pp_numb.at(ii); 
	 jj < bp_tps.pp.pp_numb.at(ii)+bp_tps.pp.pp_len.at(ii) ; 
	 ++jj) 
      logfile_printf("%f,%f\n", bp_tps.pp.depth.at(jj),bp_tps.pp.dose.at(jj));
    
  } 
}


//extract the dose from the BP_TPS depth-dose distribution at given depth
//Using B.Clasie et al. interpolation of Bragg Peak method
//Return with dose averaged over the depth step size where depth vector specifies the center of voxels
std::vector<double> bp_tps_vdose(BP_TPS &bp_tps,double layer_energy, std::vector<double> depth)
{
  std::vector<double> dose; 
  
  if (bp_tps.tbl.E.size()<2) {
    logfile_error("bp_tps.cpp : bp_tps_vdose : not enough peaks in the database for bp_tps dose interplation\n");
    return dose;
  }
  
  //   find two peaks that are either equal to or neighboring the requested peak
  unsigned long int i1=0;
  unsigned long int i2 = 1;
  for (unsigned long int ii=0; ii<bp_tps.tbl.E.size()-1; ++ii) {
    if ( fabs(bp_tps.tbl.E.at(ii)-layer_energy)+fabs(bp_tps.tbl.E.at(ii+1)-layer_energy)
	 < fabs(bp_tps.tbl.E.at(i1)-layer_energy)+fabs(bp_tps.tbl.E.at(i2)-layer_energy) ) {
      i1 = ii;
      i2 = ii+1;
    }
  }
  //cout<<layer_energy<<' '<<bp_tps.tbl.E.at(i1)<<' '<<bp_tps.tbl.E.at(i2)<<endl;
  //Extract the i1'th and i2'th depth dose vectors from the database
  std::vector<double> x1(&bp_tps.pp.depth[bp_tps.pp.pp_numb.at(i1)],&bp_tps.pp.depth[bp_tps.pp.pp_numb.at(i1)+bp_tps.pp.pp_len.at(i1)]);
  std::vector<double> x2(&bp_tps.pp.depth[bp_tps.pp.pp_numb.at(i2)],&bp_tps.pp.depth[bp_tps.pp.pp_numb.at(i2)+bp_tps.pp.pp_len.at(i2)]);
  std::vector<double> y1(&bp_tps.pp.dose[bp_tps.pp.pp_numb.at(i1)],&bp_tps.pp.dose[bp_tps.pp.pp_numb.at(i1)+bp_tps.pp.pp_len.at(i1)]);
  std::vector<double> y2(&bp_tps.pp.dose[bp_tps.pp.pp_numb.at(i2)],&bp_tps.pp.dose[bp_tps.pp.pp_numb.at(i2)+bp_tps.pp.pp_len.at(i2)]);
  // x is depth in mm and y is dose in GyRBE mm2/Gp
  
  double r80_1 = bp_tps_r80(x1, y1);
  double r80_2 = bp_tps_r80(x2, y2);
  double r80 = exp( log(r80_1) + (log(layer_energy)-log(bp_tps.tbl.E.at(i1)))
		    *(log(r80_2)-log(r80_1))
		    /(log(bp_tps.tbl.E.at(i2))-log(bp_tps.tbl.E.at(i1))));
  
  std::vector<double> norm_x1 = x1/r80_1;
  std::vector<double> norm_x2 = x2/r80_2;
  
  for (unsigned long int dd = 0; dd<depth.size(); ++dd) {  //loop over depths
     double dose_at_dd = 0;
    //check if depth(dd) is past the depth in the bragg peak database and give a dose of zero in this case
    if (depth.at(dd)/r80>fmin(vector_util_max(norm_x1),vector_util_max(norm_x2))) 
      dose_at_dd = 0;
    else {
      //perform integration of the dose over the voxel (specified by step size in depth)
      std::vector<double> tmp_z;  //anything "tmp" is evaluated on sub-voxel step sizes
      if (depth.size()>1) {
	double del_z = fabs(depth.at(1)-depth.at(0));
	for (double dz =-0.5; dz<=0.501; dz=dz+0.2) {
	  double interp_depth = depth.at(dd)+del_z*dz;
	  if (interp_depth >0) tmp_z.push_back(interp_depth);
	}
      }
      else
	tmp_z.push_back(depth.at(dd));
      
      //tmp_z stores Z values that span one voxel (specified by the original "depth" vector) in depth
            
      std::vector<double> tmp_dose;
      if (fabs(bp_tps.tbl.E.at(i1)-layer_energy)<db_small_energy) {  
	//Normally, with pbs.tbl representing every available energy, this case or the next case is true
	tmp_dose = linear_interp(x1,y1,tmp_z);
      }
      else if (fabs(bp_tps.tbl.E.at(i2)-layer_energy)<db_small_energy) {
	tmp_dose = linear_interp(x2,y2,tmp_z);
      }
      else { //interpolate the Bragg Peak using the database
	std::vector<double> norm_depth = tmp_z/r80;
	std::vector<double> d1_interp = linear_interp(norm_x1,y1,norm_depth);
	std::vector<double> d2_interp = linear_interp(norm_x2,y2,norm_depth);
	if (d1_interp.size()>0 && d2_interp.size() > 0) {
	  double w1= (log(r80)-log(r80_1)) / (log(r80_2)-log(r80_1));
	  for (unsigned long int ii=0; ii<tmp_z.size(); ++ii) {
	    double dose_point = exp(log(d1_interp.at(ii)) 
				    + w1*(log(d2_interp.at(ii))-log(d1_interp.at(ii))) );     
	    if (d1_interp.at(ii)<=0 ) dose_point=d2_interp.at(ii);
	    if (d2_interp.at(ii)<=0 ) dose_point=d1_interp.at(ii);
	    if (d1_interp.at(ii)<=0 && d2_interp.at(ii)<=0 ) dose_point=0;
	    if (dose_point <0 ) dose_point = 0;
	    if (isnan(dose_point)) {
	      tmp_dose.push_back(0);
	      logfile_error("bp_tps : bp_tps_vdose : NaN during Bragg Peak interpolation\n");
	      dose.clear();
	      return dose;
	    }
	    else                   
	      tmp_dose.push_back(dose_point); 
	  }
	}
      }
      if (tmp_dose.size() != tmp_z.size() || tmp_z.size()==0) {
	logfile_error("bp_tps : bp_tps_vdose : vector sizes do not agree when integrating the depth dose distribution.  This could be due to interpolating dose at negative depth.\n");
	dose.clear();
	return dose;
      }
      
      //    cout<<favg(tmp_dose)<<endl;
      //    dose.push_back(favg(tmp_dose));
      
      if (tmp_z.size()==1) 
	dose_at_dd=tmp_dose.at(0);
      else { //average by trapezoidal integration
	unsigned long int N=1;
	double average =tmp_dose.at(0) + tmp_dose.at(tmp_dose.size()-1);
	for (unsigned long int ctr = 1; ctr<tmp_dose.size()-1; ++ctr) {
	  average += 2*tmp_dose.at(ctr);
	  N += 1;
	}
	//     cout<<average/2/double(N)<<endl;
	dose_at_dd = average/2/double(N);
      }
    } //end of checking if depth(dd) is past the depths in the bragg peak database
    dose.push_back(dose_at_dd);
  } //end looping over depths
  
  
  
  //   //interpolate database at this energy
  //   if (fabs(bp_tps.tbl.E.at(i1)-layer_energy)<db_small_energy) {
  //     dose = linear_interp(x1,y1,depth, report);
  //   }
  //   else if (fabs(bp_tps.tbl.E.at(i2)-layer_energy)<db_small_energy) {
  //     dose = linear_interp(x2,y2,depth, report);
  //   }
  //   else {
  //     double r80_1 = bp_tps_r80(x1, y1, report);
  //     double r80_2 = bp_tps_r80(x2, y2, report);
  //     double r80 = exp( log(r80_1) + (log(layer_energy)-log(bp_tps.tbl.E.at(i1)))
  // 		      *(log(r80_2)-log(r80_1))
  // 		      /(log(bp_tps.tbl.E.at(i2))-log(bp_tps.tbl.E.at(i1))));

  //     std::vector<double> norm_depth = depth/r80;
  //     std::vector<double> d1_interp = linear_interp(x1/r80_1,y1,norm_depth, report);
  //     std::vector<double> d2_interp = linear_interp(x2/r80_2,y2,norm_depth, report);
  //     if (d1_interp.size()>0 && d2_interp.size() > 0) {
  //       double w1= (log(r80)-log(r80_1)) / (log(r80_2)-log(r80_1));
  //       for (int ii=0; ii<int(depth.size()); ++ii) {
  // 	double dose_point = exp(log(d1_interp.at(ii)) 
  // 				+ w1*(log(d2_interp.at(ii))-log(d1_interp.at(ii))) );
  // 	if (isnan(dose_point)) 	dose.push_back(0);
  // 	else dose.push_back(dose_point); //no integration if depth is a scalar
  //       }
  //     }
  //   }
  
  //   cout<<'\n'<<r80<<endl;
  //   print(depth);
  //   print(dose);
  
  
  return dose;
}


double bp_tps_dose(BP_TPS &bp_tps,double layer_energy, double depth) {
  std::vector<double> vdepth(1,depth);
  std::vector<double> vdose = bp_tps_vdose(bp_tps,layer_energy, vdepth);
  if (vdose.size()==0) return -1;
  return (vdose.at(0));
}

//return with r80 of the bragg peak
double bp_tps_r80(std::vector<double> depth, std::vector<double> dose){
  if (depth.size() != dose.size()  || depth.size() <2 || dose.size() < 2) { 
      return (-1);}

//   for (int ii=0; ii<int(depth.size()); ++ii) 
//     cout<<depth.at(ii)<<' '<<dose.at(ii)<<endl;

  double dose_80 = 0.8*vector_util_max(dose);
  for (unsigned long int ii=depth.size()-2; ii>=0; ii = ii-1) {
    if (dose.at(ii)>dose_80)  
      return(depth.at(ii) + (depth.at(ii+1)-depth.at(ii)) / (dose.at(ii+1)-dose.at(ii)) * (dose_80-dose.at(ii)) );
  }
  return (-1);
}



//depth = depth in water [mm]
//r90 = range of protons [mm]
// returns with sigma in water in [mm]
double bp_tps_mcs(double depth, double r90) {
  double norm_depth = depth/r90;
  //cout<<"mcs check "<<depth<<' '<<r90<<' '<<norm_depth<<endl;
  if (norm_depth<0) return 0;
  double y0max = 0.022751 * r90 + 0.120852e-05*r90*r90; //Note that Hanne's function is all in [cm], the constants here convert all to [mm]
#ifdef LUNDER
  if (norm_depth >= 1) return(y0max); //Use this one later as an upgrade
#endif
#ifdef BURR_TR1_OR_TR2
  if (norm_depth >= 1) return 0;
#endif
  //  return (y0max*(0.4929*pow(norm_depth,4) - 0.8946*pow(norm_depth,3) + 1.2873*pow(norm_depth,2)  + 0.1122*norm_depth));
  //return(y0max*(0.0289352*pow(norm_depth,5) + 0.4261441*pow(norm_depth,4) - 0.8367356*pow(norm_depth,3) + 1.2673264*pow(norm_depth,2) + 0.1124771*norm_depth));
return(y0max*(1.2408148*pow(norm_depth,6) 
	      - 3.6071548*pow(norm_depth,5) 
	      + 4.4114394*pow(norm_depth,4) 
	      - 2.8387369*pow(norm_depth,3) 
	      + 1.7143005*pow(norm_depth,2) 
	      + 0.0787972*norm_depth));
}
