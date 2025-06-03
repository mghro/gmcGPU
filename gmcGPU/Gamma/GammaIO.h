#pragma once

struct SGammaResults
{
  bool statusError;
  int  numIterations;
  float fractionConverged;
  float convergenceFactor;
  float gammaMax;
};

struct SGammaThresholds
{
  float threshDistanceMM;
  float threshDosePrcnt;
  float threshDoseMinPrcnt;
};
