#define _SHADER_

#include "ShaderResample.h"

// Constant Buffers
cbuffer ParamsBuffer : register(b0)
{
  SResamplerParams params;
}

// Resource Buffers
Texture3D<float> interpTexture : register(t0);
Texture3D<float> baseTexture   : register(t1);

// Unordered Access Buffers
RWTexture3D<float> sumTexture  : register(u0);

// Samplers
SamplerState resampler         : register(s0);

[numthreads(1024, 1, 1)]
void DoseSumCS(uint3 threadID : SV_DispatchThreadID )
{
  uint destVoxelIndex = threadID[0];

  if (destVoxelIndex > params.destNumVoxVolume)
  {
    return;
  }

  uint3 voxel;
  uint numSliceVox;
  uint numRowVox;

  voxel.z     = destVoxelIndex / params.destNumVoxSlice;

  numSliceVox = params.destNumVoxSlice * voxel.z;
  voxel.y     = (destVoxelIndex - numSliceVox) / params.destNumVox[0];

  numRowVox   = params.destNumVox[0] * voxel.y;
  voxel.x     = destVoxelIndex - numSliceVox - numRowVox;

  float3 destPos;
  float3 interpTexPos;

  destPos = params.destVoxOrigin + voxel * params.destVoxSize;
  interpTexPos = (destPos - params.sourceVolOrigin) / params.sourceVolSize;

  float destValue = interpTexture.SampleLevel(resampler, interpTexPos, 0);
   
  sumTexture[voxel.xyz] = baseTexture[voxel.xyz] + destValue;

  return;
}