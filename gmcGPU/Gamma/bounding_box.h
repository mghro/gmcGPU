#ifndef BOUNDING_BOX_H
#define BOUNDING_BOX_H


#include "gamma_structs.h"
#include "vector_util.h"
#include "matrix.h"

void bounding_box(const vector<float> x0, const vector<float> y0, const vector<float> z0,
		  vector<float> &x1, vector<float> &y1, vector<float> &z1,
 		  Matrix<float> &D1, float thresh, float d_gamma);

#endif
