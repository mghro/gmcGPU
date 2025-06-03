
#ifndef READ_MDOSE_H
#define READ_MDOSE_H


#include <math.h>
#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <sstream>
//#include "vector_util.h"
using namespace std;


//(D,X,Y,Z) are vectors where (D(ii),X(ii),Y(ii),Z(ii)) specifies a grid_point
//X,Y [mm] are in IEC coordinate system
//Z [mm] points upstream
//D is the dose deposited at that voxel (same units as the Astroid database, GyRBE)
struct GRID_POINTS
{
  std::vector<float> D; //,x,y,z; //These vectors are the same size, each index represents a voxel
  std::vector<float> xg,yg,zg; //Axis vectors for the grid, matlab equiv is [x,y,z]= meshgrid(xg,yg,zg)
};


GRID_POINTS read_mdose( string &filename);
std::vector<float> parse_csv(string &str);

#endif
