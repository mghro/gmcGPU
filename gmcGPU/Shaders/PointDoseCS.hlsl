#define _SHADER_

#include "PointDose.h"

struct SubResultsDose
{
  float results[8];
};

// Constant Buffers
cbuffer ParamsBuffer : register(b0)
{
  SPointDoseParams params;
}

cbuffer DoseMatrixPrim : register(b1)
{
  row_major float4x4 doseMatrixPrimary;
}

cbuffer DoseMatrixSec : register(b2)
{
  row_major float4x4 doseMatrixSecondary;
}

// Texture Buffers
Texture3D<float> rbTxtrImage     : register(t0);
Texture3D<float> rbTxtrDosePrim  : register(t1);
Texture3D<float> rbTxtrDoseSec   : register(t2);


// UAV Buffers
RWBuffer<float>  ubResultsDose : register(u0);

// Samplers
SamplerState resampler : register(s0);

[numthreads(1, 1, 1)]
void PointDoseCS( uint3 DTid : SV_DispatchThreadID )
{
  
  float3 doseVolPos;
  float3 doseVec;

  doseVolPos[0] = params.dosePoint[0] - doseMatrixPrimary[3][0];
  doseVolPos[1] = params.dosePoint[1] - doseMatrixPrimary[3][1];
  doseVolPos[2] = params.dosePoint[2] - doseMatrixPrimary[3][2];

  doseVec[0] = doseMatrixPrimary[0][0] * doseVolPos[0] + doseMatrixPrimary[0][1] * doseVolPos[1] + doseMatrixPrimary[0][2] * doseVolPos[2];
  doseVec[1] = doseMatrixPrimary[1][0] * doseVolPos[0] + doseMatrixPrimary[1][1] * doseVolPos[1] + doseMatrixPrimary[1][2] * doseVolPos[2];
  doseVec[2] = doseMatrixPrimary[2][0] * doseVolPos[0] + doseMatrixPrimary[2][1] * doseVolPos[1] + doseMatrixPrimary[2][2] * doseVolPos[2];

  ubResultsDose[3] = doseVec[0];
  ubResultsDose[4] = doseVec[1];
  ubResultsDose[5] = doseVec[2];
  
  ubResultsDose[0] = rbTxtrDosePrim.SampleLevel(resampler, doseVec, 0);
  ubResultsDose[6] = rbTxtrImage.SampleLevel(resampler, doseVec, 0);

  if (params.secondaryFlag)
  {
    doseVolPos[0] = params.dosePoint[0] - doseMatrixSecondary[3][0];
    doseVolPos[1] = params.dosePoint[1] - doseMatrixSecondary[3][1];
    doseVolPos[2] = params.dosePoint[2] - doseMatrixSecondary[3][2];

    doseVec[0] = doseMatrixSecondary[0][0] * doseVolPos[0] + doseMatrixSecondary[0][1] * doseVolPos[1] + doseMatrixSecondary[0][2] * doseVolPos[2];
    doseVec[1] = doseMatrixSecondary[1][0] * doseVolPos[0] + doseMatrixSecondary[1][1] * doseVolPos[1] + doseMatrixSecondary[1][2] * doseVolPos[2];
    doseVec[2] = doseMatrixSecondary[2][0] * doseVolPos[0] + doseMatrixSecondary[2][1] * doseVolPos[1] + doseMatrixSecondary[2][2] * doseVolPos[2];

    ubResultsDose[1] = rbTxtrDoseSec.SampleLevel(resampler, doseVec, 0);

    // 200  = 2 (for mean of both values) * 100 (convert to %)
    ubResultsDose[2] = 200 * (ubResultsDose[0] - ubResultsDose[1]) / (ubResultsDose[0] + ubResultsDose[1]);
  }
  else
  {
    ubResultsDose[1] = -1.0;
    ubResultsDose[2] = 0.0;
  }

  return;
}