
#define  _SHADER_

#include "ShaderRandom.h"
#include "ShaderPlan.h"

#define DBG_1                (1)

#define LOCATION_BLOCK       (0)
#define LOCATION_OUT_Z       (1)
#define LOCATION_OUT_LATERAL (2)
#define LOCATION_APERTURE    (3)
#define RAND_SEED            (547)

static float scatterThetaTable[21] =
{
  0.0000,
  0.0109,
  0.0245,
  0.0396,
  0.0563,
  0.0746,
  0.0946,
  0.1165,
  0.1406,
  0.1672,
  0.1968,
  0.2300,
  0.2675,
  0.3107,
  0.3614,
  0.4222,
  0.4980,
  0.5976,
  0.7421,
  0.9954,
  1.0000,
};

static float r_step = 1.0;

// Constant Buffers
cbuffer DispatchBuffer : register(b0)
{
  unsigned int dispatchIndex;
  unsigned int numProtons;

  unsigned int pad0[6];
};

// Resource Buffers
StructuredBuffer<SShaderBeam>         rbBeams          : register(t0);
StructuredBuffer<SShaderControlPt>    rbControlPts     : register(t1);
StructuredBuffer<SShaderSpot>         rbSpots          : register(t2);
StructuredBuffer<SDeviceMaterial>     rbMaterials      : register(t3);
StructuredBuffer<SShaderDevice>       rbDevices        : register(t4);
Buffer<int>                           rbDeviceIndexes  : register(t5);
StructuredBuffer<SShaderAperture>     rbApertures      : register(t6);
StructuredBuffer<float2>              rbAperturePts    : register(t7);
StructuredBuffer<SShaderCompensator>  rbCompensators   : register(t8);
Buffer<float>                         rbCompDepths     : register(t9);

// Unordered access buffer
RWStructuredBuffer<float3>            ubProtonsPos  : register(u0);
RWStructuredBuffer<float3>            ubProtonsDir  : register(u1);
RWStructuredBuffer<SShaderProtonsAux> ubProtonsAux  : register(u2);


bool CheckInsideAperture(float3 pos, int indexAperture)
{
  float slope0;
  float yCross;
  unsigned int numCrossings = 0;

  int vertEnd = rbApertures[indexAperture].indexAperturePts + rbApertures[indexAperture].numAperturePts;

  // Find intersection of vertical line from current position through all perimeter segments
  for (int vertex = rbApertures[indexAperture].indexAperturePts; vertex < vertEnd; ++vertex)
  {
    if ((pos.x < rbAperturePts[vertex].x) ^ (pos.x < rbAperturePts[vertex + 1].x))
    {

      if (rbAperturePts[vertex + 1].x != rbAperturePts[vertex].x)
      {
        // Calc where vertical line intersects perimeter segment
        slope0 = (rbAperturePts[vertex + 1].y - rbAperturePts[vertex].y) / (rbAperturePts[vertex + 1].x - rbAperturePts[vertex].x);
        yCross = slope0 * (pos.x - rbAperturePts[vertex].x) + rbAperturePts[vertex].y;

        if (yCross > pos.y)
        {
          ++numCrossings;
        }
      }
    }
  }

  bool status = numCrossings % 2;

  return status;
}

/*_____________________________________________________________________________________________

  Transport proton through single device
_______________________________________________________________________________________________*/

SShaderProton TraverseDevice(int indexDevice, SShaderProton proton, float range, unsigned int gmcRand)
{
  float3 newPos;
  int   currentLocation;
  int   newLocation;
  float step;
  float scaledStep;
  float cos_sa;
  float sin_sa;
  float dir_xy;
  float cos_phi;
  float sin_phi;
  float cos_theta;
  float sin_theta;
  float randAngle;
  float cos_ra;
  float sin_ra;
  float scatter_x;
  float scatter_y;
  float norm;
  float r1;
  float scatterAngle;
  float scatterTableVal;
  float scatterScale;
  float scatterDelta;
  float scatterThetaMax;
  float scatterThetaRatio;
  int   scatterStep;
  int   mtrlIndex;
  int   mtrlIndexNext;
  bool  runStatus;
  uint  numCrossings;
  float slope0;
  float slope1;
  float yCross;
  bool  statusAperture;

  // If not a compensator, transport proton from current position to top of device
  if (rbDevices[indexDevice].numCompensators == 0)
  {
    step = (rbDevices[indexDevice].zTop - proton.pos.z) / proton.dir.z;
    proton.pos.x += (step * proton.dir.x);
    proton.pos.y += (step * proton.dir.y);
    proton.pos.z = rbDevices[indexDevice].zTop;
  }

  int indexAperture;
  int indexStart = rbDevices[indexDevice].indexApertures;
  int indexEnd   = indexStart + rbDevices[indexDevice].numApertures;

  currentLocation = LOCATION_BLOCK;
  mtrlIndex = rbDevices[indexDevice].indexMtrl;

  // Check if initial position within aperture
  for (indexAperture = indexStart; indexAperture < indexEnd; ++indexAperture)
  {
    // Find if proton inside aperture
    if (proton.pos.x > rbApertures[indexAperture].xMin && proton.pos.x < rbApertures[indexAperture].xMax &&
      proton.pos.y > rbApertures[indexAperture].yMin && proton.pos.y < rbApertures[indexAperture].yMax)
    {
      statusAperture = CheckInsideAperture(proton.pos, indexAperture);

      if (statusAperture)
      {
        currentLocation = LOCATION_APERTURE;
        mtrlIndex = rbApertures[indexAperture].indexMtrl;
        break;
      }
    }
  }

  runStatus = true;

  while (runStatus)
  {
    step   = r_step;
    newPos = proton.pos + step * proton.dir;

    // Proton exits bottom of block
    if (newPos.z < rbDevices[indexDevice].zBottom)
    {

      // Move proton back to block bottom
      step = (rbDevices[indexDevice].zBottom - proton.pos.z) / proton.dir.z;
      newPos = proton.pos + step * proton.dir;

      runStatus = false;
    }

    mtrlIndexNext = rbDevices[indexDevice].indexMtrl;

    if (rbDevices[indexDevice].numApertures)
    {

      for (indexAperture = indexStart; indexAperture < indexEnd; ++indexAperture)
      {

        //// Find if proton inside aperture
        if (newPos.x > rbApertures[indexAperture].xMin && newPos.x < rbApertures[indexAperture].xMax &&
          newPos.y > rbApertures[indexAperture].yMin && newPos.y < rbApertures[indexAperture].yMax)
        {

          bool status = CheckInsideAperture(newPos, indexAperture);

          if (status)
          {
            newLocation = LOCATION_APERTURE;
            mtrlIndexNext = rbApertures[indexAperture].indexMtrl;
          }

          if (newLocation != currentLocation)
          {
            slope1 = (newPos.y - proton.pos.y) / (newPos.x - proton.pos.x);

            int vertEnd = rbApertures[indexAperture].indexAperturePts + rbApertures[indexAperture].numAperturePts;

            for (int vertex = rbApertures[indexAperture].indexAperturePts; vertex < vertEnd; ++vertex)
            {
              if (rbAperturePts[vertex + 1].x - rbAperturePts[vertex].x)
              {
                newPos.x = rbAperturePts[vertex].x;
              }
              else
              {
                slope0 = (rbAperturePts[vertex + 1].y - rbAperturePts[vertex].y) / (rbAperturePts[vertex + 1].x - rbAperturePts[vertex].x);

                newPos.x = (rbAperturePts[vertex].y - proton.pos.y + slope1 * rbAperturePts[vertex].x - slope0 * proton.pos.x) / (slope1 - slope0);
              }

              if ((newPos.x < rbAperturePts[vertex].x) ^ (newPos.x < rbAperturePts[vertex + 1].x))
              {
                newPos.y = proton.pos.y + slope1 * (newPos.x - proton.pos.x);
                if ((newPos.y < rbAperturePts[vertex].y) ^ (newPos.y < rbAperturePts[vertex + 1].y))
                {
                  newPos.z = (newPos.x / proton.dir.x) * proton.dir.z;

                  float3 deltaPos = newPos - proton.pos;
                  step = sqrt(deltaPos.x * deltaPos.x + deltaPos.y * deltaPos.y + deltaPos.z * deltaPos.z);
                }
              }
            } // Aperture vertices loop

          } // Location change check

          if (status)
          {
            break;
          }

        } // Aperture bounds check

      }

    } // Aperture check

    scaledStep = step * rbMaterials[mtrlIndex].scaledDensity;
    range -= scaledStep;

    // Proton runs out of energy inside block
    if (range < 0)
    {
      runStatus = false;
      proton.dir.z = FLAG_PROTON_STOP; // Used to indicate stopped in device
    }

    if (runStatus)
    {

      // *** Start CalcScatterAngle()

      scatterScale = range / 10.0;
      scatterStep = (int)scatterScale;
      scatterDelta = scatterScale - scatterStep;
      scatterStep -= 2;   // Account for wer table not starting at 0
      if (scatterStep < 0)
      {
        scatterStep = 0;
      }

      scatterTableVal = rbMaterials[mtrlIndex].wer_msc_table[scatterStep];
      scatterThetaMax = scatterTableVal + scatterDelta * (rbMaterials[mtrlIndex].wer_msc_table[scatterStep + 1] - scatterTableVal);

      scatterScale = scaledStep / range;
      if (scatterScale > 1.0)
      {
        scatterScale = 1.0;
      }
      scatterScale *= 20;
      scatterStep = (int)scatterScale;
      scatterDelta = scatterScale - scatterStep;

      scatterTableVal = scatterThetaTable[scatterStep];
      scatterThetaRatio = scatterTableVal + scatterDelta * (scatterThetaTable[scatterStep + 1] - scatterTableVal);

      scatterAngle = sqrt(scatterThetaRatio * scatterThetaMax);

      // *** End CalcScattterangle;

#ifdef DBG_0 // Scatter angle 0.0
      scatterAngle = 0.0;
#endif

#ifdef DBG_0
      scatterAngle *= 2.0;
#endif

      cos_sa = cos(scatterAngle);
      sin_sa = sin(scatterAngle);

      NEWRAND(r1);
      randAngle = r1 * (2 * 3.14159625);

      cos_ra = cos(randAngle);
      sin_ra = sin(randAngle);

      dir_xy = sqrt(proton.dir.x * proton.dir.x + proton.dir.y * proton.dir.y);

      if (dir_xy == 0.0)
      {
        cos_phi = 1.0;
        sin_phi = 0.0;
      }
      else
      {
        cos_phi = proton.dir.x / dir_xy;
        sin_phi = proton.dir.y / dir_xy;
      }

      cos_theta = proton.dir.z;
      sin_theta = dir_xy;

      scatter_x = sin_sa * cos_ra;
      scatter_y = sin_sa * sin_ra;

      proton.dir[0] = cos_phi * cos_theta * scatter_x - sin_phi * scatter_y + cos_phi * sin_theta * cos_sa;
      proton.dir[1] = sin_phi * cos_theta * scatter_x + cos_phi * scatter_y + sin_phi * sin_theta * cos_sa;
      proton.dir[2] = -sin_theta * scatter_x + cos_theta * cos_sa;

    }

    proton.scaledPathLength += scaledStep;
    proton.pos = newPos;

    currentLocation = newLocation;
    mtrlIndex = mtrlIndexNext;

  }

  return proton;
}

SShaderProton CalcCompensatorIntersection(int indexDevice, int indexComp, SShaderProton proton)
{
  float3 startPos;
  int2   gridLocation;
  float stepTop;
  float stepBottom;
  float stepMid;
  int   indexPixel;
  int   indexPixelDepths;
  int   indexDepth;
  int   indexPrevDepth = -1;

  indexPixelDepths = rbCompensators[indexComp].indexPixelDepths;
  indexPrevDepth = -1;

  stepTop = (rbDevices[indexDevice].zTop - proton.pos.z) / proton.dir.z;
  proton.pos.x += (stepTop * proton.dir.x);
  proton.pos.y += (stepTop * proton.dir.y);
  proton.pos.z = rbDevices[indexDevice].zTop;
  startPos = proton.pos;

  stepBottom = (rbDevices[indexDevice].zBottom - proton.pos.z) / proton.dir.z;
  stepTop = 0.0;

  float depth;

  do
  {
    stepMid = (stepTop + stepBottom) / 2.0;

    if (proton.pos.x < rbCompensators[indexComp].xMin || proton.pos.x > rbCompensators[indexComp].xMax ||
        proton.pos.y < rbCompensators[indexComp].yMin || proton.pos.y > rbCompensators[indexComp].yMax)
    {
#ifdef DBG_0 // DBG compensator
      proton.pos.z = rbDevices[indexDevice].zBottom;
#endif
      proton.pos.z = FLAG_PROTON_STOP;

      break;
    }
    //else // DBG_0
    //{
    //  proton.pos.z = rbDevices[indexDevice].zBottom;
    //  break;
    //}

    gridLocation[0] = ((proton.pos.x - rbCompensators[indexComp].xMin) / rbCompensators[indexComp].pixSizeX) + 0.5;
    gridLocation[1] = ((rbCompensators[indexComp].yMax - proton.pos.y) / rbCompensators[indexComp].pixSizeY) + 0.5;

    indexDepth = indexPixelDepths + (gridLocation[1] * rbCompensators[indexComp].numColumns + gridLocation[0]);

    // DBG_0
    depth = rbCompDepths[indexDepth];
    //depth = 50.0;
    //if (gridLocation[0] > 40)
    //{
    //  depth = 50.0;
    //}
    //else
    //{
    //  depth = 0.0;
    //}

    if (indexDepth == indexPrevDepth)
    {
      stepMid = ((rbDevices[indexDevice].zBottom + depth) - startPos.z) / proton.dir.z;
      proton.pos = startPos + stepMid * proton.dir;
      // DBG_0
      //proton.scaledPathLength = rbCompDepths[indexDepth];
      //proton.dir[0] = gridLocation[0];
      //proton.dir[1] = gridLocation[1];
      //proton.dir[2] = indexDepth;

      break;
    }
    else
    {
      if (proton.pos.z > (rbDevices[indexDevice].zBottom + depth) )
      {
        stepTop = stepMid;

        proton.pos = startPos + stepTop * proton.dir;

      }
      else
      {
        stepBottom = stepMid;
      }

      indexPrevDepth = indexDepth;
    }

  } while (true);

  return proton;
}

/*_____________________________________________________________________________________________

  DevicesCS - Compute Shader, Transport protons through all beam, control pt devices
_______________________________________________________________________________________________*/

[numthreads(1024, 1, 1)]
void TransportDevicesCS( uint3 thread : SV_DispatchThreadID )
{
  SShaderProton proton;
  float range;

  unsigned int protonNum = thread[0] + dispatchIndex;

  INITRAND(RAND_SEED + thread[0]);

  if (protonNum >= numProtons)
  {
    return; 
  }

  int indexControlPt = rbSpots[ubProtonsAux[protonNum].indexSpot].indexControlPt;

#ifdef DBG_0  // Limit protons to specific beam, controlPt or spot
  if (rbControlPts[indexControlPt].indexBeam != 5)
  //if (rbControlPts[indexControlPt].indexBeam != 1 && rbControlPts[indexControlPt].indexBeam != 2 && rbControlPts[indexControlPt].indexBeam != 3 &&
  //  rbControlPts[indexControlPt].indexBeam != 5 && rbControlPts[indexControlPt].indexBeam != 4 && rbControlPts[indexControlPt].indexBeam != 0)
  //if(indexControlPt != 0)
  //if(ubProtons[protonNum].indexSpot != 0)
  //if(protonNum > 0)
  //if (indexControlPt != 0)
  {
    return;
  }
#endif

  proton.pos = ubProtonsPos[protonNum];
  proton.dir = ubProtonsDir[protonNum];
  proton.indexSpot = ubProtonsAux[protonNum].indexSpot;
  proton.scaledPathLength = ubProtonsAux[protonNum].scaledPathLength;

  if (rbBeams[rbControlPts[indexControlPt].indexBeam].numDevices) // old json format
  {
    int deviceStart = rbBeams[rbControlPts[indexControlPt].indexBeam].indexDevices;
    int deviceEnd = deviceStart + rbBeams[rbControlPts[indexControlPt].indexBeam].numDevices;

#ifdef DBG_0 // Limit devices old format
    //deviceStart 1;
    deviceEnd = deviceStart + 1;
    deviceStart = 0;
    deviceEnd = -1;
#endif

    for (int indexDevice = deviceStart; indexDevice < deviceEnd; ++indexDevice)
    {
      range = rbControlPts[indexControlPt].r80 - proton.scaledPathLength;

      proton = TraverseDevice(indexDevice, proton, range, gmcRand);

      if (proton.dir.z == FLAG_PROTON_STOP)
      {
        proton.scaledPathLength = rbControlPts[indexControlPt].maxDepthDoseDepth;
        break;
      }
    }
  } 
  else // new json format
  {
    int indexDevice;

    int indexStart = rbControlPts[indexControlPt].indexDeviceIndexes;
    int indexEnd = indexStart + rbControlPts[indexControlPt].numDeviceIndexes;

#ifdef DBG_0 // Limit devices new format
    //indexStart = indexStart;
    indexEnd = indexStart + 1;
#endif

    //if (protonNum == 0) // DBG_0
    //{
    //  ubProtons[100].pos[0] = indexStart;
    //  ubProtons[100].pos[1] = indexEnd;
    //}

    for (int index = indexStart; index < indexEnd; ++index)
    {
      range = rbControlPts[indexControlPt].r80 - proton.scaledPathLength;

      indexDevice = rbDeviceIndexes[index];

      if (rbDevices[indexDevice].numCompensators) // Presently assumes one compensator
      {

        proton = CalcCompensatorIntersection(indexDevice, rbDevices[indexDevice].indexCompensators, proton);

#ifdef DBG_0 // DBG compensator
        //ubProtons[protonNum].pos.x = proton.pos.x;
        //ubProtons[protonNum].pos.y = proton.pos.y;
        //ubProtons[protonNum].dir = proton.dir;
        ubProtons[protonNum] = proton;
        ubProtons[protonNum].dir.z = proton.pos.z - rbDevices[indexDevice].zBottom;
        ubProtons[protonNum].dir.z = range;
        return;
#endif
      }

      proton = TraverseDevice(indexDevice, proton, range, gmcRand);

      if (proton.dir.z == FLAG_PROTON_STOP)
      {
        proton.scaledPathLength = rbControlPts[indexControlPt].maxDepthDoseDepth;
        break;
      }
    }

  }

  ubProtonsPos[protonNum] = proton.pos;
  ubProtonsDir[protonNum] = proton.dir;
  ubProtonsAux[protonNum].scaledPathLength = proton.scaledPathLength;

  return;
}

