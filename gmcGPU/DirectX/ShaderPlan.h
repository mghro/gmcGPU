
#ifndef _SHADER_
#pragma once
#endif



#define FLAG_PROTON_STOP    (314159)
#define RSP_BLANK           (0.01)

struct SDeviceMaterial
{
  float protonPathLength;
  float scaledDensity;
  float wer_msc_table[48];

};

struct SShaderDevice
{

#ifndef _SHADER_
  SShaderDevice()
  {
    numCompensators = 0;
    numApertures = 0;
  }
#endif

  int indexMtrl;
  int indexApertures;
  int numApertures;
  int indexCompensators;
  int numCompensators;

  float zTop;
  float zBottom;
  float halfRangeX;
  float halfRangeY;
};

struct SShaderAperture
{
  int indexMtrl;
  int indexAperturePts;
  int numAperturePts;

  float xMin;
  float xMax;
  float yMin;
  float yMax;
};

struct SShaderCompensator
{
  int indexPixelDepths;
  int numPixels;
  int numRows;
  int numColumns;

  float xMin;
  float xMax;
  float yMin;
  float yMax;
  float pixSizeX;
  float pixSizeY;

};

struct SSOBPGrid
{
  float upperLeft[2];
  float voxSizeX;
  float voxSizeY;

  unsigned int numVoxX;
  unsigned int numVoxY;
  unsigned int numVoxGrid;
};

struct SShaderBeam
{

#ifndef _SHADER_
  SShaderBeam()
  {
    numDevices    = 0;
    numControlPts = 0;
    numFractions  = 0;
    numSpots      = 0;
  }
#endif

  int   indexControlPts;
  int   numControlPts;
  int   indexSpots;
  int   indexDevices;
  int   numDevices;

  int   numSpots;
  float sad[2];
  float sadScale;
  float protonInitZ;
  float emittance;
  int   sadMaxAxis;

  int   numFractions;
};

struct SShaderControlPt
{
#ifdef _SHADER_

  row_major float3x3 beamMatrix;
  float3 isocenter;
  float3 isocenterQA;

#else

  SShaderControlPt()
  {
    numDeviceIndexes = 0;
    numSpots         = 0;
  }

  DirectX::XMFLOAT3X3 beamMatrix;
  float  isocenter[3];
  float  isocenterQA[3];

#endif

  float  spotSize[2];

  int    indexDeviceIndexes;
  int    numDeviceIndexes;
  int    indexBeam;
  int    indexSpots;
  int    numSpots;
  int    indexDepthDoses;
  int    numDepthDoses;
  int    indexLETs;
  int    numLETs;

  float  maxDepthDoseDepth;
  float  depthSpacingDepthDose;
  float  r80;

  float  maxDepthLET;
  float  depthSpacingLET;
};

struct SShaderSpot
{
  int indexControlPt;

  float spotCenterX;
  float spotCenterY;

  float weight;
  float spotHaloFactor;

};

struct SShaderSpotCalc
{
  float doseScale;
  float doseScaleQA;
  unsigned int indexProtonsCompute;
  unsigned int numProtonsCompute;
  unsigned int randSeed;
};

struct SShaderDijSpot
{
  unsigned int indexBin;
  unsigned int numBins;
  unsigned int mainDirAxis;
};

struct SShaderDijBin
{
  unsigned int indexVoxel;
  unsigned int numVoxels;
};

struct SShaderDijVoxel
{
  unsigned int key;

#ifdef _SHADER_
  unsigned int dose;
#else
  float dose;
#endif
};

struct SShaderProton
{

#ifdef _SHADER_
  float3 pos;
  float3 dir;
#else
  float pos[3];
  float dir[3];
#endif

  float scaledPathLength;

  int indexSpot;
};

struct SShaderProtonsAux
{
  float scaledPathLength;
  unsigned int indexSpot;
};

struct SShaderCalcGrid
{
  float voxSize[3];
  float volOrigin[3];
  float volSize[3];
  float voxelVolume;

  int dims[3];
  int numSliceVoxels;

  float orientation;

  float pad[1];
};

#define DISPATCH_SPOTPREP      \
  unsigned int dispatchIndex;  \
  unsigned int numSpots;       \
  unsigned int numProtons;     \
  unsigned int randSeedCycle;  \
  unsigned int flagQA;         \
  unsigned int pad0[3];

#define DISPATCH_PATIENT       \
unsigned int dispatchIndex;    \
unsigned int numProtons;       \
unsigned int seed;             \
unsigned int flagLET;          \
unsigned int flagQA;           \
unsigned int flagDij;          \
unsigned int flagSOBP;         \
unsigned int flagHCL;



#define SIGMA_HALO_SQRDX2  (2*30*30) // in mm^2





