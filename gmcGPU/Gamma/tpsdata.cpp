

#include "tpsdata.h"

TPSdata tpsdata(const char * trampfilename) 
{

  TPSdata tps;
  tps.header.trampfilename= trampfilename;

  //Read in the spot definitions from file

  std::ifstream trampfile;
  trampfile.open( trampfilename );   
  if (!trampfile) {
    logfile_error("tpsdata : tpsdata : Failed to open %s\n", trampfilename);
    return tps; }

  tps.nspots = 0;

  float n1, n2, n3, n4, n5;  // E, X, Y, Spot IC charge
  double header_gigaprotons_total =0;
  double body_gigaprotons_total =0;
  double header_rows_total =0;
  double body_rows_total =0;

  string initials = "";
  tps.sort_spot_flag = true;
  tps.dose_comparison_override = COMP;
  tps.filter_type = REDIS;
  tps.slew_faster_factorx= 1.0; //IBA coordinates
  tps.slew_faster_factory= 1.0;
  tps.mult_dose_factor = 1.0; //multiply output charge by this factor
  tps.header.patient_id = "";
  tps.header.patient_name_first = "";
  tps.header.patient_name_middle = "";
  tps.header.patient_name_last = "";
  tps.header.course_name = "";
  tps.header.beam_name = "";
  tps.header.gantry = 10000;

  //Information that is identified in the header:
  //patient_id  (this affects the header in the output file)
  //patient_name  (this affects the header in the output file and the output file name)
  //course_name  (this affects the header in the output file)
  //beam_name (this affects the header in the output file)
  //slew_faster_factor (change the slew speed between spots by this factor)
  //sort_spot_flag  (set to 0=false to not perform any sorting of the order that spots are irradiated)
  //dose_comparison_override (insert initials in .tramp file, this sets the flag to NOCOMP to not perform the dose comparison)
  char buff[5000]; 
  while (trampfile.getline(buff,5000,'\n')) {
    for (int ii =0; ii<5000; ++ii) {  //convert csv to spaces
      if (buff[ii] == ',') buff[ii]=' ';
      if (buff[ii] == '\0') break;
    }
    if (buff[0] == '#') { //header
      char str[4][1000];
      str[0][0] = '\0';str[1][0] = '\0';str[2][0] = '\0';str[3][0] = '\0'; //initialize
      sscanf(buff,"%*s %s %s %s %s",
	     str[0],str[1],str[2],str[3]); //read numbers from buff    
      if (!strcmp(str[1],"") ) {
	logfile_error("tpsdata.cpp : tpsdata : The following header of TPS file was not recognized\n");
	logfile_printf("%s\n",buff);
	logfile_printf("%s\n",str[0]);
      }
      else if (!strcmp(str[0],"patient_id") ) 
	tps.header.patient_id= UTIL_EXTRACT_STRING(buff,"_id");
      else if (!strcmp(str[0],"patient_name") ) {
	tps.header.patient_name_last=str[1];
	tps.header.patient_name_middle=str[2];
	tps.header.patient_name_first=str[3]; 
	tps.header.patient_name_first.replace(tps.header.patient_name_first.begin(),tps.header.patient_name_first.end(),'\r','\0');
      }
      else if (!strcmp(str[0],"patient_first_name") ) {
	tps.header.patient_name_first=UTIL_EXTRACT_STRING(buff,"_name");	  
      }
      else if (!strcmp(str[0],"patient_middle_initial") ) {
	tps.header.patient_name_middle=UTIL_EXTRACT_STRING(buff,"_initial");	  
      }
      else if (!strcmp(str[0],"patient_last_name") ) {
	tps.header.patient_name_last=UTIL_EXTRACT_STRING(buff,"_name");	  
      }
      else if (!strcmp(str[0],"course_name") ) 
	tps.header.course_name=UTIL_EXTRACT_STRING(buff,"_name");
      else if (!strcmp(str[0],"beam_name") ) 
	tps.header.beam_name=UTIL_EXTRACT_STRING(buff,"_name");
      else if (!strcmp(str[0],"slew_faster_factor") ) {
	if (!isFloat(str[1]))
	  logfile_error("tpsdata.cpp : tpsdata : The following line of %s does not contain a float\n%s\n",
			tps.header.trampfilename.c_str(), buff);	  
	double slew_faster_factor = atof(str[1]);
	tps.slew_faster_factorx=  slew_faster_factor;
	tps.slew_faster_factory=  slew_faster_factor;   }
      else if (!strcmp(str[0],"slew_faster_factorx") ) {
	if (!isFloat(str[1]))
	  logfile_error("tpsdata.cpp : tpsdata : The following line of %s does not contain a float\n%s\n",
			tps.header.trampfilename.c_str(), buff);	  
	tps.slew_faster_factorx=  atof(str[1]);}
      else if (!strcmp(str[0],"slew_faster_factory") ) {
	if (!isFloat(str[1]))
	  logfile_error("tpsdata.cpp : tpsdata : The following line of %s does not contain a float\n%s\n",
			tps.header.trampfilename.c_str(), buff);	  
	tps.slew_faster_factory=  atof(str[1]); }
      else if (!strcmp(str[0],"sort_spot_flag") ) {
	if (!isInteger(str[1]))
	  logfile_error("tpsdata.cpp : tpsdata : The following line of %s does not contain an integer\n%s\n",
			tps.header.trampfilename.c_str(), buff);	  
	tps.sort_spot_flag = atoi(str[1]); }
      else if (!strcmp(str[0],"dose_comparison_override") ) {
	initials = UTIL_EXTRACT_STRING(buff,"_override");	  
	if (isFloat(str[1]))
	  logfile_error("tpsdata.cpp : tpsdata : The following line of %s is a number and doesn't contain initials\n%s\n",
			tps.header.trampfilename.c_str(), buff);
	else if (!strcmp(initials.c_str(), "") || initials.length()<2) 
	  logfile_error("tpsdata.cpp : tpsdata : The following line of %s does not contain initials\n%s\n",
			tps.header.trampfilename.c_str(), buff);
	else 
	  tps.dose_comparison_override = NOCOMP;  //Not usually done
      }
      else if (!strcmp(str[0],"filter_type") ) {
	if (!isInteger(str[1]))
	  logfile_error("tpsdata.cpp : tpsdata : The following line of %s does not contain an integer\n%s\n",
			tps.header.trampfilename.c_str(), buff);	  
	tps.filter_type = static_cast<TPSDATA_FILTER_TYPE>(atoi(str[1])); }
      else if (!strcmp(str[0],"mult_dose_factor") ) {
	if (!isFloat(str[1]))
	  logfile_error("tpsdata.cpp : tpsdata : The following line of %s does not contain a float\n%s\n",
			tps.header.trampfilename.c_str(), buff);	  
	tps.mult_dose_factor=atof(str[1]); }
      else if (!strcmp(str[0],"rows_total")  ) {
	if (!isFloat(str[1]))
	  logfile_error("tpsdata.cpp : tpsdata : The following line of %s does not contain a float\n%s\n",
			tps.header.trampfilename.c_str(), buff); 
	header_rows_total = atof(str[1]); }
      else if (!strcmp(str[0],"gigaproton_total")  ) { 
	if (!isFloat(str[1]))
	  logfile_error("tpsdata.cpp : tpsdata : The following line of %s does not contain a float\n%s\n",
			tps.header.trampfilename.c_str(), buff);	  
	logfile_printf("Gigaproton total from Astroid = %s\n",str[1]);
	header_gigaprotons_total = atof(str[1]);
      }
      else if (!strcmp(str[0],"E(MeV)") || !strcmp(str[0],"\"E(MeV)\"")  ) {/*do nothing*/}
      else if (!strcmp(str[0],"astroid_id")) {
	tps.header.astroid_id=UTIL_EXTRACT_STRING(buff,"astroid_id");	        
      }
      else if (!strcmp(str[0],"gantry")  ) {
	if (!isFloat(str[1]))
	  logfile_error("tpsdata.cpp : tpsdata : The following line of %s is empty or does not contain a float\n%s\n",
			tps.header.trampfilename.c_str(), buff);	  
	tps.header.gantry = atof(str[1]);
	if (tps.header.gantry > 180)  tps.header.gantry = tps.header.gantry-360; //deg, limit to +/- 180 deg
	if (tps.header.gantry <-180)  tps.header.gantry = tps.header.gantry+360; //deg, limit to +/- 180 deg
      }
      else if (!strcmp(str[0],"couch_rotation")  ) {/*do nothing*/}
      else {
	logfile_error("tpsdata.cpp : tpsdata : The following header of TPS file was not recognized\n");
	logfile_printf("%s\n",buff);
	logfile_printf("%s\n",str[0]);
      }
    } //end of header
    else {  //spot definition
      int nread=sscanf(buff,"%f %f %f %f %f",
		       &n1,&n2,&n3,&n4,&n5); //read numbers from buff  
      if (!isFloatArray(buff))
	  logfile_error("tpsdata.cpp : tpsdata : The following line of %s does not contain floats\n%s\n",
			tps.header.trampfilename.c_str(), buff);	  	
      if (nread == 4 ) { 
	body_rows_total += 1;  //used for static checks 
	if (n4 >0) {   //spots with zero dose are thrown away here
	  body_gigaprotons_total += double(n4);
	  //fill the spot definition arrays
	  tps.E.push_back(n1);  //(MeV)  Beam energy at the entrance to the patient 
	  tps.x.push_back(n2);  
	  tps.y.push_back(n3); 
	  tps.q.push_back(n4);  
	  tps.nspots += 1;
	}
	if (n4 <0) {   //Negative Dose!!!
	  logfile_error("tpsdata.cpp : tpsdata : The following line of TPS file has negative Gigaprotons\n");
	  logfile_printf("%s\n",buff);
	}
      }
      else {
	logfile_error("tpsdata.cpp : tpsdata : The following line of TPS file was not recognized\n");
	logfile_printf("%s\n",buff);
      }
    }
  }
  trampfile.close();

  //The astroid factor (nominally 1.0, numbers larger than 1 reduce the measured dose)
  tps.mult_dose_factor=tps.mult_dose_factor/db_astroid_factor;

  if ( fabs(tps.slew_faster_factorx-1.0) > 0.0001) 
    logfile_warn("tpsdata.cpp : tpsdata : Slewing at X speeds %f times normal due to option set in %s\n",
		 tps.slew_faster_factorx, tps.header.trampfilename.c_str());
  if ( fabs(tps.slew_faster_factory-1.0) > 0.0001) 
    logfile_warn("tpsdata.cpp : tpsdata : Slewing at Y speeds %f times normal due to option set in %s\n",
		 tps.slew_faster_factory, tps.header.trampfilename.c_str());

  if (!tps.sort_spot_flag) 
    logfile_warn("tpsdata.cpp : tpsdata : Not sorting spots due to flag in %s\n",
		 tps.header.trampfilename.c_str());
  if (tps.dose_comparison_override == NOCOMP)  //not usually done 
    logfile_warn("tpsdata.cpp : tpsdata : User \"%s\" turned off the dose comparison with a flag in %s\n",
		 initials.c_str(), tps.header.trampfilename.c_str());
  if (tps.filter_type != REDIS ) 
    logfile_warn("tpsdata.cpp : tpsdata : Alternate spot filtering method %i used due to flag in %s\n",
		 tps.filter_type, tps.header.trampfilename.c_str());
  if (tps.filter_type != ROUND && tps.filter_type != REDIS && tps.filter_type !=CUT && tps.filter_type != NONE) 
    logfile_error("tpsdata.cpp : tpsdata : Filtering method %i in %s does not exist\n",
		 tps.filter_type, tps.header.trampfilename.c_str());

  if ( fabs(tps.mult_dose_factor-1.0) > 0.0001) 
    logfile_warn("tpsdata.cpp : tpsdata : Dose multiplication factor is %f due to db_astroid_factor in the DB file and/or the option set in %s\n",
		 tps.mult_dose_factor, tps.header.trampfilename.c_str());

  //input static checks
  if (tps.nspots <= 0) 
    logfile_error("tpsdata.cpp : tpsdata : There are no spots in the tramp file\n");
  if ( fabs(header_gigaprotons_total-body_gigaprotons_total) > tps.nspots*0.00005) 
    logfile_error("tpsdata.cpp : tpsdata : gigaproton_total in the tramp file header (%f) does not match the body (%f)\n",
		  header_gigaprotons_total, body_gigaprotons_total);
  if ( fabs(header_rows_total-body_rows_total) > 0.05) 
    logfile_error("tpsdata.cpp : tpsdata : row_total in the tramp file header (%.1f) does not match the body (%.1f)\n",
		  header_rows_total, body_rows_total);
  if (!strcmp(tps.header.patient_id.c_str(), "")) 
    logfile_error("tpsdata.cpp : tpsdata : No patient ID in the tramp file\n");
  if (!strcmp(tps.header.patient_name_first.c_str() , "")) 
    logfile_error("tpsdata.cpp : tpsdata : No patient first name in the tramp file\n");
  if (!strcmp(tps.header.patient_name_middle.c_str() , "")) 
    logfile_error("tpsdata.cpp : tpsdata : No patient middle initial in the tramp file\n");
  if (!strcmp(tps.header.patient_name_last.c_str() , "")) 
    logfile_error("tpsdata.cpp : tpsdata : No patient last name in the tramp file\n");
  if (!strcmp(tps.header.astroid_id.c_str() , "")) 
    logfile_error("tpsdata.cpp : tpsdata : No astroid_id in the tramp file\n");
  if (fabs(tps.header.gantry)>181) 
    logfile_error("tpsdata.cpp : tpsdata : No gantry angle in the tramp file\n");
  if (!strcmp(tps.header.course_name.c_str() , "")) 
   logfile_error("tpsdata.cpp : tpsdata : No course name in the tramp file\n");
  if (!strcmp(tps.header.beam_name.c_str() , "")) 
   logfile_error("tpsdata.cpp : tpsdata : No beam name in the tramp file\n");

  tps.q_unit = GIGAPROTONS; //from Astroid
  tps.dist_unit = MM;       //from Astroid
  tps.coord_sys = IEC;      //from Astroid
  tps.offset_applied = NOTAPPLIED;  //from Astroid
  return tps;

}


//=====================================================================================
//print out the tps data without converting to tramp file conventions
void tpsdata_print(TPSdata &tps)
{
  logfile_printf("Success(1=true): %d\n", logfile_success());
  logfile_printf("Filename: %s\n",tps.header.trampfilename.c_str());
  logfile_printf("Patient ID: %s\n",tps.header.patient_id.c_str());
  logfile_printf("Plan label: %s\n",tps.header.course_name.c_str());
  logfile_printf("Beam name: %s\n",tps.header.beam_name.c_str());
  logfile_printf("Name: %s %s %s\n", tps.header.patient_name_first.c_str(),
		 tps.header.patient_name_middle.c_str(),tps.header.patient_name_last.c_str());
  logfile_printf("rows_total: %i\n",tps.E.size());
  if (tps.q_unit == PROTON_CHARGE || tps.q_unit == IC_CHARGE) 
    logfile_printf("nC_total: %f\n",1e9*tpsdata_sumq(tps));
  else if (tps.q_unit == MU)
    logfile_printf("MU_total: %f\n",tpsdata_sumq(tps));
  else
    logfile_printf("gigaproton_total: %f\n",tpsdata_sumq(tps));
  
  logfile_printf("# E(MeV)");
  string coord_sys;
  if (tps.coord_sys == IBA)  
    coord_sys = "IBA";
  else 
    coord_sys = "IEC";
  if (tps.dist_unit == M)
    logfile_printf(" %s_X(m) %s_Y(m)", coord_sys.c_str(), coord_sys.c_str());
  else if (tps.dist_unit == CM)
    logfile_printf(" %s_X(cm) %s_Y(cm)", coord_sys.c_str(), coord_sys.c_str());
  else
    logfile_printf(" %s_X(mm) %s_Y(mm)", coord_sys.c_str(), coord_sys.c_str());
    
  double factor = 1.0;
  if (tps.q_unit == PROTON_CHARGE || tps.q_unit == IC_CHARGE) {
    logfile_printf(" N(nC)\n");
    factor = 1e9;  // PROTON_CHARGE AND IC_CHARGE is in [C], convert to [nC] for the human
  }
  else if (tps.q_unit == MU)
    logfile_printf(" N(MU)\n");
  else
    logfile_printf(" N(Gp)\n");
  for (unsigned long int ii=0; ii<tps.E.size(); ++ii)
    logfile_printf("%f %f %f %f\n", tps.E.at(ii),tps.x.at(ii),tps.y.at(ii),factor*tps.q.at(ii));
}

//=====================================================================================
//write the tps data into a tramp_modified file with normal tramp file conventions
void tpsdata_write(TPSdata tps)
{
  tpsdata_convert_dist_mm(tps);
  tpsdata_convert_q_gp(tps);
  tpsdata_convert_coord_iec(tps);
  tpsdata_remove_offset(tps);
  if (logfile_success() == false) {
    logfile_error("tpsdata.cpp : tpsdata_write : failed to convert to tramp file conventions when writing the tramp_modified file\n");
    return;
  }

  //open tramp_modified file
  ofstream tramp_file;
  string tramp_fname = tps.header.trampfilename+"_modified";
  tramp_file.open(tramp_fname.c_str(),ios::out|ios::trunc); 
  if (!tramp_file ) {
    logfile_error("tpsdata.cpp : tpsdata_write :  Failed to open %s\n", tramp_fname.c_str());
    return;
  }

tramp_file
  <<"# patient_id "             <<  tps.header.patient_id<<endl
  <<"# patient_first_name "     << tps.header.patient_name_first<<endl
  <<"# patient_middle_initial " <<tps.header.patient_name_middle<<endl
  <<"# patient_last_name "      <<tps.header.patient_name_last<<endl
  <<"# astroid_id "      <<tps.header.astroid_id<<endl
  <<"# course_name "	      <<tps.header.course_name<<endl
  <<"# beam_name "	      <<tps.header.beam_name<<endl
  <<"# gantry "		      <<tps.header.gantry<<endl
  <<"# couch_rotation "	      <<"N/A"<<endl
  <<"# gigaproton_total  "      <<tpsdata_sumq(tps)<<endl
  <<"# rows_total "             <<int(tps.E.size())<<endl;

  tramp_file<<"# E(MeV)    X(mm)    Y(mm)    N(Gp)"<<endl;
  for (unsigned long int ii=0; ii<tps.E.size(); ++ii)
    tramp_file<<tps.E.at(ii)<< ' '<<tps.x.at(ii)<< ' '<<tps.y.at(ii)<< ' '
	<<tps.q.at(ii)<<endl;
  tramp_file.close();
  return;
}



//=====================================================================================
//sum the charge in the tps structure
double tpsdata_sumq(TPSdata &tps)
{
  double sum = 0;
  for (unsigned long int ii=0; ii<tps.q.size(); ++ii)
    sum = sum+tps.q.at(ii);
  return sum;
}


//=====================================================================================
////delete a spot
void tpsdata_delete(TPSdata &tps, unsigned long int a){
  if (a<0 || a>= tps.nspots ) {
    logfile_error("tpsdata : tpsdata_delete : invalid index %u %u\n", a, tps.nspots);
    return;
  }
  tps.E.erase( tps.E.begin()+a);
  tps.x.erase( tps.x.begin()+a);
  tps.y.erase( tps.y.begin()+a);
  tps.q.erase( tps.q.begin()+a);
  tps.nspots = tps.E.size();
}

//=====================================================================================
////are spots at the same energy and  position
bool tpsdata_equal(TPSdata &tps, unsigned long int a, unsigned long int b){
  if (a<0 || a>= tps.nspots || b<0 || b>= tps.nspots) {
    logfile_error("tpsdata : tpsdata_equal : invalid index %u %u %u\n", a, b, tps.nspots);
    return false;
  }
  if ( fabs( tps.E[a] - tps.E[b] ) < db_small_energy   &&
       fabs( tps.x[a] - tps.x[b] ) < db_small_distance &&
       fabs( tps.y[a] - tps.y[b] ) < db_small_distance )
    return true;
  
  return false;
}


//=====================================================================================
////merge two spots into 1 spot
void tpsdata_merge(TPSdata &tps, unsigned long int a, unsigned long int b){
  if (a<0 || a>= tps.nspots || b<0 || b>= tps.nspots) {
    logfile_error("tpsdata : tpsdata_merge : invalid index %u %u %u\n", a, b, tps.nspots);
    return;
  }
  if (a == b) {
    logfile_error("tpsdata : tpsdata_merge : merging the same indeces %u %u %u\n", a, b, tps.nspots);
    return; //cowardly refusing to merge a spot with itself  
  }
  if ( fabs( tps.E[a] - tps.E[b] ) < db_small_energy   &&
       fabs( tps.x[a] - tps.x[b] ) < db_small_distance &&
       fabs( tps.y[a] - tps.y[b] ) < db_small_distance ) {
    tps.q.at(a) += tps.q.at(b);
    tpsdata_delete(tps,b);
  }
  else
    logfile_error("tpsdata : tpsdata_merge : attempting to merge spots at different energy or position\n");
}


//=====================================================================================
////Convert spot weight to Gp
void tpsdata_convert_q_gp(TPSdata &tps){
  for (unsigned long int ii=0; ii<tps.nspots; ++ii) {
    double factor = 1;
    switch (tps.q_unit) {
    case GIGAPROTONS:
      factor = 1 ;  
      break;
    case PROTON_CHARGE: //Coulomb
      factor = 1.0/(db_C_per_Gp);
      break;
    case IC_CHARGE:  //Coulomb
      factor = 1.0/(db_C_per_Gp * IC2_GAIN(tps.E.at(ii)));
      break;
    case MU:
      factor = 1.0/(db_C_per_Gp * IC2_GAIN(tps.E.at(ii))*db_MU_per_IC_charge);
      break;
    default :
      logfile_error("tpsdata : tpsdata_convert_q_gp : The spot weight unit %d was not identified\n",tps.q_unit);
      factor = 1;
      return;
    }
    if (factor !=1)
      tps.q.at(ii) = tps.q.at(ii)*factor;
  }
  tps.q_unit = GIGAPROTONS;    
}

//=====================================================================================
////Convert spot weight to proton charge
void tpsdata_convert_q_proton_charge(TPSdata &tps){
  for (unsigned long int ii=0; ii<tps.nspots; ++ii) {
    double factor = 1;
    switch (tps.q_unit) {
    case GIGAPROTONS:
      factor = db_C_per_Gp;  
      break;
    case PROTON_CHARGE: //Coulomb
      factor = 1;
      break;
    case IC_CHARGE:  //Coulomb
      factor = 1.0/(IC2_GAIN(tps.E.at(ii)));
      break;
    case MU:
      factor = 1.0/(IC2_GAIN(tps.E.at(ii))*db_MU_per_IC_charge);
      break;
    default :
      logfile_error("tpsdata : tpsdata_convert_q_proton_charge : The spot weight unit %d was not identified\n",tps.q_unit);
      factor = 1;
      return;
    }
    if (factor !=1)
      tps.q.at(ii) = tps.q.at(ii)*factor;
  }
  tps.q_unit = PROTON_CHARGE;    
}

//=====================================================================================
////Convert spot weight to MU
void tpsdata_convert_q_mu(TPSdata &tps){
  for (unsigned long int ii=0; ii<tps.nspots; ++ii) {
    double factor = 1;
    switch (tps.q_unit) {
    case GIGAPROTONS:
      factor = db_C_per_Gp * IC2_GAIN(tps.E.at(ii))*db_MU_per_IC_charge ;  
      break;
    case PROTON_CHARGE:
      factor = IC2_GAIN(tps.E.at(ii))*db_MU_per_IC_charge;
      break;
    case IC_CHARGE:
      factor = db_MU_per_IC_charge;
      break;
    case MU:
      factor = 1;
      break;
    default :
      logfile_error("tpsdata : tpsdata_convert_q_mu : The spot weight unit %d was not identified\n",tps.q_unit);
      factor = 1;
      return;
    }
    if (factor !=1)
      tps.q.at(ii) = tps.q.at(ii)*factor;
  }
  tps.q_unit = MU;  
}

//=====================================================================================
////Convert spot weight to IC_CHARGE (kTP corrected C of charge on IC23 i.e. like Pyramid)
void tpsdata_convert_q_ic_charge(TPSdata &tps){
  for (unsigned long int ii=0; ii<tps.nspots; ++ii) {
    double factor = 1;
    switch (tps.q_unit) {
    case GIGAPROTONS:
      factor = db_C_per_Gp * IC2_GAIN(tps.E.at(ii)) ;  
      break;
    case PROTON_CHARGE:
      factor = IC2_GAIN(tps.E.at(ii));
      break;
    case IC_CHARGE:
      factor = 1;
      break;
    case MU:
      factor = 1/db_MU_per_IC_charge;
      break;
    default :
      logfile_error("tpsdata : tpsdata_convert_q_ic_charge : The spot weight unit %d was not identified\n",tps.q_unit);
      factor = 1;
      return;
    }
    if (factor !=1)
      tps.q.at(ii) = tps.q.at(ii)*factor;
  }
  tps.q_unit = IC_CHARGE;  
}

//=====================================================================================
//Convert distance units to mm
void tpsdata_convert_dist_mm(TPSdata &tps){
  double factor = 1;
  switch (tps.dist_unit) {
  case MM:
    factor = 1;
    break;
  case M:
    factor = 1000;
    break;
  case CM:
    factor = 10;
    break;
  default :
    logfile_error("tpsdata : tpsdata_convert_dist_mm : The spot dist unit %d was not identified\n",tps.dist_unit);
    factor = 1;
    return;
  }
  if (factor !=1) {
    for (unsigned long int ii=0; ii<tps.nspots; ++ii) {
      tps.x.at(ii) = tps.x.at(ii)*factor;
      tps.y.at(ii) = tps.y.at(ii)*factor;
    }
  }
  tps.dist_unit = MM;  
}

//=====================================================================================
//Convert distance units to m
void tpsdata_convert_dist_m(TPSdata &tps){
  double factor = 1;
  switch (tps.dist_unit) {
  case MM:
    factor = 0.001;
    break;
  case M:
    factor = 1;
    break;
  case CM:
    factor = 0.01;
    break;
  default :
    logfile_error("tpsdata : tpsdata_convert_dist_m : The spot dist unit %d was not identified\n",tps.dist_unit);
    factor = 1;
    return;
  }
  if (factor !=1) {
    for (unsigned long int ii=0; ii<tps.nspots; ++ii) {
      tps.x.at(ii) = tps.x.at(ii)*factor;
      tps.y.at(ii) = tps.y.at(ii)*factor;
    }
  }
  tps.dist_unit = M;  
}

//=====================================================================================
//Convert distance units to cm
void tpsdata_convert_dist_cm(TPSdata &tps){
  double factor = 1;
  switch (tps.dist_unit) {
  case MM:
    factor = 0.1;
    break;
  case M:
    factor = 100;
    break;
  case CM:
    factor = 1;
    break;
  default :
    logfile_error("tpsdata : tpsdata_convert_dist_cm : The spot dist unit %d was not identified\n",tps.dist_unit);
    factor = 1;
    return;
  }
  if (factor !=1) {
    for (unsigned long int ii=0; ii<tps.nspots; ++ii) {
      tps.x.at(ii) = tps.x.at(ii)*factor;
      tps.y.at(ii) = tps.y.at(ii)*factor;
    }
  }
  tps.dist_unit = CM;  
}

//=====================================================================================
//Convert coordinate system to IEC
void tpsdata_convert_coord_iec(TPSdata &tps){
  switch (tps.coord_sys) {
  case IEC:
    break;
  case IBA:
    for (unsigned long int ii=0; ii<tps.nspots; ++ii) {
      double tmp_x = tps.x.at(ii);
      tps.x.at(ii) = tps.y.at(ii);
      tps.y.at(ii) = tmp_x;
    }
    break;
  default :
    logfile_error("tpsdata : tpsdata_convert_coord_iec : The coordinate system %d was not identified\n",tps.coord_sys);
    return;
  }
  tps.coord_sys = IEC;  
}

//=====================================================================================
//Convert coordinate system to IBA
void tpsdata_convert_coord_iba(TPSdata &tps){
  switch (tps.coord_sys) {
  case IBA:
    break;
  case IEC:
    for (unsigned long int ii=0; ii<tps.nspots; ++ii) {
      double tmp_x = tps.x.at(ii);
      tps.x.at(ii) = tps.y.at(ii);
      tps.y.at(ii) = tmp_x;
    }
    break;
  default :
    logfile_error("tpsdata : tpsdata_convert_coord_iba : The coordinate system %d was not identified\n",tps.coord_sys);
    return;
  }
  tps.coord_sys = IBA;  
}

//=====================================================================================
//Apply gantry sag offset
void tpsdata_apply_offset(TPSdata &tps){
  switch (tps.offset_applied) {
  case APPLIED:
    break;
  case NOTAPPLIED:
    {
      TPSDATA_DIST_UNITS orig_dist_unit=tps.dist_unit;
      TPSDATA_COORD_SYS orig_coord_sys=tps.coord_sys;
      double ga = tps.header.gantry;  //deg
      double x_alignment_shift = ( db_alignment_x4*pow(ga,4)+
				   db_alignment_x3*pow(ga,3)+
				   db_alignment_x2*pow(ga,2)+
				   db_alignment_x1*pow(ga,1)+
				   db_alignment_x0);  //shift is in [mm] in IBA system
      double y_alignment_shift = ( db_alignment_y4*pow(ga,4)+
				   db_alignment_y3*pow(ga,3)+
				   db_alignment_y2*pow(ga,2)+
				   db_alignment_y1*pow(ga,1)+
				   db_alignment_y0);  //shift is in [mm] in IBA system
      logfile_printf("Gantry(deg)=%f, Alignment shift X(mm)=%f Y(mm)=%f in IBA coords\n",
		     ga, x_alignment_shift, y_alignment_shift);
      tpsdata_convert_dist_mm(tps);
      tpsdata_convert_coord_iba(tps);
      for (unsigned long int ii=0; ii<tps.nspots; ++ii) {
	tps.x.at(ii) = tps.x.at(ii)+x_alignment_shift;
	tps.y.at(ii) = tps.y.at(ii)+y_alignment_shift;
      }
      //convert back to original distance units and coordinate system
      switch (orig_dist_unit) {
      case MM:
	tpsdata_convert_dist_mm(tps);
	break;
      case M:
	tpsdata_convert_dist_m(tps);
	break;
      case CM:
	tpsdata_convert_dist_cm(tps);
	break;
      default :
	logfile_error("tpsdata : tpsdata_apply_offset : The spot dist unit %d was not identified\n",tps.dist_unit);
	return;
      }  
      switch (orig_coord_sys) {
      case IBA:
	tpsdata_convert_coord_iba(tps);
	break;
      case IEC:
	tpsdata_convert_coord_iec(tps);
	break;
      default :
	logfile_error("tpsdata : tpsdata_apply_offset : The coordinate system %d was not identified\n",tps.coord_sys);
	return;
      }
      break;
    }
  default : 
    logfile_error("tpsdata : tpsdata_apply_offset : The  apply_offset flag %d was not identified\n",tps.offset_applied);
    return;
  }
  tps.offset_applied = APPLIED;  
}

//=====================================================================================
//REMOVE gantry sag offset
void tpsdata_remove_offset(TPSdata &tps){
  switch (tps.offset_applied) {
  case NOTAPPLIED:
    break;
  case APPLIED:
    {
      TPSDATA_DIST_UNITS orig_dist_unit=tps.dist_unit;
      TPSDATA_COORD_SYS orig_coord_sys=tps.coord_sys;
      double ga = tps.header.gantry;  //deg
      double x_alignment_shift = ( db_alignment_x4*pow(ga,4)+
				   db_alignment_x3*pow(ga,3)+
				   db_alignment_x2*pow(ga,2)+
				   db_alignment_x1*pow(ga,1)+
				   db_alignment_x0);  //shift is in [mm] in IBA system
      double y_alignment_shift = ( db_alignment_y4*pow(ga,4)+
				   db_alignment_y3*pow(ga,3)+
				   db_alignment_y2*pow(ga,2)+
				   db_alignment_y1*pow(ga,1)+
				   db_alignment_y0);  //shift is in [mm] in IBA system
      tpsdata_convert_dist_mm(tps);
      tpsdata_convert_coord_iba(tps);
      for (unsigned long int ii=0; ii<tps.nspots; ++ii) {
	tps.x.at(ii) = tps.x.at(ii)-x_alignment_shift;
	tps.y.at(ii) = tps.y.at(ii)-y_alignment_shift;
      }
      //convert back to original distance units and coordinate system
      switch (orig_dist_unit) {
      case MM:
	tpsdata_convert_dist_mm(tps);
	break;
      case M:
	tpsdata_convert_dist_m(tps);
	break;
      case CM:
	tpsdata_convert_dist_cm(tps);
	break;
      default :
	logfile_error("tpsdata : tpsdata_remove_offset : The spot dist unit %d was not identified\n",tps.dist_unit);
	return;
      }  
      switch (orig_coord_sys) {
      case IBA:
	tpsdata_convert_coord_iba(tps);
	break;
      case IEC:
	tpsdata_convert_coord_iec(tps);
	break;
      default :
	logfile_error("tpsdata : tpsdata_remove_offset : The coordinate system %d was not identified\n",tps.coord_sys);
	return;
      }
      break;
    }
  default : 
    logfile_error("tpsdata : tpsdata_remove_offset : The  apply_offset flag %d was not identified\n",tps.offset_applied);
    return;
  }
  tps.offset_applied = NOTAPPLIED;  
}



bool isInteger(char  str[]) {
  unsigned long int ii = 0;
  bool success = true;
  while (str[ii] != '\0' && success) {
    if ( !isdigit(str[ii]) && str[ii] != '-') success = false;
    ii += 1;
  }
  if (ii==0)
    success = false;
  return success;
} 


bool isFloat(char  str[]) {
  unsigned long int ii = 0;
  bool success = true;
  while (str[ii] != '\0' && success) {
    if ( !isdigit(str[ii]) && str[ii] != '.'  && str[ii] != '-' && str[ii] != 'e') success = false;
    ii += 1;
  }
  if (ii==0)
    success = false;
  return success;

}

bool isFloatArray(char  str[]) {
  unsigned long int ii = 0;
  bool success = true;
  while (str[ii] != '\0' && success) {
    if ( !isdigit(str[ii]) && str[ii] != '.'  && str[ii] != '-' && str[ii] != 'e' && str[ii] != 'E' && str[ii] != ' ' && str[ii] != '\t') success = false;
    ii += 1;
  }
  if (ii==0)
    success = false;
  return success;

}



// //=====================================================================================
// //swap 2 spots to change the spot ordering
// void tpsdata_swap(TPSdata &tps, unsigned long int a, unsigned long int b) {
//   if (a>= tps.nspots || b>= tps.nspots) {
//     report<<"ERROR: TPSdata.swap() invalid index"<<endl;
//     return false;
//   }
//   double dtmp;
//   dtmp = tps.E[a]; tps.E[a] = tps.E[b]; tps.E[b] = dtmp;
//   dtmp = tps.x[a]; tps.x[a] = tps.x[b]; tps.x[b] = dtmp;
//   dtmp = tps.y[a]; tps.y[a] = tps.y[b]; tps.y[b] = dtmp;
//   dtmp = tps.q[a]; tps.q[a] = tps.q[b]; tps.q[b] = dtmp;
// }



// //=====================================================================================
// //spot a position < spot b position
// bool tpsdata_lessthan(TPSdata &tps, unsigned long int a, unsigned long int b){
//   if (a>= tps.nspots || b>= tps.nspots ) {
//     report<<"ERROR: TPSdata.lessthan() invalid index"<<endl;
//     return false;
//   }

//   if ( tps.E[a] < tps.E[b] )
//     return false;
//   if ( tps.E[a] > tps.E[b] )
//     return true;
//   if ( tps.x[a] < tps.x[b] )
//     return true;
//   if ( tps.x[a] > tps.x[b] )
//     return false;
//   if ( tps.y[a] < tps.y[b] )
//     return true;
//   if ( tps.y[a] > tps.y[b] )
//     return false;

//   //spots are equal
//   return false;
// }

// //=====================================================================================
// //is spot a position < spot b position given spot c is the previous spot
// bool tpsdata_lessthan(TPSdata &tps, unsigned long int a, unsigned long int b, unsigned long int c){
//   if (a>= tps.nspots || b>= tps.nspots || c>= tps.nspots) {
//     report<<"ERROR: TPSdata.lessthan() invalid index"<<endl;
//     return false;
//   }

//   if ( tps.E[a]< tps.E[b])
//     return false;
//   if ( tps.E[a]> tps.E[b])
//     return true;
//   if ( tps.E[a]!= tps.E[c])
//     return false;
//   if ( tps.x[a]< tps.x[b])
//     return true;
//   if ( tps.x[a]> tps.x[b])
//     return false;
//   if ( fabs(tps.y[a]- tps.y[c])< fabs(tps.y[b]- tps.y[c]))
//     return true;
//   if ( fabs(tps.y[a]- tps.y[c]) > fabs(tps.y[b]- tps.y[c]))
//     return false;

//   //same distance to spot c
//   return false;
// }
