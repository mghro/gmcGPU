#ifndef DATABASE_H
#define DATABASE_H

//April 2012, astroid factor for RPC
//  const double db_astroid_factor = 1.0/0.985;
//Oct 2012 to April 1 2013.  Based on 0.8% from ref cube and 1.0% from halo dose
//  const double db_astroid_factor = 1.0/1.02;
//April 7 2013 to .  Based on additional 1% from ref cube and 1.0% from halo dose and 0.5% from low MU spots
  const double db_astroid_factor = 1.0/1.035;

const double db_ChargePerMU_IC2 =3.0e-9;  //  2.90604e-9;
  const double db_MU_per_IC_charge = 1.0/db_ChargePerMU_IC2; //Number of PBS MU's per Coulomb in signal from IC2 (MU/IC2_C)
  

  //Janni 82
  const double db_energy_to_range_coeff1 = -0.013411; //Energy to range converstion (1/MeV^3)
  const double db_energy_to_range_coeff2 = 0.15334; //Energy to range converstion (1/MeV^2)
  const double db_energy_to_range_coeff3 = 1.21961; //Energy to range converstion (1/MeV^1)
  const double db_energy_to_range_coeff4 = -5.50631; //Energy to range converstion (unitless)
  
  const double db_range_to_energy_coeff1 = 0.001073; 
  const double db_range_to_energy_coeff2 = -0.000807; 
  const double db_range_to_energy_coeff3 = 0.553081; 
  const double db_range_to_energy_coeff4 = 3.463601;
  
const double db_Nwet = 0.344; // 0.341; //Nozzle water equivalent thickness (g/cm2)
  
//2009
/*   //Use scaled DE/DX curve in abs_norm_depth_dose.xls */
/*   const double db_IC2_gain_coeff1 = 3.0*2053.789;  //coeff for IC2 charge / proton charge (unitless) */
/*   const double db_IC2_gain_coeff2 = -0.66555;     //coeff for IC2 charge / proton charge (MeV) */
/*   const double db_IC2_gain_coeff3 = -0.69715;     //coeff for IC2 charge / proton charge (unitless) */
/*   const double db_IC2_gain_coeff4 = -1.05678E-05;     //coeff for IC2 charge / proton charge (1/MeV) */
//Feb 2011
  //Use scaled DE/DX curve in Feb_2011_abs_norm_depth_dose.xls
  const double db_IC2_gain_coeff1 = db_ChargePerMU_IC2*1e9*12.3547*153.69;  //coeff for IC2 charge / proton charge (unitless)
  const double db_IC2_gain_coeff2 = -0.6891;     //Exponent of kinetic energy (assumes E in MeV)
  
  
  const double db_timeslice_duration = 250e-6; //s
  const double db_cmd_frequ = 1/db_timeslice_duration;  // From pyramid hardware,  1/(timeslice duration) (Hz)
  //Convert SMPS Volts to (X,Y) position at iso in m (units m/V). From sixteen (5/15/08) 
  //Use to treat the first patient
  const double db_iso_X_SMPS_X_coeff = 18785.0/1000.0; //relates SMPS volts to position at iso (m*MeV/V)
  const double db_iso_Y_SMPS_Y_coeff = 15474.0/1000.0; //relates SMPS volts to position at iso (m*MeV/V)
  
  const double db_SMPS_x_slew_rate = 0.057775*1000.0; //  X scanning magnet slew rate (V/s) (1.6 m/s at E=218 MeV)
  const double db_SMPS_y_slew_rate = 2.0*0.709515*1000.0; //  Y scanning magnet slew rate (V/s) (32.5  m/s at E=218 MeV)
  
  const double db_max_nozzleI = 2.0e-9; //maximum proton current in the nozzle 2.0e-9 (A)
/*   const double db_ISOx_per_ICx = 2.0;  //unitless  position at iso divided by position at ICx */
/*   const double db_ISOy_per_ICy = 2.0;  //unitless  position at iso divided by position at ICy */
  
//First patient until Feb 2012
//const double db_ess_transmission_1 = 1.96e-6; //unitless
//After tuning the trajectory of the beam in the gantry Feb 2012
  const double db_ess_transmission_1 = 3.02e-6; //unitless
  const double db_ess_transmission_2 = 5.28; //(1/T/m)
  
  const double db_max_cyclo_current = 275e-9; // maximum set-current in the MCR (A) 
  const double db_min_cyclo_current = 5e-9;   // minimum set-current in the MCR (A) 
  const double db_min_iseu_cmd = 4.0e-3; // (A)  ie 4 mA
  const double db_max_iseu_cmd = 20.0e-3; //(A)  ie 20 mA
  
  //PHYSICS CONSTANTS
  const double db_seconds_per_min = 60;  // (s/min)
  const double db_Mp = 938.272;  //Mass of the proton (MeV)
  const double db_charge_proton = 1.6022e-19; // (C)
  const double db_C_per_Gp = db_charge_proton/1.0e-9; // 0.16e-9 (C/Gp) 1 Gp = 0.16 nC of protons (~10.1 MU)
  const double db_magnetic_rigidity_factor = 299.792; // (MeV/T/m)
  
  //TRAMP PARAMETERS
/*   //Initial high-energy zero layer */
/*   const unsigned long int db_do_tune_layer =  0; //  // 1 = true, else is false */
/*   const unsigned long int db_num_zero_lay =  0; //  // */
  const double db_max_proton_range = 32.03; //g/cm2
  const double db_min_proton_range = 6.9269; //g/cm2
  const unsigned long int db_num_zero_slews = 8;
  const double db_range_step_zero_lay = 0.6; //g/cm2  delta_r between zero layers
  const double db_max_range_step = 2.1; //max change in setrange between layers before inserting zero layers //g/cm2
    
  const double db_small_energy = 0.01; //Energy tolerance for grouping spots into layers (MeV)
  //A small distance, spots are indistinguishable if they are separated by less than this amount
  const double db_small_distance = 0.0005; //(m)
  const double db_small_charge = 1e-13; // (IC Coulombs)
  const double db_small_current = 0.01e-9; // (cyclo amps)
  //DO not go above 3 (limit of resolution in scanalgo)
  const unsigned long int db_mu_decimal_places = 3; //(unitless) number of decimal places when writing MU's
  const double db_small_timeslices = 0.3; // a small number of timeslices for round-off error

  //Delays to wait for magnets/beam current to settle at a spot
  //  12/09/10  Scanalgo is doing the delay before spots
  // Do not set the delay smaller than 0.00025 ms or spots elements will become segment elements
  //Note Pyramid waits 4 slews (or 1ms) after shutting the beam off before moving
  const double db_x_delay2_small = 0.00025; //(s) for small X position changes
  const double db_x_delay2_large = 0.00025; //(s) for large X position changes
  const double db_y_delay2_small = 0.00025; //(s)
  const double db_y_delay2_large = 0.00025; //(s)
  const double db_I_delay2 =       0.00025; //(s) delay to turn off the beam
  const double db_large_x_change = 1.0 ; //defines when to use the small or large delay (V)
  const double db_large_y_change = 0.75 ; //defines when to use the small or large delay (V)

  //Gantry dependent position offset (mm) as a polynomial of position (deg)
  const double db_alignment_x4=    2.31979E-09 ;  // mm/deg^4
  const double db_alignment_x3=  -1.15432E-08 ;  // mm/deg^3
  const double db_alignment_x2=  -8.61164E-05  ;     
  const double db_alignment_x1=  -3.15721E-04 ;     
  const double db_alignment_x0=  2.04792E-01   ;  // mm
  
  const double db_alignment_y4= -7.1849E-10    ;  // mm/deg^4
  const double db_alignment_y3=  -5.1755E-07  ;  // mm/deg^3
  const double db_alignment_y2=  4.5459E-05	  ;     
  const double db_alignment_y1=  1.5643E-02	  ;     
  const double db_alignment_y0=  -2.5889E-01  ;  // mm
  
// WATCH OUT UNITS ARE METERS
//No shift
/*  const double db_x_alignment_shift =0.0/1000.0; //(m) shift in the HW X direction to align beam with BB */
/*  const double db_y_alignment_shift =0.0/1000.0; //(m) shift in the HW Y direction to align beam with BB */

//G000 12/04/2012 with 3cm shifter
/*  const double db_x_alignment_shift =0.0/1000.0; //(m) shift in the HW X direction to align beam with BB */
/*  const double db_y_alignment_shift =0.0/1000.0; //(m) shift in the HW Y direction to align beam with BB */

//G045 03/19/2013 with 3cm shifter
/*  const double db_x_alignment_shift =0.0/1000.0; //(m) shift in the HW X direction to align beam with BB */
/*  const double db_y_alignment_shift =0.0/1000.0; //(m) shift in the HW Y direction to align beam with BB */

//G090 12/26/2012 with 4.75cm shifter 
/*   const double db_x_alignment_shift =0.0/1000.0; //(m) shift in the HW X direction to align beam with BB */
/*   const double db_y_alignment_shift =1.5/1000.0; //(m) shift in the HW Y direction to align beam with BB */

//G=+180 12/26/2012 with 4.75cm shifter
/*  const double db_x_alignment_shift =0.0/1000.0; //(m) shift in the HW X direction to align beam with BB */
/*  const double db_y_alignment_shift =0.0/1000.0; //(m) shift in the HW Y direction to align beam with BB */

//G270 12/04/2012 with 3cm shifter
/*   const double db_x_alignment_shift =0.0/1000.0; //(m) shift in the HW X direction to align beam with BB */
/*   const double db_y_alignment_shift =-0.85/1000.0; //(m) shift in the HW Y direction to align beam with BB */

//G315 03/19/2013 with 3cm shifter
/*   const double db_x_alignment_shift =0.0/1000.0; //(m) shift in the HW X direction to align beam with BB */
/*   const double db_y_alignment_shift =-0.85/1000.0; //(m) shift in the HW Y direction to align beam with BB */

/* //G130 07/16/13 */
/*  const double db_x_alignment_shift =-0.6/1000.0; //(m) shift in the HW X direction to align beam with BB */
/*  const double db_y_alignment_shift =1.4/1000.0; //(m) shift in the HW Y direction to align beam with BB */

/* //G220 03/19/13 at 225, verified 7/15/13 at 220 */
/*  const double db_x_alignment_shift =-0.6/1000.0; //(m) shift in the HW X direction to align beam with BB */
/*  const double db_y_alignment_shift =-0.6/1000.0; //(m) shift in the HW Y direction to align beam with BB */

//REPEATED OR UNVERIFIED DATA
//G000 03/19/2013 with 3cm shifter
/*   const double db_x_alignment_shift =0.0/1000.0; //(m) shift in the HW X direction to align beam with BB */
/*   const double db_y_alignment_shift =0.0/1000.0; //(m) shift in the HW Y direction to align beam with BB */
//G030 12/04/2012 with 8cm shifter
/*   const double db_x_alignment_shift =0.0/1000.0; //(m) shift in the HW X direction to align beam with BB */
/*   const double db_y_alignment_shift =0.0/1000.0; //(m) shift in the HW Y direction to align beam with BB */
//G135 03/19/2013 with 3cm shifter
/*   const double db_x_alignment_shift =-0.9/1000.0; //(m) shift in the HW X direction to align beam with BB */
/*   const double db_y_alignment_shift =1.1/1000.0; //(m) shift in the HW Y direction to align beam with BB */
//G225 03/19/2013 with 3cm shifter
/*   const double db_x_alignment_shift =-0.6/1000.0; //(m) shift in the HW X direction to align beam with BB */
/*   const double db_y_alignment_shift =-0.8/1000.0; //(m) shift in the HW Y direction to align beam with BB */
//G270 03/19/2013 with 3cm shifter
/*   const double db_x_alignment_shift =-0.6/1000.0; //(m) shift in the HW X direction to align beam with BB */
/*   const double db_y_alignment_shift =-0.8/1000.0; //(m) shift in the HW Y direction to align beam with BB */
//G330 12/04/2012 with 8cm shifter
/*   const double db_x_alignment_shift =0.6/1000.0; //(m) shift in the HW X direction to align beam with BB */
/*   const double db_y_alignment_shift =-0.85/1000.0; //(m) shift in the HW Y direction to align beam with BB */



//NEW CROSS HAIR BUT OLD POSITION ALGORITHMS
//G180 and G200, CoM and Gaussabola algorithms 3/6/2012
/*   const double db_x_alignment_shift =-0.35/1000.0; //(m) shift in the HW X direction to align beam with BB */
/*   const double db_y_alignment_shift =-1.35/1000.0; //(m) shift in the HW Y direction to align beam with BB */
//G280, CoM and Gaussabola algorithms 3/11/2012
/*   const double db_x_alignment_shift =0.85/1000.0; //(m) shift in the HW X direction to align beam with BB */
/*   const double db_y_alignment_shift =-1.35/1000.0; //(m) shift in the HW Y direction to align beam with BB */


//OLD cross hair
//G000 
//  const double db_x_alignment_shift =0.75/1000.0; //(m) shift in the HW X direction to align beam with BB
//  const double db_y_alignment_shift =-0.75/1000.0; //(m) shift in the HW Y direction to align beam with BB
//G015
//  const double db_x_alignment_shift =1.0/1000.0; //(m) shift in the HW X direction to align beam with BB
//  const double db_y_alignment_shift =0.0/1000.0; //(m) shift in the HW Y direction to align beam with BB
//G060
//  const double db_x_alignment_shift =1.0/1000.0; //(m) shift in the HW X direction to align beam with BB
//  const double db_y_alignment_shift =0.75/1000.0; //(m) shift in the HW Y direction to align beam with BB
//G090
//  const double db_x_alignment_shift =1.9/1000.0; //(m) shift in the HW X direction to align beam with BB
//  const double db_y_alignment_shift =0.75/1000.0; //(m) shift in the HW Y direction to align beam with BB
//G180
//  const double db_x_alignment_shift =-1.4/1000.0; //(m) shift in the HW X direction to align beam with BB
// const double db_y_alignment_shift =-0.75/1000.0; //(m) shift in the HW Y direction to align beam with BB
  
  const double db_max_layer_mu = 5000;  //(s) maximum MU per layer
  const double db_min_layer_mu = 0.4;  //(s) maximum MU per layer
  const double db_max_layer_irrad_time = 10; //(s) maximum time allowed by scanning controller per layer
  const double db_min_layer_irrad_time = 0.6; //(s) minimum time allowed by scanning controller per layer
  const double db_max_layer_timeslices = 200000/3.0; //(ts) maximum time allowed by scanning controller per layer

  const double db_delta_Q_percent = 6.0; //3.5  percent
  const double db_MaxTurnOffTime = 280e-6;// seconds. Divide delta_MU at the smallest error by the smallest MU_dot.  The uncertainty in the time to stop the beam
  const double db_min_spot_mu_uncertainty = 1.54e-3; //  (1.75e-3 ,8e-4) (MU) best MU precision in a spot
  const double db_min_spot_mu_ic3 = 0.15; //was 0.15 in R3.  limit to low MU threshold due to IC3 (MU)
  const double db_min_spot_timeslices = 11; //(s) minimum timeslices allowed by scanning controller per spot.  Increased 5 to 11 on 8/22/11 b/c error in dose is too large for small spots.  
  const double db_max_spot_irrad_time = 8.190/3.0; //(s) maximum time allowed by scanning controller per spot
  const double db_max_spot_mu = 85; //(MU) max mu in a spot 
  const double db_pos_q_threshold =0.28043e-9; // 0.5e-9; //(C) IC charge threshold charge for FCU to calculate beam position.  Use Charge_PER_MU_IC2 = 2.90604e-9 C/MU
  const long int db_max_low_mu_spot_in_a_row = 5; //Maximum number of spots with weight < q_threshold in a row
  
 //field size given in IBA coordinates
  const double db_max_fieldx = 0.305; //0.305;  //(m) maximum full field size in the slow direction
  const double db_max_fieldy = 0.28; //0.28;  //(m) maximum full field size in the fast direction
  const double db_max_fieldx_cut = db_max_fieldx; //(m) full field size outside of which spots are cut
  const double db_max_fieldy_cut = db_max_fieldy; //

  const double db_max_field_mu = 9950;  //(MU) maximum number of MU allowed by DCEU
  //NEEDS REPORT 
  const double db_ISEU_dynamic_range = 30.0; // Could be 30, but use a smaller number until understood
  //NEEDS REPORT 
  const double db_max_ISEU_dynamic_range = 30.0; //Below which there is no beam, ie 4.3 mA
  //Give an error if the code wants to throw out a spot with weight above this max threshold
  const double db_hard_max_MU_threshold = 0.01; //(MU). 1MU == 0.1 Gp
  
  //const char db_steps_file_name[] = {"/home/bmc24/mgh/astroid/XiO/pbs.steps"};
  //  const char db_steps_file_name[] = {"/home/bmc24/mgh/bp_tps_db/03_15_11_sigma_rbe/pbs.steps"};
  // Windows
  //  const char db_steps_file_name[] = {"../03_15_11_sigma_rbe/pbs.steps"};
  const char db_steps_file_name[] = {"database/pbs.steps"};
  const char db_optics_file_name[] = {"database/pbs.optics"};
  const char db_tbl_file_name[] = {"database/pbs.tbl"};
  const char db_pp_dir_name[] = {"database/default/"};
  
  const float db_pi = 3.14159265358;
  const float db_grid_pitch = 3; //[mm] for writing mdose file
  const float db_pad_grid = 10; //[mm] margin around beam positions for adding voxels
  const int db_Nmax = 20; //Gamma index calc, maximum number of iterations
  const float db_d_gamma = 2.0; //mm distance tolerance in the gamma index
  const float db_D_gamma = 2.0; //%D1max dose tolerance in the gamma index
  const float db_gamma_thresh = 10; //%D1max threshold for the gamma mask TG-119
  const float db_gamma_pass_rate = 90.0; //% of voxels, required gamma pass rate in all slices and total field
  const float db_tolerated_dose_diff = 10.0; //%D1max maximum tolerated dose difference in any voxel
  const float db_halo_mask = 75.0; //%D1max for calculating the halo correction mask
  const float db_halo_lim = 5.0;   //% maximum allowed change for halo correction
  const float db_iso_depth = 170; //mm for mdose calculation 
  const double db_SADx    = 1.0e6; //;1.94; // m  IBA X for mdose calculation 
  const double db_SADy    = 1.0e6; //2.34; // m  IBA Y for mdose calculation 
#endif
  
