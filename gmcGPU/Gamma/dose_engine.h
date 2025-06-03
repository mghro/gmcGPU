#ifndef DOSE_ENGINE_H
#define DOSE_ENGINE_H


#include "tpsdata.h"
#include "bp_tps.h"
#include <math.h>
#include <string.h>




//(X,Y,Z) are vectors where (X(ii),Y(jj),Z(kk)) specifies a grid_point
//E[MeV] is the beam energy used to calculate the grid point
//X,Y [mm] are in IEC coordinate system
//Z [mm] points upstream
// D is the dose in GyRBE.  It is a vector with index =kk*yg.size()*xg.size()+jj*xg.size()+ii
struct DOSE_GRID
{
  std::vector<double> D,xg,yg,zg;
};

//alpha is the strength of the primary gaussian (1-f_h), where f_h is from Pedroni et al
struct HALO_PARAM
{
  double alpha,sigma_h;
};



DOSE_GRID dose_engine(BP_TPS &bp_tps, TPSdata tps); //create dose grid and calculate dose
DOSE_GRID dose_engine(BP_TPS &bp_tps, TPSdata tps, std::vector<double> xg,  
		      std::vector<double> yg, std::vector<double> zg);  //calculate dose on specified grid
void dose_engine_write_mdose(TPSdata tps, DOSE_GRID dose_grid);  //write mdose file
HALO_PARAM dose_engine_halo_param(double depth, double R_pb); //Depth and R_pb are in [mm].  Range is R80

#endif
