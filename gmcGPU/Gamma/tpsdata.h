#ifndef TPSDATA_H
#define TPSDATA_H

#include <vector>
#include <iostream>
#include <fstream>
#include <cmath>

#include "database.h"
#include "tramp_utils.h"
#include <string.h>

using namespace std;

//units for spot weight.  IC_CHARGE is kTP corrected Coulomb on IC23 
enum TPSDATA_Q_UNITS {GIGAPROTONS, PROTON_CHARGE, IC_CHARGE, MU}; 
enum TPSDATA_DIST_UNITS {MM, M, CM};
enum TPSDATA_COORD_SYS {IEC, IBA};
enum TPSDATA_OFFSET_APPLIED {APPLIED, NOTAPPLIED};

enum TPSDATA_FILTER_TYPE {ROUND,REDIS,CUT,NONE};
//                    int{  0  ,  1  , 2 , 3  } 

enum TPSDATA_DOSE_COMP_OVERRIDE {NOCOMP,COMP};

struct TPSdata_header
{
  string trampfilename;  //name of the tramp file
  string astroid_id;
  string patient_id;
  string patient_name_first;
  string patient_name_middle;
  string patient_name_last;
  string course_name;
  string beam_name;
  double gantry;

};

struct TPSdata
{
  unsigned long int nspots;  //number of non-zero spots in the .tramp file
  //Containers for spots from TPS
  //E at phantom (MeV), X(mm), Y(mm), Beam charge (Gp)
  std::vector<double> E, x, y, q;
  TPSDATA_Q_UNITS q_unit;
  TPSDATA_DIST_UNITS dist_unit;
  TPSDATA_COORD_SYS coord_sys;
  TPSDATA_OFFSET_APPLIED offset_applied;
  TPSDATA_FILTER_TYPE filter_type;

  TPSdata_header header;

  //******************************************
  //optional data specified in the .tramp file
  bool sort_spot_flag ;  //default is true.  i.e. sort the spots 
  double slew_faster_factorx; //IBA coordinates.  default is 1.0, no speed up
  double slew_faster_factory; //IBA coordinates.  default is 1.0
  double mult_dose_factor; //default is 1.0.  multiply output charge by this factor
  TPSDATA_DOSE_COMP_OVERRIDE dose_comparison_override;  //default is COMP.  i.e. do the gamma/dose comparison.  Use initials to override and set to false
  //******************************************
};


TPSdata tpsdata(const char * trampfilename);
void tpsdata_print(TPSdata &tps);
void tpsdata_write(TPSdata tps);
double tpsdata_sumq(TPSdata &tps);
void tpsdata_delete(TPSdata &tps, unsigned long int a); //delete a spot 
bool tpsdata_equal(TPSdata &tps, unsigned long int a, unsigned long int b); //are spots at the same E,X,Y
void tpsdata_merge(TPSdata &tps, unsigned long int a, unsigned long int b);  //merge two spots into 1 spot
void tpsdata_convert_q_gp(TPSdata &tps);
void tpsdata_convert_q_proton_charge(TPSdata &tps);
void tpsdata_convert_q_mu(TPSdata &tps);
void tpsdata_convert_q_ic_charge(TPSdata &tps);
void tpsdata_convert_dist_mm(TPSdata &tps);
void tpsdata_convert_dist_m(TPSdata &tps);
void tpsdata_convert_dist_cm(TPSdata &tps);
void tpsdata_convert_coord_iec(TPSdata &tps);
void tpsdata_convert_coord_iba(TPSdata &tps);
void tpsdata_apply_offset(TPSdata &tps);
void tpsdata_remove_offset(TPSdata &tps);
bool isInteger(char  str[]);
bool isFloat(char  str[]);
bool isFloatArray(char  str[]);

/* //obsolete */
/* void tpsdata_swap(TPSdata &tps, unsigned long int a, unsigned long int b);  //swap 2 spots to change the spot ordering */
/* void tpsdata_delete(TPSdata &tps, unsigned long int a); //delete a spot */
/* bool tpsdata_lessthan(TPSdata &tps, unsigned long int a, unsigned long int b); //spot a position < spot b position  */
/* bool tpsdata_lessthan(TPSdata &tps, unsigned long int a, unsigned long int b, unsigned long int c); //spot a position < spot b position given spot c is the previous spot */
/* bool tpsdata_equal(TPSdata &tps, unsigned long int a, unsigned long int b); //spot a position == spot b position  */

#endif
