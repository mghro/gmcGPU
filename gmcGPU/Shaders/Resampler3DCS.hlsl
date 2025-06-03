#define _SHADER_

#include "ShaderResample.h"

// Constant Buffers
cbuffer ParamsBuffer : register(b0)
{
  SResamplerParams params;
}

// Resource Buffers
Texture3D<float> sourceTexture  : register(t0);

// Unordered Access Buffers
RWTexture3D<float> destTexture  : register(u0);

// Samplers
SamplerState resampler          : register(s0);

[numthreads(1024, 1, 1)]
void Resampler3DCS(uint3 threadID : SV_DispatchThreadID )
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
  float3 sourceTexPos;

  destPos = params.destVoxOrigin + voxel * params.destVoxSize;
  sourceTexPos = (destPos - params.sourceVolOrigin) / params.sourceVolSize;

  float destValue = sourceTexture.SampleLevel(resampler, sourceTexPos, 0);
   
  destTexture[voxel.xyz] = destValue;

  return;
}