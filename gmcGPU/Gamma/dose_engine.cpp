#include "dose_engine.h"
#include "bp_tps.h"

DOSE_GRID  dose_engine(BP_TPS &bp_tps, TPSdata tps, std::vector<double> xg,  std::vector<double> yg, std::vector<double> zg) {
  DOSE_GRID  dose_grid;
  tpsdata_convert_q_gp(tps); 
  tpsdata_convert_dist_mm(tps);
  tpsdata_convert_coord_iec(tps);
  tpsdata_remove_offset(tps);
  if (logfile_success() == false) {
    logfile_error("dose_engine.cpp : dose_engine : failed to convert tramp file units\n");
    return dose_grid;
  }
  //TPS X,Y arrays are now in IEC coordinate system and units of [mm]

  double tps_layer_energy = 0;
  double del = db_grid_pitch;  //mm  grid pitch
  
  //try to create and initialize the dose grid
  try {
    dose_grid.xg=xg;
    dose_grid.yg=yg;
    dose_grid.zg=zg;
    dose_grid.D.resize(dose_grid.xg.size()*dose_grid.yg.size()*dose_grid.zg.size(),0);
  }
  catch (...) {
    logfile_error("dose_engine.cpp : dose_engine : Cannot allocate grid and dose vectors\n");
  }
  if (dose_grid.D.size()==0 )
    logfile_error("dose_engine.cpp : dose_engine : no grid points\n");
  if (dose_grid.D.size() !=  dose_grid.xg.size()*dose_grid.yg.size()*dose_grid.zg.size())
    logfile_error("dose_engine.cpp : dose_engine : dose grid has the wrong size\n");


  std::vector<double> SADx_scale = (db_SADy-dose_grid.zg/1000)/db_SADy; // NB SAD is in IBA coord and units of [m]
  std::vector<double> SADy_scale = (db_SADx-dose_grid.zg/1000)/db_SADx; 
  std::vector<double> grid_point_depth = db_iso_depth-dose_grid.zg;//[mm]

  double SADsq_x = db_SADy*db_SADy*1000*1000; //IEC coord and mm^2
  double SADsq_y = db_SADx*db_SADx*1000*1000; //IEC coord and mm^2

//   print (tps.q);
//   loop through spots
  for (unsigned long int ss=0; ss<tps.E.size()&& logfile_success()==true; ++ss) { // ss= spot number in tpsdata
    // Astroid dist units are [mm]
    std::vector<double> divergent_depth = grid_point_depth;
    for (unsigned long int kk=0; kk<divergent_depth.size();++kk) //calculates depth of grid point along the ray of the pencil beam
      divergent_depth[kk]=divergent_depth[kk]*sqrt(1+ tps.x.at(ss)*tps.x.at(ss)/SADsq_x  
						   +  tps.y.at(ss)*tps.y.at(ss)/SADsq_y);
    std::vector<double> tps_depth_dose= bp_tps_vdose(bp_tps,tps.E.at(ss),divergent_depth);
    //print(tps_depth_dose);
     for (unsigned long int kk=0; kk<dose_grid.zg.size() && logfile_success()==true;++kk) { // loop over z in the dose grid
       if (tps_depth_dose.at(kk)>0.001) {
	tps_layer_energy=tps.E.at(ss);
	//Fill tps_depth_dose vector for this new energy
	unsigned long int bp_index = find_closest(bp_tps.tbl.E,tps_layer_energy);
	//       if (ss==0) {
	// 	print(bp_tps.steps.r90);
	// 	print(bp_tps.steps.r80);
	// 	print(bp_tps.steps.E);
	//       }
	double r80 = linear_interp(bp_tps.steps.E, bp_tps.steps.r80, tps_layer_energy); //r80 for this energy
	if (r80==0) {
	  logfile_error("dose_engine.cpp : dose_engine : cannot extract r80\n");
	}
	double r90 = linear_interp(bp_tps.steps.E, bp_tps.steps.r90, tps_layer_energy); //r90 for this energy
	if (r90==0) {
	  logfile_error("dose_engine.cpp : dose_engine : cannot extract r90\n");
	}
	double grid_layer_Z=dose_grid.zg[kk];
	double tps_sigma_x2 = ((  bp_tps.optics.Ax.at(bp_index) +   //sigma sqared in IEC coord, [mm2]
			   bp_tps.optics.Ay.at(bp_index)*grid_layer_Z + 
			   bp_tps.optics.Az.at(bp_index)*grid_layer_Z*grid_layer_Z)/2.0);
	double tps_sigma_y2 = ((  bp_tps.optics.Bx.at(bp_index) + //IEC coords 
			   bp_tps.optics.By.at(bp_index)*grid_layer_Z + 
			   bp_tps.optics.Bz.at(bp_index)*grid_layer_Z*grid_layer_Z)/2.0);
	
	//calculate MCS spreading in water and add in quadrature with optical sigma
	double sigma_MCS = bp_tps_mcs(divergent_depth[kk], r90);
	//if(ss==0)
	//  cout<<sigma_MCS<<' '<<divergent_depth[kk]<<' '<<r90<<endl;
	//cout<<"Sig1 "<<tps_layer_energy<<' '<<bp_index<<' '<<sqrt(tps_sigma_x2)<<' '<<sqrt(tps_sigma_y2)<<endl;
	double sx= sqrt(tps_sigma_x2+sigma_MCS*sigma_MCS);
	double sy= sqrt(tps_sigma_y2+sigma_MCS*sigma_MCS);
	//cout<<"Sig_total "<<r80<<' '<<r90<<' '<<divergent_depth[kk]<<' '<<sqrt(tps_sigma_x2+sigma_MCS*sigma_MCS)<<' '<<sqrt(tps_sigma_y2+sigma_MCS*sigma_MCS)<<endl;
	if (sx <= 0 || sy<=0 ){
	  logfile_error("dose_engine.cpp : dose_engine : sigma is less than or equal to zero\n");
	  logfile_printf("E = %f z = %f sx = %f sy = %f",  tps.E.at(ss), dose_grid.zg.at(kk), sx, sy);
	}	
	// 	//Dose calculation now
	if (logfile_success()==true) {
	  double tmp_D = tps.q.at(ss)*tps_depth_dose.at(kk)/(del*del);
	  std::vector<double> Gx = gauss_grid(dose_grid.xg, 1, sx, tps.x.at(ss)*SADx_scale.at(kk) );
	  std::vector<double> Gy = gauss_grid(dose_grid.yg, 1, sy, tps.y.at(ss)*SADy_scale.at(kk) );
	  HALO_PARAM hp = dose_engine_halo_param(divergent_depth[kk],r80);
	  std::vector<double> Gx_halo = gauss_grid(dose_grid.xg, 1, hp.sigma_h, tps.x.at(ss)*SADx_scale.at(kk) );
	  std::vector<double> Gy_halo = gauss_grid(dose_grid.yg, 1, hp.sigma_h, tps.y.at(ss)*SADy_scale.at(kk) );
	  if (Gx.size()==0 || Gy.size() ==0 || Gx_halo.size()==0 || Gy_halo.size() ==0) {
	    logfile_error("dose_engine.cpp : dose_engine : gauss_grid returned vectors with size %u,%u\n",
			  Gx.size(), Gy.size());
	  }
	  unsigned long int Gx_size = Gx.size();
	  unsigned long int Gy_size = Gy.size();
	  for (unsigned long int jj=0; jj<Gy_size;++jj) {
	    if (Gy[jj]>0){
	      unsigned long int ind =kk*dose_grid.yg.size()*dose_grid.xg.size()+jj*dose_grid.xg.size();
	      for (unsigned long int ii=0; ii<Gx_size;++ii) {
		if (Gx[ii]>0) {
		  dose_grid.D[ind+ii] += tmp_D*( hp.alpha*Gx[ii]*Gy[jj] + (1-hp.alpha)*Gx_halo[ii]*Gy_halo[jj]);
		}
	      }// end looping over xg
	    } 
	  } //end looping over yg
	} //end of success == true
       } //end of if the tps depth dose is greater than zero
       else if (tps_depth_dose.at(kk)<0) {
	  logfile_error("dose_engine.cpp : dose_engine : depth dose less than zero\n");
	  vector_util_print(tps_depth_dose);
       }
     } //end of dose calculation for a spot and end of looping over z in the grid
  }  //end looping over spots
  
  if (logfile_success() == false)
    dose_grid.D.clear();

  return dose_grid;
 
}

DOSE_GRID  dose_engine(BP_TPS &bp_tps, TPSdata tps) { 

  DOSE_GRID  dose_grid;
  tpsdata_convert_q_gp(tps); 
  tpsdata_convert_dist_mm(tps);
  tpsdata_convert_coord_iec(tps);
  tpsdata_remove_offset(tps);
  if (logfile_success() == false) {
    logfile_error("dose_engine.cpp : dose_engine : failed to convert tramp file units\n");
    return dose_grid;
  }
  //TPS X,Y arrays are now in IEC coordinate system and units of [mm]

  double tps_layer_energy = 0;
  double del = db_grid_pitch;  //mm  grid pitch

  std::vector<double> r90_vec;  //r90_vec in [mm] and sigma_square [mm2] for each spot
  for (unsigned long int ss = 0;  ss< tps.E.size(); ++ss) { //loop over spots and find tables
    if (fabs(tps.E.at(ss)-tps_layer_energy)>db_small_energy) { //is this a new layer in tps?
      tps_layer_energy=tps.E.at(ss);
      unsigned long int ir = find_closest(bp_tps.steps.E,tps.E.at(ss));
      r90_vec.push_back( bp_tps.steps.r90.at(ir) );
    }
  }

  //create and initialize the dose grid
  for ( double x = vector_util_min(tps.x)-db_pad_grid; x <=vector_util_max(tps.x)+db_pad_grid; x += del) 
    dose_grid.xg.push_back(x);
  for ( double y = vector_util_min(tps.y)-db_pad_grid; y <=vector_util_max(tps.y)+db_pad_grid; y += del)
    dose_grid.yg.push_back(y);
  for ( double z = -10*ceil(vector_util_max(r90_vec)/10)-3*del+db_iso_depth; z <=db_iso_depth; z += del)
    dose_grid.zg.push_back(z);  
  dose_grid =  dose_engine(bp_tps, tps, dose_grid.xg, dose_grid.yg, dose_grid.zg);

  //don't take this write feature out of this function or there could be a chance you connect the wrong dose_grid with the tps structure
  if (logfile_success() == true)
    dose_engine_write_mdose(tps, dose_grid);  
  else
    dose_grid.D.clear();

  return dose_grid;
}




 //write mdose file
void dose_engine_write_mdose(TPSdata tps, DOSE_GRID dose_grid){ 

  //double ga = tps.header.gantry;  //deg

  //get the mdose file ready
  ofstream output_file;
  string null_string("");
  string tmp1(".tramp");
  string tmp2(".csv");
  long int i = tps.header.trampfilename.find(tmp1);
  if (i>=0)
    tps.header.trampfilename.replace(i, tmp1.size(), null_string);
  i = tps.header.trampfilename.find(tmp2);
  if (i>=0)
    tps.header.trampfilename.replace(i, tmp2.size(), null_string);
  string filename_with_mdose(tps.header.trampfilename + ".mdose");
  replace(filename_with_mdose.begin(),filename_with_mdose.end(),'\r','_');
  output_file.open(filename_with_mdose.c_str(),ios::out|ios::trunc); 
  if (!output_file) {
    logfile_error("Failed to open %s\n", filename_with_mdose.c_str());
    return ; }
  output_file.exceptions(ios::failbit | ios::badbit | ios::eofbit);

  //write some header info
  //count the number of layers
  std::vector<double> Eunique = tps.E;
  vector<double>::iterator new_end=
    unique(Eunique.begin(), Eunique.end());
  unsigned long int nlayers = distance( Eunique.begin(), new_end);
  output_file<<"Beam, "<<tps.header.patient_id<<", "<<tps.header.patient_name_first<<", "
	     <<tps.header.patient_name_middle<<", "<<tps.header.patient_name_last<<", "
	     <<tps.header.course_name<<", "<<tps.header.beam_name<<", "<<tpsdata_sumq(tps)<<", "
	     <<tpsdata_sumq(tps)<<", "<<nlayers<<endl;
  output_file<<"Iso depth (mm)\n"<<db_iso_depth<<endl;
  output_file<<"Grid Pitch (mm)\n"<<db_grid_pitch<<endl;
  output_file<<"Halo on(1)/Off(0)\n1"<<endl; //Halo calc is done by default

  //Output the grid points
  for (unsigned long int ii=0; ii<dose_grid.xg.size(); ++ii) {
    if (ii == 0) {
      output_file<<"X grid (mm)\n" //IEC X [mm]
		 <<dose_grid.xg.at(ii);
    }
    else
      output_file<<','<<dose_grid.xg.at(ii);
  }
  output_file<<endl;
  for (unsigned long int ii=0; ii<dose_grid.yg.size(); ++ii) {
    if (ii == 0) {
      output_file<<"Y grid (mm)\n" //IEC Y [mm]
		 <<dose_grid.yg.at(ii);
    }
    else
      output_file<<','<<dose_grid.yg.at(ii);
  }
  output_file<<endl;
  for (unsigned long int ii=0; ii<dose_grid.zg.size(); ++ii) {
    if (ii == 0) {
      output_file<<"Z grid (mm)\n" //IEC Z [mm]
		 <<dose_grid.zg.at(ii);
    }
    else
      output_file<<','<<dose_grid.zg.at(ii);
  }
  output_file<<endl;
  for (unsigned long int ii=0; ii<dose_grid.xg.size();++ii) {
    for (unsigned long int kk=0; kk<dose_grid.zg.size();++kk) {
      for (unsigned long int jj=0; jj<dose_grid.yg.size();++jj) {
	unsigned long int ind = kk*dose_grid.yg.size()*dose_grid.xg.size()+jj*dose_grid.xg.size()+ii;
	if (jj ==0 && kk==0) 
	  output_file<<dose_grid.D.at(ind);
	else
	  output_file<<','<<dose_grid.D.at(ind);
      }}
    output_file<<'\n';
  }
  output_file.close();
  return;
}

//Depth and R_pb are in [mm]
//alpha is the strength of the primary gaussian (1-f_h), where f_h is from Pedroni et al
HALO_PARAM dose_engine_halo_param(double depth, double R_pb)
{
  HALO_PARAM hp;
  float r_cm = R_pb/10.0;  //R80
  float t = depth/R_pb;
  hp.alpha = fmin(1.0, 1.002    -5.9e-4*r_cm  
		  +(2.128e-03 -2.044e-02*r_cm  + 3.178e-04*r_cm*r_cm)*t
		  +(-2.549e-03 +2.125e-02*r_cm  - 3.788e-04*r_cm*r_cm)*t*t );
  hp.sigma_h = 10.0*(6.5 - 0.34*r_cm + 0.0078*r_cm*r_cm);
  return hp;
}
