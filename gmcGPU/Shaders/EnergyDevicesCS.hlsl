#define _SHADER_

#include "ShaderPlan.h"

// Constant buffers
cbuffer DispatchBuffer : register(b0)
{
  uint dispatchIndex;
  uint numProtons;

  uint pad0[2];
};

// Resource buffers
StructuredBuffer<SShaderProtonsAux> rbProtonsAux : register(t0);

// Output buffers
RWStructuredBuffer<double2> ubEnergyBins  : register(u0);

[numthreads(1024, 1, 1)]
void EnergyDevicesCS( uint3 threadID : SV_DispatchThreadID)
{
  float binEnergy = 0.0;

  unsigned int protonNum = threadID[0];

  while (protonNum < numProtons)
  {

   binEnergy += rbProtonsAux[protonNum].scaledPathLength; // Used to store cumulative energy losses of devices (done in PatientCS)
    
    protonNum += 1024;
  }

  double2 energy = { binEnergy, 0.0 };

  ubEnergyBins[threadID[0]] = energy;

  return;
}