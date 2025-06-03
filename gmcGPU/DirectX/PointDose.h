#ifndef _SHADER_
#pragma once
#endif

struct SPointDoseParams
{
#ifdef _SHADER_
  float3 dosePoint;
#else
  SPt3 dosePoint;
#endif

  int   secondaryFlag;

};


