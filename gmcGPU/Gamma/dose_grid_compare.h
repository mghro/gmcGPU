#ifndef DOSE_GRID_COMPARE_H
#define DOSE_GRID_COMPARE_H

#include "tgif.h"
#include "dose_engine.h"


//Compare two dose grids and write the result to the report file
void dose_grid_compare(DOSE_GRID &dose_grid_0, string unit0, string unitD0,
		       DOSE_GRID &dose_grid_1, string unit1, string unitD1,
		       float d_gamma, string unit_d, float D_gamma, string unit_D, 
		       float thresh, string unit_th);

#endif
