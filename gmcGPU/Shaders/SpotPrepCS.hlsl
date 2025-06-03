#define  _SHADER_

#include "ShaderRandom.h"
#include "ShaderPlan.h"

#define DBG_1    (1)
#define TWO_PI   (6.28318530718)
#define SAD_QA   (10000000.0)
#define MAX_UINT (4294967295.0)

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

// Resource Buffer
StructuredBuffer<SShaderBeam>      rbBeams       : register(t0);
StructuredBuffer<SShaderControlPt> rbControlPts  : register(t1);
StructuredBuffer<SShaderSpot>      rbSpots       : register(t2);
StructuredBuffer<SShaderSpotCalc>  rbSpotscales  : register(t3);

// Unordered Access Buffers
RWStructuredBuffer<uint4>             ubRandSeeds   : register(u0);
RWStructuredBuffer<float3>            ubProtonsPos  : register(u1);
RWStructuredBuffer<float3>            ubProtonsDir  : register(u2);
RWStructuredBuffer<SShaderProtonsAux> ubProtonsAux  : register(u3);

/*_____________________________________________________________________________________________

   SpotPrepCS - Compute Shader, Randomly initialize spot protons position, direction
_______________________________________________________________________________________________*/

[numthreads(1024, 1, 1)]
void SpotPrepCS(uint3 groupThreadID : SV_GroupThreadID)
{

  //uint indexSpot = threadID.x;

  //if (indexSpot >= numSpots)
  //{
  //  return;
  //}

  float randVal1;
  float randVal2;
  float sqrtVal1;
  float sqrtVal2;
  float inverseDirNorm;
  float isoPlanePosX;
  float isoPlanePosY;
  float sadScale;
  float protonInitZ;
  float dirX;
  float dirY;
  uint indexControlPt;
  uint indexBeam;
  uint startSpotProton;
  uint endSpotProton;
  uint indexProton;
    

  INITRAND(groupThreadID[0] + randSeedCycle);

  for (uint indexSpot = 0; indexSpot < numSpots; ++indexSpot)
  {
    indexControlPt = rbSpots[indexSpot].indexControlPt;
    indexBeam = rbControlPts[indexControlPt].indexBeam;

    startSpotProton = rbSpotscales[indexSpot].indexProtonsCompute + groupThreadID[0];
    endSpotProton = rbSpotscales[indexSpot].indexProtonsCompute + rbSpotscales[indexSpot].numProtonsCompute;

    if (flagQA)
    {
      sadScale = 0.0;
      protonInitZ = SAD_QA;
    }
    else
    {
      sadScale = rbBeams[indexBeam].sadScale;
      protonInitZ = rbBeams[indexBeam].protonInitZ;
    }

    for (indexProton = startSpotProton; indexProton < endSpotProton; indexProton += 1024)
    {

      NEWRAND(randVal1)
        //NEWRAND128(randVal1);
        if (randVal1 == 0)
        {
          randVal1 = 1.0 / MAX_UINT;
        }
      sqrtVal1 = sqrt(-2 * log(randVal1));

      NEWRAND(randVal2);
      //NEWRAND128(randVal2);
      randVal2 *= TWO_PI;

      // Find proton pos in isocenter plane using Guassian around spot center
      isoPlanePosX = rbSpots[indexSpot].spotCenterX + rbControlPts[indexControlPt].spotSize[0] * sqrtVal1 * cos(randVal2);
      isoPlanePosY = rbSpots[indexSpot].spotCenterY + rbControlPts[indexControlPt].spotSize[1] * sqrtVal1 * sin(randVal2);

#ifdef DBG_0

      float s0 = sqrt(rbControlPts[indexControlPt].spotSize[0] * rbControlPts[indexControlPt].spotSize[0] + 16);
      float s1 = sqrt(rbControlPts[indexControlPt].spotSize[1] * rbControlPts[indexControlPt].spotSize[1] + 16);
      s0 = 0.000;
      s1 = 0.000;

      isoPlanePosX = rbSpots[indexSpot].spotCenterX + s0 * sqrtVal1 * cos(randVal2);
      isoPlanePosY = rbSpots[indexSpot].spotCenterY + s1 * sqrtVal1 * sin(randVal2);

      //isoPlanePosX = rbSpots[indexSpot].spotCenterX + 75.0 * sqrtVal1 * cos(randVal2);
      //isoPlanePosY = rbSpots[indexSpot].spotCenterY + 75.0 * sqrtVal1 * sin(randVal2);
#endif

    // Find proton pos in emittance plane
      if (rbBeams[indexBeam].sadMaxAxis == 1)
      {
        ubProtonsPos[indexProton][0] = 0.0;
        ubProtonsPos[indexProton][1] = isoPlanePosY * sadScale;
      }
      else
      {
        ubProtonsPos[indexProton][1] = 0.0;
        ubProtonsPos[indexProton][0] = isoPlanePosX * sadScale;
      }

      NEWRAND(randVal1);
      //NEWRAND128(randVal1);
      if (randVal1 == 0)
      {
        randVal1 = 1.0 / (float)MAX_UINT;
      }
      sqrtVal1 = sqrt(-2.0 * log(randVal1));

      NEWRAND(randVal2);
      //NEWRAND128(randVal2);
      randVal2 *= TWO_PI;

      float emitScale = rbBeams[indexBeam].emittance;

      // Smear location of proton in emittance plane
      ubProtonsPos[indexProton][0] += emitScale * rbControlPts[indexControlPt].spotSize[0] * sqrtVal1 * cos(randVal2);
      ubProtonsPos[indexProton][1] += emitScale * rbControlPts[indexControlPt].spotSize[1] * sqrtVal1 * sin(randVal2);

      ubProtonsPos[indexProton][2] = protonInitZ;

      NEWRAND(randVal2);
      //NEWRAND128(randVal2);
      randVal2 *= TWO_PI;

      // Invert cone, move isocenter pos by prev radius (emitScale*spotSize*sqrtVal1), random azimuth 
      isoPlanePosX += emitScale * rbControlPts[indexControlPt].spotSize[1] * sqrtVal1 * cos(randVal2);
      isoPlanePosY += emitScale * rbControlPts[indexControlPt].spotSize[1] * sqrtVal1 * sin(randVal2);

      // Calc proton trajectory
      dirX = isoPlanePosX - ubProtonsPos[indexProton][0];
      dirY = isoPlanePosY - ubProtonsPos[indexProton][1];

      inverseDirNorm = 1.0 / sqrt(dirX * dirX + dirY * dirY + ubProtonsPos[indexProton][2] * ubProtonsPos[indexProton][2]);

      // Normalize proton trajectory
      ubProtonsDir[indexProton][0] = dirX * inverseDirNorm;
      ubProtonsDir[indexProton][1] = dirY * inverseDirNorm;
      ubProtonsDir[indexProton][2] = -ubProtonsPos[indexProton][2] * inverseDirNorm;

      ubProtonsAux[indexProton].indexSpot = indexSpot;
      ubProtonsAux[indexProton].scaledPathLength = 0.0;

    }
  }
        
  
  return;
}