#ifndef _SHADER_
#pragma once
#endif

struct SResamplerParams
{

#ifdef _SHADER_

  float3 sourceVolOrigin;
  float  pad1;
  float3 sourceVolSize;
  float  pad2;

  float3 destVoxOrigin;
  float  pad3;
  float3 destVoxSize;
  float  pad4;
  uint3  destNumVox;
  uint   pad5;
  uint   destNumVoxSlice;
  uint   destNumVoxVolume;

#else

  float sourceVolOrigin[3];
  float pad1;
  float sourceVolSize[3];
  float pad2;

  float destVoxOrigin[3];
  float pad3;
  float destVoxSize[3];
  float pad4;
  unsigned int destNumVox[3];
  unsigned int pad5;
  unsigned int destNumVoxSlice;
  unsigned int destNumVoxVolume;

#endif

  float pad[2];
};

//struct StcVoxelGrid
//{
//
//#ifdef _SHADER_
//
//  float3 gridOrigin;
//  float3 gridMax;
//  float3 gridSize;
//
//  float3 voxelOrigin;
//  float3 voxSize;
//
//  uint  numVoxDims;
//  uint  numVoxSlice;
//  uint  numVoxGrid;
//
//#else
//
//  float gridOrigin[3];
//  float gridMax[3];
//  float gridSize[3];
//
//  float voxelOrigin[3];
//  float voxSize[3];
//
//  uint  numVoxDims[3];
//  uint  numVoxSlice;
//  uint  numVoxGrid;
//
//#endif
//
//  float pad[4];
//};