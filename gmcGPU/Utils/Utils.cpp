#include "stdafx.h"
#include "AppConfigInstance.h"
#include "UniArray.h"
#include "IonPlan.h"
#include "Utils.h"


CUtils::CUtils(void)
{
  return;
}

CUtils::~CUtils(void)
{
  return;
}

void CUtils::ExportDepthInfo(CIonPlan* plan, int beamIndex, int controlPtIndex)
{
  SShaderBeam* beam = plan->m_arrayBeams(beamIndex);

  SShaderControlPt* cntrlPt = plan->m_arrayControlPts(controlPtIndex);

  float depth;
  float dose;
  float let;
  float value;
  float doseLET;

  CString fileName;
  fileName.Format("%s\\Data\\LET.txt", g_configInstance->m_directoryBase);
  FILE* fd = fopen(fileName, "w");

  for (int index = 0; index < cntrlPt->numDepthDoses - 1; ++index)
  {
    depth = index * cntrlPt->depthSpacingDepthDose;

    dose = plan->m_arrayDepthDoses[index];

    value = depth / cntrlPt->depthSpacingLET;
    int indexLETs = (int)(value);

    if (indexLETs < cntrlPt->numLETs -1)
    {// MLW ionplan2
      //let = cntrlPt->lets[indexLETs] + (value - indexLETs) * (cntrlPt->lets[indexLETs + 1] - cntrlPt->lets[indexLETs]);
    }
    else
    {
      let = 0.0;
    }
    
    doseLET = dose * let;

    fprintf(fd, "%f %f %f %f %f\n", depth, dose, let, doseLET, (dose / 1.1) * (1 + 0.03 * let));
  }
  fclose(fd);

  return;
}

//float CUtils::Interpolate(float* data, float* sample, )
//{
//  float value;
//
//
//  return value;
//}



#define NUM_BINS (50) // MLW dbg
void CUtils::SpreadProtons(int numProtons, SPt2* spotSize)
{
  float inverse = 1.0F / RAND_MAX;
  SPt2* rands = new SPt2[numProtons];
  int bins[100][100];
  memset(bins[0], 0, 100 * 100 * sizeof(int));

  float x, y;
  int xBin, yBin;
  int xmin = 200, xmax = 0 , ymin = 200, ymax = 0;
  for (SPt2* randVal = rands; randVal < rands + numProtons; ++randVal)
  {
    randVal->x = rand() * inverse;
    randVal->y = rand() * inverse;

    x = (-1.5F + 3 * randVal->x) * spotSize->x;
    y = (-1.5F + 3 * randVal->y) * spotSize->y;


    xBin = (x + 100.0F) / 2.0F;
    yBin = (y + 100.0F) / 2.0F;

    if (xBin < xmin)
    {
      xmin = xBin;
    }
    else if(xBin > xmax)
    {
      xmax = xBin;
    }

    if (yBin < ymin)
    {
      ymin = yBin;
    }
    else if(yBin > ymax)
    {
      ymax = yBin;
    }

    ++bins[xBin][yBin];

  }

  float minCount = numProtons, maxCount = bins[0][0];
  int numFills = 0;
  int numTotal = 0;
  for (int* bin = &bins[0][0]; bin < &bins[0][0] + 10000; ++bin)
  {
    if (*bin)
    {
      if (*bin > maxCount)
      {
        maxCount = *bin;
      }
      else if (*bin < minCount)
      {
        minCount = *bin;
      }
      numTotal += *bin;
      ++numFills;
    }
  }

  float avrg = ((float)numTotal) / numFills;
  float spread = 0;

  int countBins[NUM_BINS];
  int binNum;
  memset(countBins, 0, NUM_BINS * sizeof(int));

  for (int* bin = &bins[0][0]; bin < &bins[0][0] + 10000; ++bin)
  {
    if (*bin)
    {
      spread = +(*bin - avrg) * (*bin - avrg);

      binNum = (int)((*bin / maxCount) * NUM_BINS);
      ++countBins[binNum];
    }
  }

  float stdDev = sqrt(spread / numFills);

  FILE* fd;
  CString fileName;
  fileName.Format("%s\\Data\\SpreadHistory.txt", g_configInstance->m_directoryBase);
  fd = fopen(fileName, "w");

  for (int bin = 0; bin < NUM_BINS; ++bin)
  {
    fprintf(fd, "%f %e\n", bin * (maxCount / NUM_BINS), (float)countBins[bin]);
  }
  fclose(fd);

  //m_randBuff.CreateStructuredBufferFixed(m_numProtonsPerDispatch, sizeof(SPt2), rands);

  delete[] rands;

}

void CUtils::CalcScatterAngle(float* depthDose, int numDose)
{

  float rspVal = 1.0;
  float preScatterVoxelPathLength = 0.5;

  float energy;
  float energyRatioSqrd;
  float scatterAngle;
  float initEnergy = depthDose[numDose - 1];

  FILE* fd;
  CString fileName;
  fileName.Format("%s\\Data\\ScatterAngle.txt", g_configInstance->m_directoryBase);
  fd = fopen(fileName, "w");

  for (int index = 0; index < numDose; ++index)
  {
    energy = depthDose[index];

    energyRatioSqrd = energy / initEnergy;
    energyRatioSqrd = energyRatioSqrd * energyRatioSqrd;

    if (energy > 0.0 && energyRatioSqrd >= 0.03 && energyRatioSqrd <= 0.999)
    {

      float arg0 = log10(1 - energyRatioSqrd);
      float arg1 = log10(energy);
      float arg2 = 0.5244 + 0.1975 * arg0 + arg1 * (0.2320 - 0.0098 * arg0);

      arg0 = 15 / energy;

      scatterAngle = arg0 * sqrt(arg2 / 46.88 * rspVal * (preScatterVoxelPathLength / 5));

    }
    else
    {
      scatterAngle = 0.0;
    }

    fprintf(fd, "%d %f\n", index, scatterAngle);

  }

  return;
}

void CUtils::ArrayTest(void)
{
  CUniArray<int> array(5, 1);
  CUniArray<int>* parray = &array;

  // numItems
  forPrrayN(parray, index, 5) // Pass
  {
    TRACE("index %d\n", *index);
  }

  forPrrayN(parray, index, 6) // Fail
  {
    TRACE("index %d\n", *index);
  }

  forArrayN(array, index, 5) // Pass
  {
    TRACE("index %d\n", *index);
  }

  forArrayN(array, index, 6) // Fail
  {
    TRACE("index %d\n", *index);
  }

  // Start index + numItems
  forPrrayIN(parray, index, 1, 2) // Pass
  {
    TRACE("index %d\n", *index);
  }

  forPrrayIN(parray, index, 1, 5) // Fail
  {
    TRACE("index %d\n", *index);
  }

  forArrayIN(array, index, 1, 2) // Pass
  {
    TRACE("index %d\n", *index);
  }

  forArrayIN(array, index, 1, 5) // Fail
  {
    TRACE("index %d\n", *index);
  }

  // start & end indexes
  forPrrayI2(parray, index, 1, 4) // Pass
  {
    TRACE("index %d\n", *index);
  }

  forPrrayI2(parray, index, -1, 4) // Fail
  {
    TRACE("index %d\n", *index);
  }

  forPrrayI2(parray, index, 1, 6) // Fail
  {
    TRACE("index %d\n", *index);
  }

  forArrayI2(array, index, 1, 4) // Pass
  {
    TRACE("index %d\n", *index);
  }

  forArrayI2(array, index, -1, 4) // Fail
  {
    TRACE("index %d\n", *index);
  }

  forArrayI2(array, index, 1, 6) // Fail
  {
    TRACE("index %d\n", *index);
  }

  return;
}

//template <typename T> void CUtils::FindBounds(ID3D11Texture3D* inputTexture, ID3D11Texture3D* readTexture)
//{
//
//  D3D11_TEXTURE3D_DESC desc;
//  inputTexture->GetDesc(&desc);
//
//  D3D11_MAPPED_SUBRESOURCE mapResource;
//
//  m_dxContext->CopyResource(m_textureRead, inputTexture); // MLW dbg
//  m_dxContext->Map(m_textureRead, 0, D3D11_MAP_READ, 0, &mapResource);
//
//  T* xEnd;
//  T* yEnd;
//  T* xPtr;
//  T* yPtr;
//  T* zPtr;
//  T* zStart = (T*)mapResource.pData;
//  T* zEnd   = zStart + desc.Depth * (mapResource.DepthPitch / sizeof(T));
//  int zStep = mapResource.DepthPitch / sizeof(T);
//  int yStep = mapResource.RowPitch / sizeof(T);
//  T max = *zStart;
//  T min = *zStart;
//  T doseSum = 0.0;
//
//  for (zPtr = zStart; zPtr < zEnd; zPtr += zStep)
//  {
//    yEnd = zPtr + desc.Height * yStep;
//
//    for (yPtr = zPtr; yPtr < yEnd; yPtr += yStep)
//    {
//      xEnd = yPtr + desc.Width;
//
//
//      for (xPtr = yPtr; xPtr < xEnd; ++xPtr)
//      {
//
//        doseSum += *xPtr;
//
//        if (*xPtr < min)
//        {
//          min = *xPtr;
//        }
//
//        if (*xPtr > max)
//        {
//          max = *xPtr;
//        }
//      }
//    }
//  }
//
//  m_dxContext->Unmap(m_textureReadResults.m_textureRead, 0);
//
//  TRACE("DoseCompute %f %f\n", min, max);
//
//  return;
//}