#define _SHADER_

// Constant Buffers
cbuffer DispatchBuffer : register(b0)
{
  uint dispatchIndex;
  uint numVoxels;

  uint pad0[2];
};

Texture3D<float> data               : register(t0);

RWStructuredBuffer<double2> bounds  : register(u0);

[numthreads(1024, 1, 1)]
void TextureMaxCS( uint3 threadID : SV_DispatchThreadID)
{
  uint width;
  uint height;
  uint depth;
  data.GetDimensions(width, height, depth);
  uint numVoxelsSlice = width * height;

  uint x;
  uint y;
  uint z;
  int4 voxel;
  float value;
  float maxValue = 0.0;
  float minValue = 100000;

  uint voxelIndex = threadID[0] + dispatchIndex;
  voxel.w = 0;

  while (voxelIndex < numVoxels)
  {
    voxel.z = voxelIndex / numVoxelsSlice;

    voxel.y = (voxelIndex - voxel.z * numVoxelsSlice) / width;

    voxel.x = voxelIndex - voxel.z * numVoxelsSlice - voxel.y * width;

    value = data.Load(voxel);

    if (value > maxValue)
    {
      maxValue = value;
    }

    if (value < minValue)
    {
      minValue = value;
    }

    voxelIndex += 1024;
  }

  bounds[threadID[0]][0] = minValue;
  bounds[threadID[0]][1] = maxValue;

  return;
}