#define  _SHADER_

#include "ShaderRandom.h"
#include "ShaderPlan.h"

#define DBG_1        (1)
#define TWO_PI       (6.28318530718)
#define MAX_UINT     (4294967295.0)
#define SOURCE_SIGMA (36.0)

// Constant Buffers
cbuffer DispatchBuffer : register(b0)
{
  DISPATCH_SPOTPREP

  //unsigned int dispatchIndex;  \
  //unsigned int numSpots;       \
  //unsigned int numProtons;     \
  //unsigned int randSeedCycle;  \
  //unsigned int flagQA;         \
  //unsigned int pad0[3];
};

// Resource Buffers
StructuredBuffer<SShaderBeam>      rbBeams       : register(t0);
StructuredBuffer<SShaderControlPt> rbControlPts  : register(t1);
StructuredBuffer<SShaderSpot>      rbSpots       : register(t2);
StructuredBuffer<SShaderSpotCalc>  rbSpotscales  : register(t3);
StructuredBuffer<SShaderSpotCalc>  rbSOBPGrids   : register(t4);

// Unordered Access Buffers
RWStructuredBuffer<uint4>             ubRandSeeds  : register(u0);
RWStructuredBuffer<float3>            ubProtonsPos : register(u1);
RWStructuredBuffer<float3>            ubProtonsDir : register(u2);
RWStructuredBuffer<SShaderProtonsAux> ubProtonsAux : register(u3);
RWBuffer<float>                       ubSOBPHalos  : register(u4);

/*_____________________________________________________________________________________________

   SpotPrepCS - Compute Shader, Randomly initialize spot protons position, direction
_______________________________________________________________________________________________*/

[numthreads(1024, 1, 1)]
void SpotPrepSOBPCS(uint3 groupThreadID : SV_GroupThreadID)
{
  float randVal1;
  float randVal2;
  float inverseDirNorm;
  float isoPlanePosX;
  float isoPlanePosY;
  float sadScale;
  float protonInitZ;
  float dirX;
  float dirY;
  uint indexControlPt;
  uint indexBeam;
  uint indexProton;
  uint startSpotProton;
  uint endSpotProton;

  INITRAND128(ubRandSeeds[groupThreadID[0]]);

  for (uint indexSpot = 0; indexSpot < numSpots; ++indexSpot)
  {
    indexControlPt = rbSpots[indexSpot].indexControlPt;
    indexBeam      = rbControlPts[indexControlPt].indexBeam;
    
    startSpotProton = rbSpotscales[indexSpot].indexProtonsCompute + groupThreadID[0];
    endSpotProton   = rbSpotscales[indexSpot].indexProtonsCompute + rbSpotscales[indexSpot].numProtonsCompute;

    for (indexProton = startSpotProton; indexProton < endSpotProton; indexProton += 1024)
    {

      //INITRAND(rbSpotscales[indexSpot].randSeed + threadID[0] + randSeedCycle);

      sadScale = rbBeams[indexBeam].sadScale;
      protonInitZ = rbBeams[indexBeam].protonInitZ;

      ubProtonsPos[indexProton][1] = seedCurrent[0];
      ubProtonsPos[indexProton][2] = seedCurrent[3];

      //NEWRAND(randVal1)
      NEWRAND128(randVal1);
      randVal1 = sqrt(randVal1);

      ////NEWRAND(randVal2);
      NEWRAND128(randVal2);
      randVal2 *= TWO_PI;

      // Find proton pos in isocenter plane using Guassian around spot center
      isoPlanePosX = rbSpots[indexSpot].spotCenterX + rbControlPts[indexControlPt].spotSize[0] * randVal1 * cos(randVal2);
      isoPlanePosY = rbSpots[indexSpot].spotCenterY + rbControlPts[indexControlPt].spotSize[1] * randVal1 * sin(randVal2);

#ifdef DBG_0 //
      isoPlanePosX = rbControlPts[indexControlPt].spotSize[0] * randVal1 * cos(randVal2);
      isoPlanePosY = rbControlPts[indexControlPt].spotSize[1] * randVal1 * sin(randVal2);
#endif

      //NEWRAND(randVal1);
      NEWRAND128(randVal1);
      if (randVal1 == 0)
      {
        randVal1 = 1.0 / (float)MAX_UINT;
      }
      float sqrtVal1 = sqrt(-2.0 * log(randVal1));

      //NEWRAND(randVal2);
      NEWRAND128(randVal2);
      randVal2 *= TWO_PI;

  #ifdef DBG_0 // Turn off emittance;
      sqrtVal1 = 0.0;
  #endif

      // Smear location of proton in emittance plan
      ubProtonsPos[indexProton][0] = SOURCE_SIGMA * sqrtVal1 * cos(randVal2);
      ubProtonsPos[indexProton][1] = SOURCE_SIGMA * sqrtVal1 * sin(randVal2);
      ubProtonsPos[indexProton][2] = protonInitZ;

      // Calc proton trajectory
      dirX = isoPlanePosX - ubProtonsPos[indexProton][0];
      dirY = isoPlanePosY - ubProtonsPos[indexProton][1];

      inverseDirNorm = 1.0 / sqrt(dirX * dirX + dirY * dirY + ubProtonsPos[indexProton][2] * ubProtonsPos[indexProton][2]);

      // Normalize proton trajectory
      ubProtonsDir[indexProton][0] = dirX * inverseDirNorm;
      ubProtonsDir[indexProton][1] = dirY * inverseDirNorm;
      ubProtonsDir[indexProton][2] = -ubProtonsPos[indexProton][2] * inverseDirNorm;

#ifdef DBG_0 // Set proton init dir -1
      ubProtonsDir[indexProton][0] = 0;
      ubProtonsDir[indexProton][1] = 0;
      ubProtonsDir[indexProton][2] = -1;
#endif

      ubProtonsAux[indexProton].indexSpot = indexSpot;
      ubProtonsAux[indexProton].scaledPathLength = 0.0;

      float equivRadius = 40.0;
      ubSOBPHalos[indexProton] = 1 - exp(-equivRadius * equivRadius / (SIGMA_HALO_SQRDX2));

      }

  }

  STORESEED128(ubRandSeeds[groupThreadID[0]])

  return;
}