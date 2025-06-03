#ifndef BP_TPS_H
#define BP_TPS_H

#include <vector>
#include <iostream>
#include <fstream>
#include <cmath>
#include <sstream>   

#include "database.h"
#include "vector_util.h"
#include "logfile.h"

//rows in this file correspond to deliverable energies
struct STEPS
{
  std::vector<double> r90, r80, E;
};

//rows in this file correspond to energies in BP database 
struct OPTICS
{
  std::vector<double> Ax, Ay, Az, Bx, By, Bz, wfit;
};


//rows in this file correspond to energies in BP database 
struct TBL
{
  std::vector<double> r100, r90, w80, E;
};

//rows in this file correspond to depth-dose values for all Bragg peaks 
struct PP
{
  std::vector<double> r90, depth, dose;
  std::vector<unsigned long int> pp_numb;
  std::vector<unsigned long int> pp_len;
};



struct BP_TPS
{
  STEPS steps;
  OPTICS optics;
  TBL tbl;
  PP pp;
  bool success;
};

BP_TPS  bp_tps_read(const char steps_file_name[],const char optics_file_name[],
       const char tbl_file_name[], const char pp_dir_name[]);

//extract a vector of dose from the database interpolated to energy "layer_energy"
std::vector<double> bp_tps_vdose(BP_TPS &bp_tps,double layer_energy, 
				 std::vector<double> depth);
double bp_tps_r80(std::vector<double> depth, std::vector<double> dose);
void bp_tps_print(BP_TPS &bp_tps);

//extract dose at a single depth
double bp_tps_dose(BP_TPS &bp_tps,double layer_energy, double depth);

double bp_tps_mcs(double depth, double r80);  //MCS parameterization
#endif
