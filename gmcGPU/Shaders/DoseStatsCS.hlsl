#define _SHADER_

// Constant Buffers
cbuffer DispatchBuffer : register(b0)
{
  unsigned int dispatchIndex;
  unsigned int cycleIndex;
  unsigned int numCyclesTotal;

  unsigned int numVoxSlice;
  unsigned int numVoxRow;

  unsigned int pad0[3];
};

// Resource Buffers


// Unordered Access Buffers
RWTexture3D<unsigned int> ubTxtrDose      : register(u0);
RWTexture3D<unsigned int> ubTxtrDoseAvrg  : register(u1);
RWTexture3D<unsigned int> ubTxtrDoseVarnc : register(u2);

[numthreads(1024, 1, 1)]
void DoseStatsCS(uint3 threadID : SV_DispatchThreadID)
{
  int3 vox;

  unsigned int voxelIndex = threadID[0] + dispatchIndex;

  vox[2] = voxelIndex / numVoxSlice;
  vox[1] = (voxelIndex - vox[2] * numVoxSlice) / numVoxRow;
  vox[0] = voxelIndex - vox[2] * numVoxSlice - vox[1] * numVoxRow;

  //float doseVal      = asfloat(ubTxtrDose.Load(vox)) * numCyclesTotal;

  float doseSum = asfloat(ubTxtrDose.Load(vox));
  float doseAvrgPrev = asfloat(ubTxtrDoseAvrg.Load(vox));

  float doseVal;
  if (cycleIndex == 0)
  {
    doseVal = doseSum * numCyclesTotal;
  }
  else
  {
    doseVal = doseSum * numCyclesTotal - doseAvrgPrev * cycleIndex;
  }
  

  float doseAvrgNew = (doseAvrgPrev * cycleIndex + doseVal) / (cycleIndex + 1);

  float varPrev = asfloat(ubTxtrDoseVarnc.Load(vox));
  float varNew  = (cycleIndex * varPrev + (doseVal - doseAvrgPrev) * (doseVal - doseAvrgNew)) / (cycleIndex + 1);

  ubTxtrDoseAvrg[vox.xyz]  = asuint(doseAvrgNew);
  ubTxtrDoseVarnc[vox.xyz] = asuint(varNew);

  //if (cycleIndex != (numCyclesTotal - 1))
  //{
  //  ubTxtrDose[vox.xyz]   = asuint(0.0);
  //}
  
  return;
}