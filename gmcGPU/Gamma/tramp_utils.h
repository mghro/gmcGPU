

#ifndef TRAMP_UTILS_H
#define TRAMP_UTILS_H

#include <iostream>
#include <iomanip>
#include <math.h>
#include <vector>
#include <fstream>
#include <cmath>
#include <stdlib.h>
#include <string.h>
#include <sstream>

#include "database.h"
#include "logfile.h"


using namespace std;


double ESS_TRANSMISSION_FUNC(double Ebeam);  // 0 to 1
double IC2_GAIN(double Ebeam);
double ENERGY_TO_RANGE(double Ebeam); //convert Ebeam [MeV] to r80 [g/cm2] in water
double RANGE_TO_ENERGY(double range); //convert r80 in water to Ebeam
double ENERGY_TO_SETRANGE(double Ebeam); //convert Ebeam at iso to setrange [g/cm2]
double SETRANGE_TO_ENERGY(double setrange); //convert setrange to Ebeam at iso
double NUMERICAL_SETRANGE_TO_ENERGY(double setrange); //convert setrange to Ebeam at iso

unsigned long int FIND_CLOSEST(std::vector<double> &list, double val);

char* UTIL_EXTRACT_STRING( char *str, const char *delim);
string UTIL_STRIP_PATH(string fullpath);


#endif
