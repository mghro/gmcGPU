#define _SHADER_
#define DBG_1 (1)

// Constant Buffers
cbuffer DispatchBuffer : register(b0)
{

  float4 originDose;
  float4 voxSizeDose;
  float4 extentImg;

  uint numVoxels;

  uint pad0[7];
};

// Resource Buffers
Texture3D<float> imageTexture      : register(t0);
Texture3D<float> doseTexture       : register(t1);

RWStructuredBuffer<double2> energy : register(u0);

// Samplers
SamplerState imgSampler            : register(s0);

[numthreads(1024, 1, 1)]
void EnergyPatientFreeCS( uint3 threadID : SV_DispatchThreadID)
{
  uint width;
  uint height;
  uint depth;
  uint numVoxelsSlice;
  double binEnergy = 0.0;
  double numVox = 0;

  uint voxelIndex = threadID[0];

  doseTexture.GetDimensions(width, height, depth);
  numVoxelsSlice = width * height;

  int4 voxel;
  float3 imgVec;
  voxel.w = 0;

  while (voxelIndex < numVoxels)
  {
    voxel.z = voxelIndex / numVoxelsSlice;
    voxel.y = (voxelIndex - voxel.z * numVoxelsSlice) / width;
    voxel.x = voxelIndex - voxel.z * numVoxelsSlice - voxel.y * width;

    imgVec[0] = (originDose[0] + voxel.x * voxSizeDose[0]) / extentImg[0];
    imgVec[1] = (originDose[1] + voxel.y * voxSizeDose[1]) / extentImg[1];
    imgVec[2] = (originDose[2] + voxel.z * voxSizeDose[2]) / extentImg[2];

    binEnergy += doseTexture.Load(voxel) * imageTexture.SampleLevel(imgSampler, imgVec, 0);
#ifdef DBG_0// Calc for all RSP = 1.0, comment out above line
    binEnergy += doseTexture.Load(voxel); 
#endif
    numVox += 1.0;

    voxelIndex += 1024;
  }

  double2 energy2 = { binEnergy, numVox };

  energy[threadID[0]] = energy2;

  return;
}