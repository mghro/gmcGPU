
#include "tramp_utils.h"

double ESS_TRANSMISSION_FUNC(double Ebeam){   // 0 to 1
  double Pbeam = sqrt(Ebeam*(Ebeam+2.0*db_Mp)); //MeV  Beam momentum
  double beam_rigidity = Pbeam/db_magnetic_rigidity_factor;
  return ( db_ess_transmission_1*exp(db_ess_transmission_2*beam_rigidity) );
}



double ENERGY_TO_RANGE(double Ebeam) { //convert Ebeam [MeV] to r80 in water [g/cm2]
  return exp (db_energy_to_range_coeff1*pow(log(Ebeam),3)
	      +db_energy_to_range_coeff2*pow(log(Ebeam),2)
	      +db_energy_to_range_coeff3*log(Ebeam)
	      +db_energy_to_range_coeff4);
}

double ENERGY_TO_SETRANGE(double Ebeam) { 
  return ENERGY_TO_RANGE(Ebeam)+db_Nwet;
}

double RANGE_TO_ENERGY(double range) { //convert r80 in water to Ebeam
  return exp (db_range_to_energy_coeff1*pow(log(range),3)
	      +db_range_to_energy_coeff2*pow(log(range),2)
	      +db_range_to_energy_coeff3*log(range)
	      +db_range_to_energy_coeff4);
}

double SETRANGE_TO_ENERGY(double setrange) { 
  return RANGE_TO_ENERGY(setrange-db_Nwet);
}


double NUMERICAL_SETRANGE_TO_ENERGY(double setrange) { //convert setrange [cm] to Ebeam at iso [MeV]

  double Eguess = SETRANGE_TO_ENERGY(setrange);
  //Newton method
  double f = ENERGY_TO_SETRANGE(Eguess)-setrange;
  double fprime = (ENERGY_TO_SETRANGE(Eguess+0.1)-ENERGY_TO_SETRANGE(Eguess-0.1))/0.2;
  Eguess = Eguess - f/fprime;

  return Eguess;
}


double IC2_GAIN(double Ebeam) {  // (IC2 nC) / (beam nC)
  return db_IC2_gain_coeff1*pow(Ebeam, db_IC2_gain_coeff2);


//   return db_IC2_gain_coeff1*pow(Ebeam,
// 			     db_IC2_gain_coeff2/Ebeam 
// 			     + db_IC2_gain_coeff3 
// 			     + db_IC2_gain_coeff4*Ebeam );
}




//return the index of the vector that is closest to val
unsigned long int FIND_CLOSEST(std::vector<double> &list, double val) {
  if (list.size()==0) {
    logfile_error("tramp_utils : FIND_CLOSEST : the list size in is zero\n");
    return 0;
  }

  unsigned long int min_index = 0;
  double min_diff = fabs(list.at(0) - val);
  for (unsigned long int ii=0; ii<list.size(); ++ii) {
    double diff = fabs(list.at(ii) - val);
    if (diff < min_diff) {
      min_diff = diff;
      min_index = ii;
    }
  }

  return min_index;
}

//Extract remaining characters from string str following the string delim.  
char* UTIL_EXTRACT_STRING( char *str, const char *delim) {
  int len = int(strlen(delim));
  int ii=0;
  while (str[ii+len] != '\0' && strncmp(str+ii, delim, len))  {ii +=1;}
  while (str[ii+len] != '\0' && str[ii+len]==' ') {ii +=1;}  //delete preceeding white spaces
  int jj=0;
  while (str[jj] != '\0' && jj < int(strlen(str)) ) {
    if (str[jj] == '\r') str[jj]='\0';  //editing text files with Word inserts these invisible characters
    jj += 1;
  }
  return (str+ii+len);
}


string UTIL_STRIP_PATH(string fullpath){

  string path = "."; //provide a default path
  int char_pos1=-1; 
  for (int nchar = fullpath.size()-1; nchar >=0; --nchar) {
    if (fullpath[nchar]=='/') {
       char_pos1 = nchar;
       break;
     }
  }
  if (char_pos1 != -1) {
    path.clear();
    for (int nchar = 0; nchar<=char_pos1; ++nchar) {
      path.push_back(fullpath[nchar]);
    }
    //path.push_back('\0');
  }
  return path;
}


