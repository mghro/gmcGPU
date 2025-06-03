#define _SHADER_

#define BIO_FACTOR1   (1.1)
#define BIO_FACTOR2   (0.03)
#define RBE_FACTOR0   (0.99064)
#define RBE_FACTOR1   (0.35605)
#define RBE_FACTOR2   (1.1012)
#define RBE_FACTOR3   (0.0038703)
#define RBE_FACTOR4   (2 * 1.1)

// Constant Buffers
cbuffer DispatchBuff : register(b0)
{
  unsigned int dispatchIndex;
  unsigned int numVoxels;
  unsigned int numFractions;

  float ratio0;
  float ratio1;
  float ratio2;

  int pad[2];
};

// Texture Buffers
Texture3D<float>        rbTextureDose           : register(t0);
Texture3D<float>        rbTextureProtonCount    : register(t1);
Texture3D<unsigned int> rbTextureStructureMask  : register(t2);

// Strucured Buffers
StructuredBuffer<float> rbRatios                : register(t3);

// UAV Buffers
RWTexture3D<unsigned int> ubTextureLET          : register(u0);
RWTexture3D<float>        ubTextureBioDose      : register(u1);
RWTexture3D<float>        ubrbTextureDoseDivLET : register(u2);
RWTexture3D<float>        ubTextureRBE0         : register(u3);
RWTexture3D<float>        ubTextureRBE1         : register(u4);
RWTexture3D<float>        ubTextureRBE2         : register(u5);

/*_____________________________________________________________________________________________

  AuxDosesCS - Compute Shader calculate auxillary doses 
_______________________________________________________________________________________________*/

[numthreads(1024, 1, 1)]
void AuxDosesCS( uint3 threadID : SV_DispatchThreadID)
{
  unsigned int voxelIndex = threadID[0] + dispatchIndex;

  if (voxelIndex > numVoxels)
  {
    return;
  }

  unsigned int width;
  unsigned int height;
  unsigned int depth;

  rbTextureDose.GetDimensions(width, height, depth);

  unsigned int x;
  unsigned int y;
  unsigned int z;
  unsigned int numVoxelsSlice = width * height;

  z = voxelIndex / numVoxelsSlice;

  y = (voxelIndex - z * numVoxelsSlice) / width;

  x = voxelIndex - z * numVoxelsSlice - y * width;

  int4  voxel = { x, y, z, 0};
  float num   = rbTextureProtonCount.Load(voxel);

  if (num == 0.0)
  {
    return;
  }

  float dose       = rbTextureDose.Load(voxel);
  float scaledDose = dose / numFractions;
  float let        = asfloat(ubTextureLET.Load(voxel)) / rbTextureProtonCount.Load(voxel);

  ubTextureLET[voxel.xyz]     = asuint(let);
  ubTextureBioDose[voxel.xyz] = (dose / BIO_FACTOR1) * (1.0 + BIO_FACTOR2 * let);


  float arg1;
  float arg2;

  if (ratio0)
  {
    arg1 = 4 * scaledDose * ratio0 * (RBE_FACTOR0 + RBE_FACTOR1 * let / ratio0);
    arg2 = 2 * scaledDose * (RBE_FACTOR2 - RBE_FACTOR3 * sqrt(ratio0) * let);

    ubTextureRBE0[voxel.xyz] = (sqrt(ratio0 * ratio0 + arg1 + arg2 * arg2) - ratio0) * (numFractions / RBE_FACTOR4);
  }

  if (ratio1)
  {
    arg1 = 4 * scaledDose * ratio1 * (RBE_FACTOR0 + RBE_FACTOR1 * let / ratio1);
    arg2 = 2 * scaledDose * (RBE_FACTOR2 - RBE_FACTOR3 * sqrt(ratio1) * let);

    ubTextureRBE1[voxel.xyz] = (sqrt(ratio1 * ratio1 + arg1 + arg2 * arg2) - ratio1) * (numFractions / RBE_FACTOR4);
  }

  if (ratio2)
  {
    arg1 = 4 * scaledDose * ratio2 * (RBE_FACTOR0 + RBE_FACTOR1 * let / ratio2);
    arg2 = 2 * scaledDose * (RBE_FACTOR2 - RBE_FACTOR3 * sqrt(ratio2) * let);

    ubTextureRBE2[voxel.xyz] = (sqrt(ratio2 * ratio2 + arg1 + arg2 * arg2) - ratio1) * (numFractions / RBE_FACTOR4);
  }

  if (let)
  {
    ubrbTextureDoseDivLET[voxel.xyz] = scaledDose / let;
  }

  return;
}