#define _SHADER_
#define DBG_1

#include "ShaderRandom.h"
#include "ShaderPlan.h"

#define DOSE_SCALE   (1.0e-9)
#define JOULE_TO_MEV (6.42e-3)

struct STransportState
{
  int    inside;
  int4   vox;
  float  energy;
  float  scaledPathLength;
  float3 pos;
  float3 dir;
  int    voxelFace;
};

// Constant Buffers
cbuffer DispatchBuffer : register(b0)
{
  unsigned int dispatchIndex;
  unsigned int numProtons;
  unsigned int seed;
  unsigned int flagLET;
  unsigned int flagQA;
  unsigned int flagDij;
  unsigned int flagSOBP;
  unsigned int flagHCL;

  //unsigned int pad0[1];
};

cbuffer TransportParams : register(b1)
{
  float voxSize0;
  float voxSize1;
  float voxSize2;
  float volOrigin0;
  float volOrigin1;
  float volOrigin2;
  float volSize0;
  float volSize1;
  float volSize2;
  float voxelVolume;

  int dims0;
  int dims1;
  int dims2;
  int numSliceVoxels;

  float orientation;

  float pad1[1];
};

// Resource Buffers: Transport
Texture3D<float>                     rbVoxelRSP            : register(t0);
StructuredBuffer<SShaderBeam>        rbBeams               : register(t1);
StructuredBuffer<SShaderControlPt>   rbControlPts          : register(t2);
StructuredBuffer<SShaderSpot>        rbSpots               : register(t3);
StructuredBuffer<SShaderSpotCalc>    rbSpotScales          : register(t4);
Buffer<float>                        rbIntegratedDepthDose : register(t5);
Buffer<float>                        rbAlphas              : register(t6);
Buffer<float>                        rbLETs                : register(t7);

// Resource Buffers: Dij
StructuredBuffer<SShaderDijSpot>     rbDijSpots            : register(t8);
StructuredBuffer<SShaderDijBin>      rbDijBins             : register(t9);

Buffer<float>                        rbSOBPHalos           : register(t10);


// Unordered Access Buffers: Transport
RWStructuredBuffer<float3>            ubProtonsPos  : register(u0);
RWStructuredBuffer<float3>            ubProtonsDir  : register(u1);
RWStructuredBuffer<SShaderProtonsAux> ubProtonsAux  : register(u2);
RWTexture3D<unsigned int>             ubProtonCount : register(u3);
RWTexture3D<unsigned int>             ubDoseVolume  : register(u4);
RWTexture3D<unsigned int>             ubLETVolume   : register(u5);


// Unordered Access Buffers: Dij
RWStructuredBuffer<SShaderDijVoxel>  ubDijVoxel     : register(u6);

#ifdef DBG_0 // DijSpots
  RWBuffer<unsigned int>             ubDijFill      : register(u7);
#endif

float CalcProtonEnergyLoss(float depth, int indexControlPt)
{
  if (depth >= rbControlPts[indexControlPt].maxDepthDoseDepth)
  {
    return rbIntegratedDepthDose[rbControlPts[indexControlPt].indexDepthDoses + rbControlPts[indexControlPt].numDepthDoses - 1];
  }

  float value   = depth / rbControlPts[indexControlPt].depthSpacingDepthDose;
  int index     = (int)(value);
  int baseIndex = rbControlPts[indexControlPt].indexDepthDoses + index;

  return (rbIntegratedDepthDose[baseIndex] + (value - index) * (rbIntegratedDepthDose[baseIndex + 1] - rbIntegratedDepthDose[baseIndex]));
}

float InterpolateAlpha(float depth, int indexControlPt)
{
  if (depth > depth >= rbControlPts[indexControlPt].maxDepthDoseDepth)
  {
    return rbAlphas[rbControlPts[indexControlPt].indexDepthDoses + rbControlPts[indexControlPt].numDepthDoses - 1];
  }

  float value = depth / rbControlPts[indexControlPt].depthSpacingDepthDose;
  int index = (int)(value);
  int baseIndex = rbControlPts[indexControlPt].indexDepthDoses + index;

  return (rbAlphas[baseIndex] + (value - index) * (rbAlphas[baseIndex + 1] - rbAlphas[baseIndex]));
}

/*_____________________________________________________________________________________________

   TransportPatientCS - Compute Shader, Proton transport through patient volume
_______________________________________________________________________________________________*/

[numthreads(1024, 1, 1)]
void TransportPatientCS(uint3 thread : SV_DispatchThreadID)
{
  STransportState proton;
  float scatterAngle;
  float azAngle;
  float trialLength;
  float tLow;
  float tHigh;
  float tLowNext;
  float tHighNext;
  float initEnergy;
  float initEnergyMev;
  float energyMev;
  uint writeValue;
  uint checkValue;
  uint readValue;
  uint  loop = 0; // MLW dbg
  float sumCount = 0.0;
  float rspVal;
  float3 posImgVol;
  float3 tmp;

  // Dij variables
  uint spotBinIndex;
  uint binIndex;
  uint dirAxis;
  uint voxelKey;
  float mainVoxInit;
  float sumDose = 0.0;
  float sumEnergy = 0.0;
  float sumDij = 0.0;
  float sumVoxelDij = 0.0;

  INITRAND(seed + thread[0]);
  unsigned int protonNum = thread[0] + dispatchIndex;

  if (protonNum >= numProtons)
  {
    return;
  }

  int indexSpot = ubProtonsAux[protonNum].indexSpot;
  int indexControlPt = rbSpots[indexSpot].indexControlPt;

#ifdef DBG_0 // Initialize pos 0 dir -1
  indexSpot = 0;
  indexControlPt = 0;

  ubProtonsPos[protonNum][0] = 0.0;
  ubProtonsPos[protonNum][1] = 0.0;
  ubProtonsDir[protonNum][0] = 0.0;
  ubProtonsDir[protonNum][1] = 0.0;
  ubProtonsDir[protonNum][2] = -1.0;
#endif

#ifdef DBG_0 // Limit protons to specific beam, controlPt or spot
  if (rbControlPts[indexControlPt].indexBeam != 5)
  //if (rbControlPts[indexControlPt].indexBeam != 1 && rbControlPts[indexControlPt].indexBeam != 2 && rbControlPts[indexControlPt].indexBeam != 3 &&
  //  rbControlPts[indexControlPt].indexBeam != 5 && rbControlPts[indexControlPt].indexBeam != 4 || rbControlPts[indexControlPt].indexBeam != 0)
  //if (indexControlPt < 18)
  //if (indexSpot != 0)
  //if (indexControlPt != 0)
  //if(protonNum > 0)
  {
    return;
  }
#endif

  // Indicates proton trapped in device
  if (ubProtonsDir[protonNum][0] == FLAG_PROTON_STOP)
  {
    ubProtonsDir[protonNum][1] = rbIntegratedDepthDose[rbControlPts[indexControlPt].indexDepthDoses + rbControlPts[indexControlPt].numDepthDoses - 1];
    ubProtonsDir[protonNum][1] *= rbSpotScales[indexSpot].doseScale;

    ubProtonsAux[protonNum].scaledPathLength = rbIntegratedDepthDose[rbControlPts[indexControlPt].indexDepthDoses +
      rbControlPts[indexControlPt].numDepthDoses - 1] * rbSpotScales[indexSpot].doseScale;

    ubProtonsPos[protonNum][0] = flagQA;

    return;
  }

#ifdef DBG_0 // Enable to get ubProtons (linac coordinates, no rotations) from SpotPrep or ComputeDevices
  ubProtons[0].scaledPathLength = 418;
  return;
#endif

  if (flagQA == 1)
  {
    proton.pos[0] =  ubProtonsPos[protonNum][0] + orientation * (rbControlPts[indexControlPt].isocenterQA[0] - volOrigin0);
    proton.pos[1] = -ubProtonsPos[protonNum][2] + orientation * (rbControlPts[indexControlPt].isocenterQA[1] - volOrigin1);
    proton.pos[2] =  ubProtonsPos[protonNum][1] + abs(rbControlPts[indexControlPt].isocenterQA[2] - volOrigin2);

    proton.dir[0] =  ubProtonsDir[protonNum][0];
    proton.dir[1] = -ubProtonsDir[protonNum][2];
    proton.dir[2] =  ubProtonsDir[protonNum][1];
  }
  else if (flagHCL)
  {
    tmp = mul(rbControlPts[indexControlPt].beamMatrix, ubProtonsPos[protonNum]) + rbControlPts[indexControlPt].isocenter;

    proton.pos[0] = tmp[0] - volOrigin0;
    proton.pos[1] = tmp[1] - volOrigin1;
    proton.pos[2] = tmp[2] - volOrigin2;

    proton.dir = mul(rbControlPts[indexControlPt].beamMatrix, ubProtonsDir[protonNum]);
  }
  else
  {
    tmp = mul(rbControlPts[indexControlPt].beamMatrix, ubProtonsPos[protonNum]);

    proton.pos[0] =  tmp[0] + orientation * (rbControlPts[indexControlPt].isocenter[0] - volOrigin0);
    proton.pos[1] = -tmp[2] + orientation * (rbControlPts[indexControlPt].isocenter[1] - volOrigin1);
    proton.pos[2] =  tmp[1] + abs(rbControlPts[indexControlPt].isocenter[2] - volOrigin2);

    tmp = mul(rbControlPts[indexControlPt].beamMatrix, ubProtonsDir[protonNum]);

    proton.dir[0] =  tmp[0];
    proton.dir[1] = -tmp[2];
    proton.dir[2] =  tmp[1];
  }

#ifdef DBG_0 // Enable to get ubProtons (patient coordinates) before projecting to patient volume

  ubProtonsPos[protonNum]. = proton.pos;
  ubProtonsDir[protonNum]. = proton.dir;
  ubProtons[protonNum].scaledPathLength = 420;
 
  return;
#endif

  tLow = -100000;

  if (proton.pos[0] < 0.0 || proton.pos[0] > volSize0 ||
      proton.pos[1] < 0.0 || proton.pos[1] > volSize1 ||
      proton.pos[2] < 0.0 || proton.pos[2] > volSize2)
  {
    // Find intersection of proton path with patient volume

    if (proton.dir[0] != 0.0)
    {
      if (proton.dir[0] > 0.0)
      {
        tLow  = -proton.pos[0] / proton.dir[0];
        tHigh = (volSize0 - proton.pos[0]) / proton.dir[0];
      }
      else
      {
        tHigh = -proton.pos[0] / proton.dir[0];
        tLow  = (volSize0 - proton.pos[0]) / proton.dir[0];
      }
    }

    if(proton.dir[1] != 0.0)
    {
      if (proton.dir[1] > 0.0)
      {
        tLowNext  = -proton.pos[1] / proton.dir[1];
        tHighNext = (volSize1 - proton.pos[1]) / proton.dir[1];
      }
      else
      {
        tHighNext = -proton.pos[1] / proton.dir[1];
        tLowNext  = (volSize1 - proton.pos[1]) / proton.dir[1];
      }
    }

    if (tLowNext > tLow)
    {
      tLow = tLowNext;
    }

    if (tHighNext < tHigh)
    {
      tHigh = tHighNext;
    }

    if (proton.dir[2] != 0.0)
    {
      if (proton.dir[2] > 0.0)
      {
        tLowNext  = -proton.pos[2] / proton.dir[2];
        tHighNext = (volSize2 - proton.pos[2]) / proton.dir[2];
      }
      else
      {
        tHighNext = -proton.pos[2] / proton.dir[2];
        tLowNext  = (volSize2 - proton.pos[2]) / proton.dir[2];
      }
    }

    if (tLowNext > tLow)
    {
      tLow = tLowNext;
    }

    // Transport proton to volume surface
    proton.pos += tLow * proton.dir;

  }
  //else already inside image volume

#ifdef DBG_0 // Enable to get proton at patient surface (patient coordinates)
  
  ubProtonsPos[protonNum]. = proton.pos;
  ubProtonsDir[protonNum]. = proton.dir;
  ubProtons[protonNum].scaledPathLength = tLow;

  return;
#endif

  // Find voxel of proton entry point
  proton.vox[0] = (int)((double)proton.pos[0] / (double)voxSize0);
  proton.vox[1] = (int)((double)proton.pos[1] / (double)voxSize1);
  proton.vox[2] = (int)((double)proton.pos[2] / (double)voxSize2);

  if (proton.vox[0] == dims0) proton.vox[0] = dims0 - 1;
  if (proton.vox[1] == dims1) proton.vox[1] = dims1 - 1;
  if (proton.vox[2] == dims2) proton.vox[2] = dims2 - 1;


  proton.vox[3] = 0; // Texture mip level

  if (flagDij)
  {
    spotBinIndex = rbDijSpots[indexSpot].indexBin;
    dirAxis      = rbDijSpots[indexSpot].mainDirAxis;
    mainVoxInit  = proton.vox[dirAxis];
  }

  // Convert proton position to proton position within voxel
  float tmp3 = proton.pos[0];
  proton.pos[0] = proton.pos[0] - proton.vox[0] * voxSize0;
  proton.pos[1] = proton.pos[1] - proton.vox[1] * voxSize1;
  proton.pos[2] = proton.pos[2] - proton.vox[2] * voxSize2;

  initEnergy = rbIntegratedDepthDose[rbControlPts[indexControlPt].indexDepthDoses + rbControlPts[indexControlPt].numDepthDoses - 1];
  initEnergyMev = JOULE_TO_MEV * initEnergy;

  proton.inside = true;

/*_____________________________________________________________________________________________

  Calculate current energy of proton after passing through any devices
_______________________________________________________________________________________________*/

  proton.scaledPathLength = ubProtonsAux[protonNum].scaledPathLength;

  float preScatterVoxelPathLength;
  float postScatterVoxelPathLength;
  float voxelDeltaDose;
  float voxelDeltaEnergy;
  float energyLossCumulative;
  float haloScale;
  float alphaScale;
  float energyLossBare = CalcProtonEnergyLoss(proton.scaledPathLength, indexControlPt);

  if (flagSOBP)
  {
    haloScale = rbSOBPHalos[protonNum];
  }
  else
  {
    haloScale = rbSpots[indexSpot].spotHaloFactor;
  }

  energyLossBare = CalcProtonEnergyLoss(proton.scaledPathLength, indexControlPt);
  alphaScale = InterpolateAlpha(proton.scaledPathLength, indexControlPt);
  energyLossCumulative = alphaScale + (energyLossBare - alphaScale) * haloScale;

  
#ifdef DBG_0 // Enable to turn off halo, see also dose calc
  energyLossCumulative = energyLossBare;
#endif


  proton.energy = initEnergy - energyLossCumulative;
  ubProtonsAux[protonNum].scaledPathLength = energyLossCumulative * rbSpotScales[indexSpot].doseScale; // Storing for EnergyDevicesCS

  // Enable to get voxel trajectory
#ifdef DBG_0
  ubProtonsPos[protonNum]. = proton.pos;
  ubProtonsDir[protonNum]. = proton.dir;

  return;
#endif

/*_____________________________________________________________________________________________

      Start transport loop of proton through image volume
_______________________________________________________________________________________________*/
  do
  {

#ifdef DBG_0
    if (protonNum == 0)
    {
      ubProtons[loop].pos = proton.pos;
      ubProtons[loop].dir = proton.dir;
      ubProtons[loop].scaledPathLength = 415;
    }
#endif

    preScatterVoxelPathLength = voxSize0;

    // Calc chord length to voxel boundary 
    if (proton.dir[0] > 0.0)
    {
      preScatterVoxelPathLength = (voxSize0 - proton.pos[0]) / proton.dir[0];
      proton.voxelFace = 1;
    }
    else if(proton.dir[0] < 0.0)
    {
      preScatterVoxelPathLength = -proton.pos[0] / proton.dir[0];
      proton.voxelFace = 0;
    }

    if (preScatterVoxelPathLength <= 0.0)
    {
      preScatterVoxelPathLength = voxSize0;
    }

    if (proton.dir[1] > 0.0)
    {
      trialLength = (voxSize1 - proton.pos[1]) / proton.dir[1];

      if (trialLength < preScatterVoxelPathLength && trialLength > 0)
      {
        preScatterVoxelPathLength = trialLength;
        proton.voxelFace = 3;
      }
    }
    else if(proton.dir[1] < 0.0)
    {
      trialLength = -proton.pos[1] / proton.dir[1];

      if (trialLength < preScatterVoxelPathLength && trialLength > 0)
      {
        preScatterVoxelPathLength = trialLength;
        proton.voxelFace = 2;
      }
    }

    if (proton.dir[2] > 0.0)
    {
      trialLength = (voxSize2 - proton.pos[2]) / proton.dir[2];

      if (trialLength < preScatterVoxelPathLength && trialLength > 0)
      {
        preScatterVoxelPathLength = trialLength;
        proton.voxelFace = 5;
      }
    }
    else if(proton.dir[2] < 0.0)
    {
      trialLength = -proton.pos[2] / proton.dir[2];

      if (trialLength < preScatterVoxelPathLength && trialLength > 0)
      {
        preScatterVoxelPathLength = trialLength;
        proton.voxelFace = 4;
      }
    }

    rspVal = rbVoxelRSP.Load(proton.vox);

#ifdef DBG_0
    if (protonNum == 0)
    {
      ubProtons[loop].pos[0] = proton.vox[0];
      ubProtons[loop].pos[1] = proton.vox[1];
      ubProtons[loop].pos[2] = proton.vox[2];
      ubProtons[loop].dir[0] = rspVal;
      ubProtons[loop].dir[2] = 422;
    }
#endif

#ifdef DBG_0 // RSP 1 (dose cube), coordinate with energy shaders
    rspVal = 1.0;
#endif

    if (rspVal > RSP_BLANK)
    {

      preScatterVoxelPathLength /= 2;

      proton.pos[0] += proton.dir[0] * preScatterVoxelPathLength;
      proton.pos[1] += proton.dir[1] * preScatterVoxelPathLength;
      proton.pos[2] += proton.dir[2] * preScatterVoxelPathLength;

#ifdef DBG_0 // RSP 1 (patient) Coordinate with energy shaders
      rspVal = 1.0;
#endif

      // Take into account voxel density for energy loss and scattering
      energyMev = proton.energy * JOULE_TO_MEV;
      float energyRatioSqrd = energyMev / initEnergyMev;
      energyRatioSqrd = energyRatioSqrd * energyRatioSqrd;
      
      float arg0;
      float arg1;
      float arg2;
      float arg3;
      
      if (energyMev > 0.0 && energyRatioSqrd >= 0.03 && energyRatioSqrd <= 0.999)
      {

        arg0 = log10(1 - energyRatioSqrd);
        arg1 = log10(energyMev);
        arg2 = 0.5244 + 0.1975 * arg0 + arg1 * (0.2320 - 0.0098 * arg0);

        arg3 = 15 / energyMev;

        if (arg2 > 0.0)
        {
          scatterAngle = arg3 * sqrt(arg2 / 46.88 * rspVal * (preScatterVoxelPathLength / 5));
        }
        else
        {
          scatterAngle = 0.0;
        }
      }
      else
      {
        scatterAngle = 0.0;
      }
      
      initEnergyMev = energyMev;

#ifdef DBG_0 // Scatter angle 0.0
      scatterAngle = 0.0;
#endif

      float cos_sa = cos(scatterAngle);
      float sin_sa = sin(scatterAngle);

      // Apply deflection
      NEWRAND(azAngle);
      azAngle *= 6.2831531;

      float cos_ra = cos(azAngle);
      float sin_ra = sin(azAngle);

      float dir_xy = sqrt(proton.dir[0] * proton.dir[0] + proton.dir[1] * proton.dir[1]);

      float cos_phi = proton.dir[0] / dir_xy;
      float sin_phi = proton.dir[1] / dir_xy;

      float cos_theta = proton.dir[2];
      float sin_theta = dir_xy;

      float scatter_x = sin_sa * cos_ra;
      float scatter_y = sin_sa * sin_ra;

      proton.dir[0] =  cos_phi * cos_theta * scatter_x - sin_phi * scatter_y + cos_phi * sin_theta * cos_sa;
      proton.dir[1] =  sin_phi * cos_theta * scatter_x + cos_phi * scatter_y + sin_phi * sin_theta * cos_sa;
      proton.dir[2] = -sin_theta * scatter_x + cos_theta * cos_sa;

      // Find distance from scatter point to voxel boundary
      if (proton.dir[0] > 0.0)
      {
        postScatterVoxelPathLength = (voxSize0 - proton.pos[0]) / proton.dir[0];
        proton.voxelFace = 1; // +X
      }
      else
      {
        postScatterVoxelPathLength = -proton.pos[0] / proton.dir[0];
        proton.voxelFace = 0; // -X
      }

      if (postScatterVoxelPathLength < 0.0)
      {
        postScatterVoxelPathLength = voxSize0;
      }

      if (proton.dir[1] > 0.0)
      {
        trialLength = (voxSize1 - proton.pos[1]) / proton.dir[1];

        if (trialLength < postScatterVoxelPathLength && trialLength > 0)
        {
          postScatterVoxelPathLength = trialLength;
          proton.voxelFace = 3; // +Y
        }
      }
      else
      {
        trialLength = -proton.pos[1] / proton.dir[1];

        if (trialLength < postScatterVoxelPathLength && trialLength > 0)
        {
          postScatterVoxelPathLength = trialLength;
          proton.voxelFace = 2; // -Y
        }
      }

      if (proton.dir[2] > 0.0)
      {
        trialLength = (voxSize2 - proton.pos[2]) / proton.dir[2];

        if (trialLength < postScatterVoxelPathLength && trialLength > 0)
        {
          postScatterVoxelPathLength = trialLength;
          proton.voxelFace = 5; // +Z
        }
      }
      else
      {
        trialLength = -proton.pos[2] / proton.dir[2];

        if (trialLength < postScatterVoxelPathLength && trialLength > 0)
        {
          postScatterVoxelPathLength = trialLength;
          proton.voxelFace = 4; // -Z
        }
      }

/*_____________________________________________________________________________________________

      Calculate energy loss through voxel
_______________________________________________________________________________________________*/

      proton.scaledPathLength += (preScatterVoxelPathLength + postScatterVoxelPathLength) * rspVal;

      energyLossBare = CalcProtonEnergyLoss(proton.scaledPathLength, indexControlPt);

      alphaScale = InterpolateAlpha(proton.scaledPathLength, indexControlPt);
      energyLossCumulative = alphaScale + (energyLossBare - alphaScale) * haloScale;

      
#ifdef DBG_0 // Enable to turn off halo, see also init
      energyLossCumulative = energyLossBare; 
#endif

      voxelDeltaEnergy = proton.energy - (initEnergy - energyLossCumulative);

#ifdef DBG_0
      if (protonNum == 0)
      {
        ubProtons[loop].pos[0] = preScatterVoxelPathLength;
        ubProtons[loop].pos[1] = postScatterVoxelPathLength;
        ubProtons[loop].pos[2] = proton.scaledPathLength;
        ubProtons[loop].dir[0] = voxelDeltaEnergy;
        ubProtons[loop].dir[1] = energyLossBare;
        ubProtons[loop].dir[2] = energyLossCumulative;
        ubProtons[loop].scaledPathLength = 415;
      }
#endif

/*_____________________________________________________________________________________________

     Store deposited dose
_______________________________________________________________________________________________*/

      if (voxelDeltaEnergy > 0.0)
      {
        proton.energy -= voxelDeltaEnergy;

        if (flagQA)
        {
          voxelDeltaDose = rbSpotScales[indexSpot].doseScaleQA * voxelDeltaEnergy * DOSE_SCALE / (rspVal * voxelVolume);
        }
        else
        {
          voxelDeltaDose = rbSpotScales[indexSpot].doseScale * voxelDeltaEnergy * DOSE_SCALE / (rspVal * voxelVolume);
        }

#ifdef DBG_0
        voxelDeltaDose = 1.0;
#endif

        // Add voxelDeltaDose to voxel dose array with no thread collisions
        writeValue = asuint(voxelDeltaDose);
        checkValue = 0;
        [allow_uav_condition] while (true)
        {
          InterlockedCompareExchange(ubDoseVolume[proton.vox.xyz], checkValue, writeValue, readValue);
          if (readValue == checkValue)
          {
            break;
          }
          checkValue = readValue;
          writeValue = asuint(voxelDeltaDose + asfloat(readValue));
        }
        sumDose   += voxelDeltaDose;
        sumEnergy += voxelDeltaEnergy * rbSpotScales[indexSpot].doseScale;
        sumCount  += 1.0;

/*_____________________________________________________________________________________________

      Calculate LET info
 _______________________________________________________________________________________________*/

        if (flagLET)
        {
          float let;
          int baseIndex = -1;
          if (proton.scaledPathLength >= rbControlPts[indexControlPt].maxDepthLET)
          {
            let = 0.0;
          }
          else
          {

            float value = proton.scaledPathLength / rbControlPts[indexControlPt].depthSpacingLET;
            int index = (int)(value);
            baseIndex = rbControlPts[indexControlPt].indexLETs + index;

            if (index > rbControlPts[indexControlPt].numLETs - 2) // MLW dbg
            {
              let = rbLETs[rbControlPts[indexControlPt].indexLETs + rbControlPts[indexControlPt].numLETs - 1];
            }
            else
            {
              let = rbLETs[baseIndex] + (value - index) * (rbLETs[baseIndex + 1] - rbLETs[baseIndex]);
            }
          }

          writeValue = asuint(let);
          checkValue = 0;
          [allow_uav_condition] while (true)
          {
            InterlockedCompareExchange(ubLETVolume[proton.vox.xyz], checkValue, writeValue, readValue);
            if (readValue == checkValue)
            {
              break;
            }
            checkValue = readValue;
            writeValue = asuint(let + asfloat(readValue));
          }

          writeValue = asuint(1.0); // MLW todo: change to InterlockedAdd
          checkValue = 0;
          [allow_uav_condition] while (true)
          {
            InterlockedCompareExchange(ubProtonCount[proton.vox.xyz], checkValue, writeValue, readValue);
            if (readValue == checkValue)
            {
              break;
            }
            checkValue = readValue;
            writeValue = asuint(1.0 + asfloat(readValue));
          }

        } // flagLET

  /*_____________________________________________________________________________________________

        Calculate Dij Entries
  _______________________________________________________________________________________________*/

        if (flagDij)
        {

          binIndex = spotBinIndex + abs(proton.vox[dirAxis] - mainVoxInit);
          voxelKey = (proton.vox[0] << 20) + (proton.vox[1] << 10) + proton.vox[2] + 1;

          unsigned int voxelIndexBinStart = rbDijBins[binIndex].indexVoxel;
          unsigned int voxelIndexBinEnd = voxelIndexBinStart + rbDijBins[binIndex].numVoxels;

          unsigned int voxelIndex;
          unsigned int storedKey;
          for (voxelIndex = voxelIndexBinStart; voxelIndex < voxelIndexBinEnd; ++voxelIndex)
          {
            InterlockedCompareExchange(ubDijVoxel[voxelIndex].key, 0, voxelKey, storedKey);
            if (storedKey == 0 || storedKey == voxelKey)
            {
              writeValue = asuint(voxelDeltaDose);
              checkValue = 0;
              [allow_uav_condition] while (true)
              {
                InterlockedCompareExchange(ubDijVoxel[voxelIndex].dose, checkValue, writeValue, readValue);
                if (readValue == checkValue)
                {
                  break;
                }
                checkValue = readValue;
                writeValue = asuint(voxelDeltaDose + asfloat(readValue));
              }
#ifdef DBG_0
              sumVoxelDij += 1.0;
#endif
              sumDij += voxelDeltaDose;

              break;
            }
          }

        } // flagDij

      } // voxelDeltaEnergy

    } // rspVal
    else
    {
      postScatterVoxelPathLength = preScatterVoxelPathLength;
      voxelDeltaEnergy = 3.14159; // Set to non-zero so while loop continues (value not used)
    }

/*_____________________________________________________________________________________________

    Transport proton to next voxel, find coordinates
_______________________________________________________________________________________________*/

    proton.pos += proton.dir * postScatterVoxelPathLength;

    // Find next voxel
    switch (proton.voxelFace)
    {
    case 0:  // -X
      proton.vox[0] -= 1;
      if (proton.vox[0] == -1)
      {
        proton.inside = false;
      }
      else
      {
        proton.pos[0] = voxSize0;
      }
      break;

    case 1:  // +X
      proton.vox[0] += 1;
      if (proton.vox[0] == dims0)
      {
        proton.inside = false;
      }
      else
      {
        proton.pos[0] = 0;
      }
      break;

    case 2:  // -Y
      proton.vox[1] -= 1;
      if (proton.vox[1] == 0)
      {
        proton.inside = false;
      }
      else
      {
        proton.pos[1] = voxSize1;
      }
      break;

    case 3:  // +Y
      proton.vox[1] += 1;
      if (proton.vox[1] == dims1)
      {
        proton.inside = false;
      }
      else
      {
        proton.pos[1] = 0;
      }
      break;

    case 4:  // -Z
      proton.vox[2] -= 1;
      if (proton.vox[2] == -1)
      {
        proton.inside = false;
      }
      else
      {
        proton.pos[2] = voxSize2;
      }
      break;

    case 5:  // +Z
      proton.vox[2] += 1;
      if (proton.vox[2] == dims2)
      {
        proton.inside = false;
      }
      else
      {
        proton.pos[2] = 0;
      }
      break;
    }

    ++loop;
  } 
  while (proton.inside && (voxelDeltaEnergy > 0.0) && (loop < 1000));

  // Set proton pos to final pos
  ubProtonsPos[protonNum] = proton.pos;

  return;
}