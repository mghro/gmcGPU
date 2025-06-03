#include "stdafx.h"
#include "AppConfigInstance.h"
#include "AuxDosesCS.hxx"
#include "DoseSumCS.hxx"
#include "TextureMaxCS.hxx"
#include "ArrayMaxCS.hxx"
#include "EnergyCS.hxx"
#include "EnergyDevicesCS.hxx"
#include "EnergyPatientFreeCS.hxx"
#include "SliceHedra.h"
#include "DirectX.h"
#include "FileIO.h"
#include "ShaderResample.h"
#include "SliceHedra.h"
#include "DicomStruct.h"
#include "DicomDose.h"
#include "GammaIO.h"
#include "Transport.h"
#include "IonPlan.h"
#include "EnginePostCalc.h"

#define GY_2_CGY            (100)
#define SWAP_ENDIAN(value)  ((((value) & 0xFF000000) >> 24) | (((value) & 0x00FF0000) >> 8) | (((value) & 0x0000FF00) << 8) | ((value) << 24))

struct SDispatchBuffAuxDose
{
  unsigned int dispatchIndex;
  unsigned int numVoxels;
  unsigned int numFractions;

  float rbe0;
  float rbe1;
  float rbe2;

  int pad[2];
};

CEnginePostCalc::CEnginePostCalc(void)
{

  return;
}

CEnginePostCalc::~CEnginePostCalc(void)
{

  return;
}

bool CEnginePostCalc::CreateShaders(void)
{

  m_boundsCalcTxtrCS.BuildShader(g_textureMaxCS, sizeof(g_textureMaxCS));

  m_arrayMaxCS.BuildShader(g_arrayMaxCS, sizeof(g_arrayMaxCS));

  m_energyCS.BuildShader(g_energyCS, sizeof(g_energyCS));
  m_energyDevicesCS.BuildShader(g_energyDevicesCS, sizeof(g_energyDevicesCS));
  m_energyPatientFreeCS.BuildShader(g_energyPatientFreeCS, sizeof(g_energyPatientFreeCS));

  return true;
}

bool CEnginePostCalc::CreatePersistentResources(void)
{
  m_bufferDispatch.CreateConstantBufferDyn(sizeof(SDispatchBuffAuxDose));

  // Multi-purpose buffers
  m_bufferThreads.CreateBuffRWStruct(1024, 2*sizeof(double));
  m_bufferThreads.CreateBufferRead();



  return true;
}

void CEnginePostCalc::ClearResources(void)
{

  forPrray(m_txtrGroupDosesExternal, txtr)
  {
    txtr->Release();
  }
  m_txtrGroupDosesExternal->Clear();

  return;
}

void CEnginePostCalc::LoadTransportInfo(void)
{

  ClearResources();

  if (m_txtrGroupDosesQA->m_numItems)
  {
    forPrray(m_txtrGroupDosesQA, txtrDose)
    {
      CalcTextureBounds(txtrDose);
    }
  }

  if(m_txtrGroupDosesPatient->m_numItems)
  {
    CalcTextureBounds(m_txtrGroupDosesPatient->ItemPtr(DOSE_TYPE_DOSE));
  }

  return;
}

void CEnginePostCalc::CalcAuxFields(int numFractions)
{

  CDxShaderCompute shaderComputeBioDose;

  SVoxGridGeometry* doseGrid = &m_txtrGroupDosesPatient->m_volumeGrid;

  ID3D11ShaderResourceView* nullResView = NULL;
  m_dxContext->PSSetShaderResources(2, 1, &nullResView); // MLW todo: make engine method

  shaderComputeBioDose.BuildShader(g_auxDosesCS, sizeof(g_auxDosesCS));

  forPrrayIN(m_txtrGroupDosesPatient, txtr, DOSE_TYPE_BIODOSE, 5)
  {
    txtr->CreateRWTextureAll3D(doseGrid->dims, DXGI_FORMAT_R32_FLOAT);
  }

  ID3D11Buffer* buffers[] = { m_bufferDispatch.m_buffer };
  ID3D11ShaderResourceView* views[] =
  {
    m_txtrGroupDosesPatient->GetTextureView(DOSE_TYPE_DOSE),
    m_txtrNumProtonsVox->m_view
  };

  ID3D11UnorderedAccessView* uavViews[] =
  {
    m_txtrGroupDosesPatient->GetTextureViewRW(DOSE_TYPE_LET),
    m_txtrGroupDosesPatient->GetTextureViewRW(DOSE_TYPE_BIODOSE),
    m_txtrGroupDosesPatient->GetTextureViewRW(DOSE_TYPE_DIVLET),
    m_txtrGroupDosesPatient->GetTextureViewRW(DOSE_TYPE_RBE0),
    m_txtrGroupDosesPatient->GetTextureViewRW(DOSE_TYPE_RBE1),
    m_txtrGroupDosesPatient->GetTextureViewRW(DOSE_TYPE_RBE2)
  };

  shaderComputeBioDose.SetConstantBuffers(buffers, sizeof(buffers));
  shaderComputeBioDose.SetResourceViews(views, sizeof(views));
  shaderComputeBioDose.SetUAVViews(uavViews, sizeof(uavViews));

  shaderComputeBioDose.LoadResources();

  SDispatchBuffAuxDose dispatchBuff = {};
  dispatchBuff.numVoxels = doseGrid->numVoxels;
  dispatchBuff.numFractions = numFractions;
  memcpy(&dispatchBuff.rbe0, m_transportParams->rbeVals, 3*sizeof(float));

  // Run the BioDose Compute Shader
  unsigned int numFullDispatches = dispatchBuff.numVoxels / NUM_DISPATCH_THREADS;
  unsigned int numRemainderGroups = (dispatchBuff.numVoxels % NUM_DISPATCH_THREADS) / NUM_GROUP_THREADS;

  for (unsigned int dispatch = 0; dispatch < numFullDispatches; ++dispatch)
  {
    dispatchBuff.dispatchIndex = dispatch * NUM_DISPATCH_THREADS;
    m_bufferDispatch.Upload(&dispatchBuff);

    m_dxContext->Dispatch(NUM_DISPATCH_GROUPS, 1, 1);
  }

  dispatchBuff.dispatchIndex = numFullDispatches * NUM_DISPATCH_THREADS;
  m_bufferDispatch.Upload(&dispatchBuff);

  m_dxContext->Dispatch(numRemainderGroups, 1, 1);

  // Clear output buffers so they can be used as input in downstream processing
  ClearComputeUAViews(0, sizeof(uavViews) / sizeof(ID3D11UnorderedAccessView*) - 1);

  forPrrayIN(m_txtrGroupDosesPatient, txtr, DOSE_TYPE_LET, 6)
  {
    CalcTextureBounds(txtr);
  }

  //CalcDoseProfiles(); // MLW dbg

  return;
}

void CEnginePostCalc::CalcTextureBounds(CDxTexture3D* inputTexture)
{

  D3D11_MAPPED_SUBRESOURCE textureMap;
  D3D11_TEXTURE3D_DESC desc;
  float bounds[2];
  double doseSum = 0.0;

  inputTexture->m_texture->GetDesc(&desc);

  TRACE("DIMS %d %d %d\n", desc.Width, desc.Height, desc.Depth);

  m_dxContext->CopyResource(inputTexture->m_textureRead, inputTexture->m_texture); // MLW dbg
  m_dxContext->Map(inputTexture->m_textureRead, 0, D3D11_MAP_READ, 0, &textureMap);

  float* xEnd;
  float* yEnd;
  float* xPtr;
  float* yPtr;
  float* zPtr;
  float* zStart = (float*)textureMap.pData;
  float* zEnd   = zStart + desc.Depth * (textureMap.DepthPitch / sizeof(float));
  int zStep = textureMap.DepthPitch / sizeof(float);
  int yStep = textureMap.RowPitch / sizeof(float);

  bounds[0] = *zStart;
  bounds[1] = *zStart;
  unsigned int voxelIndex = 0;
  int numVoxFilled = 0;

  for (zPtr = zStart; zPtr < zEnd; zPtr += zStep)
  {
    yEnd = zPtr + desc.Height * yStep;

    for (yPtr = zPtr; yPtr < yEnd; yPtr += yStep)
    {
      xEnd = yPtr + desc.Width;

      for (xPtr = yPtr; xPtr < xEnd; ++xPtr)
      {
        doseSum += *xPtr;

        if (*xPtr > 0.0)
        {
          ++numVoxFilled;
        }

        if (*xPtr > bounds[1]) // MLw dbg
        {
          bounds[1] = *xPtr;
          voxelIndex = (xPtr - yPtr) + (yPtr - zPtr)/yStep * desc.Width
            + (zPtr - zStart)/zStep * desc.Width * desc.Height;
        }
        else if (*xPtr < bounds[0])
        {
          bounds[0] = *xPtr;
        }
      }
    }
  }

  m_dxContext->Unmap(inputTexture->m_textureRead, 0);

  TRACE("TextureBoundsMap %6.4e %6.4e\n", bounds[0], bounds[1]);
  TRACE("DosePatientTxtrMap %6.4e %d\n", doseSum, numVoxFilled);

  inputTexture->m_dataMin = bounds[0];
  inputTexture->m_dataMax = bounds[1];

  CalcTextureBounds2(inputTexture->m_view);

  return;
}

float CEnginePostCalc::LoadExternalDoseDicom(LPCTSTR fileName)
{
  CDicomDose dicomDose;

  SafeDeleteArray(m_dataGridExtrnl.data);

  m_dataGridExtrnl = {};

  dicomDose.ParseDoseFile(fileName, &m_dataGridExtrnl);

  CDxTexture3D* txtr = m_txtrGroupDosesExternal->Alloc(2);

  txtr->CreateTexture3D(m_dataGridExtrnl.dims, m_dataGridExtrnl.data, m_dataGridExtrnl.bounds);

  m_txtrGroupDosesExternal->m_volumeGrid = m_dataGridExtrnl;

  return m_dataGridExtrnl.bounds[1];
}

bool CEnginePostCalc::LoadStructsDicom(LPCTSTR fileName)
{
  CDicomStruct structRT;
  CSliceHedra sliceHedra;

  structRT.ParseStructFile(fileName, &sliceHedra);

  return true;
}

float CEnginePostCalc::LoadExternalDoseMSH(FILE* fd, SVoxGridGeometry* grid)
{
  //float buffer[2];

  ////m_spotDoseInfo.m_volumeGrid = &m_spotDoseGrid;

  //memcpy(&m_voxGridDoseExternal, grid, sizeof(SVoxGridGeometry));

  ////for (int i = 0; i < 3; ++i)
  ////{
  ////  fread(&grid->dims[i], sizeof(float), 1, fd);
  ////  fread(buffer, sizeof(float), 2, fd);
  ////  grid->voxelOrigin[i] = buffer[0];
  ////  grid->voxelSize[i] = buffer[1] - buffer[0];
  ////  fseek(fd, (grid->dims[i] - 2) * sizeof(float), SEEK_CUR);
  ////}

  //m_voxGridDoseExternal.data = new float[grid->numVoxels];

  //int numVoxSlice = grid->dims[0] * grid->dims[1];

  //long int n;
  //n = ftell(fd);
  //if ( (n = fread(m_voxGridDoseExternal.data, sizeof(float), grid->numVoxels, fd)) != grid->numVoxels)
  //{
  //  return false;
  //}

  //unsigned int* dataStart = (unsigned int*)m_voxGridDoseExternal.data;
  //unsigned int* dataPtr = dataStart;

  ////for (unsigned int* dataPtr = dataStart; dataPtr < dataEnd; ++dataPtr)
  ////{
  ////  dataVal = (float)(SWAP_ENDIAN(*dataPtr) / GY_2_CGY);

  ////  if (dataVal > doseMax)
  ////  {
  ////    doseMax = dataVal;
  ////  }

  ////  doseSum += dataVal;
  ////}

  //float* dosePtr;
  //float doseVal;
  //float doseMax = 0.0;
  //float doseSum = 0.0;

  //unsigned int* startX;
  //unsigned int* endX;
  //unsigned int* startZ;
  //unsigned int* endZ;
  //unsigned int* startY = dataStart;
  //unsigned int* endY = startY + grid->dims[0] * grid->dims[1];

  //unsigned int* voxX;
  //unsigned int* voxY;
  //unsigned int* voxZ;

  //int count = 0;
  //for (voxY = startY; voxY < endY; voxY += grid->dims[0])
  //{
  //  startZ = voxY;
  //  endZ   = startZ + (grid->dims[2] - 1) * numVoxSlice;

  //  for (voxZ = endZ; voxZ >=startZ; voxZ -= numVoxSlice)
  //  {
  //    startX = voxZ;
  //    endX   = startX + grid->dims[0];

  //    for(voxX = startX; voxX < endX; ++voxX)
  //    {
  //      doseVal = ((float)SWAP_ENDIAN(*voxX)) / GY_2_CGY;

  //      if (doseVal > doseMax)
  //      {
  //        doseMax = doseVal;
  //      }

  //      dosePtr = (float*)voxX;
  //      *dosePtr = doseVal;

  //      ++count;
  //    }
  //  }
  //}

  //int index;
  //int x, y, z;
  //for (float* fPtr = m_voxGridDoseExternal.data; fPtr < m_voxGridDoseExternal.data + grid->numVoxels; ++fPtr)
  //{
  //  if (*fPtr != 10.0)
  //  {
  //    index = fPtr - m_voxGridDoseExternal.data;
  //    z = index / numVoxSlice;
  //    y = (index - z * numVoxSlice) / grid->dims[1];
  //    x = index - z * numVoxSlice - y * grid->dims[1];
  //  }
  //}

  //m_voxGridDoseExternal.bounds[1] = doseMax;
  //m_voxGridDoseExternal.bounds[0] = 0;


  //m_arrayTxtrExternal.Alloc(1);

  //CDxTexture3D* txtr = m_arrayTxtrExternal.ItemPtr(0);
  //txtr->CreateTexture3D(m_voxGridDoseExternal.dims, m_voxGridDoseExternal.data, m_voxGridDoseExternal.bounds);

  ////m_txtrGroupExternal->txtrArray  = &m_arrayTxtrExternal;
  ////m_txtrGroupExternal->m_volumeGrid = &m_voxGridDoseExternal;

  ////// Connect the external dose to the transport results
  ////m_transportResults->doseExternal.txtrArray = &m_arrayTxtrExternal;
  ////m_transportResults->doseExternal.m_volumeGrid = &m_voxGridDoseExternal;

  //SafeDeleteArray(m_voxGridDoseExternal.data);

  return 0;
}


void CEnginePostCalc::CalcTextureBounds2(ID3D11ShaderResourceView* textureview)
{
  SVoxGridGeometry* doseGrid = &m_txtrGroupDosesPatient->m_volumeGrid; // MLW todo: make general;

  //CDxBuffRW voxBuff;
  //voxBuff.CreateBuffRWTyped(doseGrid->numVoxels, DXGI_FORMAT_R32_FLOAT);
  //voxBuff.CreateBufferRead();

  //CDxBuffRW appendBuff;
  //appendBuff.CreateBuffRWStruct(1024, 4 * sizeof(float), NULL, APPEND_BUFFER);
  //appendBuff.CreateBufferRead();

  ID3D11Buffer*              buffers[]  = { m_bufferDispatch.m_buffer };
  ID3D11ShaderResourceView*  views[]    = { textureview };
  ID3D11UnorderedAccessView* uavViews[] = { m_bufferThreads.m_viewRW};

  m_boundsCalcTxtrCS.SetConstantBuffers(buffers, sizeof(buffers));
  m_boundsCalcTxtrCS.SetResourceViews(views, sizeof(views));
  m_boundsCalcTxtrCS.SetUAVViews(uavViews, sizeof(uavViews));

  m_boundsCalcTxtrCS.LoadResources();

  // Run the BioDose Compute Shader
  unsigned int dispatchInfo[4] = {};
  dispatchInfo[1] = doseGrid->numVoxels;

  TRACE("NumVox %d\n", dispatchInfo[1]);

  //dispatchInfo[1] = doseGrid->numVoxels;
  //unsigned int numFullDispatches   = doseGrid->numVoxels / NUM_DISPATCH_THREADS;
  //unsigned int numRemainderThreads = doseGrid->numVoxels % NUM_DISPATCH_THREADS;
  //unsigned int numRemainderGroups  = numRemainderThreads / NUM_GROUP_THREADS;
  //if (numRemainderThreads % NUM_GROUP_THREADS)
  //{
  //  ++numRemainderGroups;
  //}

  //// *********** Run the Shader ****************
  //for (unsigned int dispatch = 0; dispatch < numFullDispatches; ++dispatch)
  //{
  //  dispatchInfo[0] = dispatch * NUM_DISPATCH_THREADS;
  //  m_bufferDispatch.Upload(dispatchInfo);

  //  m_dxContext->Dispatch(NUM_DISPATCH_GROUPS, 1, 1);
  //}

  dispatchInfo[0] = 0;
  m_bufferDispatch.Upload(dispatchInfo);

  //m_dxContext->Dispatch(numRemainderGroups, 1, 1);
  m_dxContext->Dispatch(1, 1, 1);

  D3D11_MAPPED_SUBRESOURCE mappedResource2;

  //float check[4096]; // MLW dbg: check thread buffer


  //m_dxContext->CopyResource(appendBuff.m_bufferRead, appendBuff.m_buffer);
  //m_dxContext->Map(appendBuff.m_bufferRead, 0, D3D11_MAP_READ, 0, &mappedResource2);

  //memcpy(check, mappedResource2.pData, 4096 * sizeof(float));
  //float* data = (float*)mappedResource2.pData;

  //FILE* fd = fopen("../Data/BinMax.txt", "w");
  //for(int i = 0; i < 1024; ++i, data += 4)
  //{
  //  fprintf(fd, "%4d %f %f %f %f\n", i, *data, *(data + 1), *(data + 2), *(data + 3));
  //}
  //fclose(fd);

  ////TRACE("Append %f %f %d %f\n", min, max, maxBin, voxel);

  //m_dxContext->Unmap(appendBuff.m_bufferRead, 0);

  m_dxContext->CopyResource(m_bufferThreads.m_bufferRead, m_bufferThreads.m_buffer);
  m_dxContext->Map(m_bufferThreads.m_bufferRead, 0, D3D11_MAP_READ, 0, &mappedResource2);

  // Note data is float2 on GPU alternating min,max
  double* data = (double*)mappedResource2.pData;
  double min = *data;
  double max = *(data + 1);

  //TRACE("MinMax %f %f\n", see[0], see[1]);
  for(int i = 0; i < 1024; ++i, ++data)
  {
    if (*data < min)
    {
      min = *data;
    }

    ++data;
    if (*data > max)
    {
      max = *data;
    }
  }

  TRACE("DoseMinMaxBin %6.4e %6.4e\n", min, max);

  m_dxContext->Unmap(m_bufferThreads.m_bufferRead, 0);

  //m_dxContext->CopyResource(voxBuff.m_bufferRead, voxBuff.m_buffer);
  //m_dxContext->Map(voxBuff.m_bufferRead, 0, D3D11_MAP_READ, 0, &mappedResource2);

  //float* usData = (float*)mappedResource2.pData;
  //float voxMax = *usData;
  //int maxIndex = 0;
  //int firstIndex = 0;
  //int voxIndex;
  //float firstValue;

  ////TRACE("MinMax %f %f\n", see[0], see[1]);
  //for(int i = 0; i < dispatchInfo[1]; ++i, ++usData)
  //{
  //  if ((firstIndex == 0) && (*usData > 0))
  //  {
  //    firstValue = *usData;
  //    firstIndex = i;
  //  }

  //  if (*usData == max)
  //  {
  //    float see = max;
  //    maxIndex = i;
  //  }
  //  
  //  if (*usData > voxMax)
  //  {
  //    voxMax = *usData;
  //    voxIndex = i;
  //  }
  //}

  //TRACE("VoxFirst %d %f\n", firstIndex, firstValue);
  //TRACE("VoxMax %d %d %f\n", maxIndex, voxIndex, voxMax);

  //usData = (float*)mappedResource2.pData;

  //m_dxContext->Unmap(voxBuff.m_bufferRead, 0);


  // Clear the RWview so resource can be used as input in next shader
  ID3D11UnorderedAccessView* nullUnorderedView = NULL;
  m_dxContext->CSSetUnorderedAccessViews(0, 1, &nullUnorderedView, (UINT*)(&nullUnorderedView));

  //ID3D11Buffer*              buffers2[]  = { m_bufferDispatch.m_buffer };
  //ID3D11ShaderResourceView*  views2[]    = { m_bufferThreads.m_view };
  //ID3D11UnorderedAccessView* uavViews2[] = { m_resultsFloatBuffer.m_viewRW };

  //m_arrayMaxCS.SetConstantBuffers(buffers2, sizeof(buffers2));
  //m_arrayMaxCS.SetResourceViews(views2, sizeof(views2));
  //m_arrayMaxCS.SetUAVViews(uavViews2, sizeof(uavViews2));

  //m_arrayMaxCS.LoadResources();

  //m_dxContext->Dispatch(1, 1, 1);

  //D3D11_MAPPED_SUBRESOURCE mappedResource;
  //m_dxContext->CopyResource(m_resultsFloatBuffer.m_bufferRead, m_resultsFloatBuffer.m_buffer);
  //m_dxContext->Map(m_resultsFloatBuffer.m_bufferRead, 0, D3D11_MAP_READ, 0, &mappedResource);

  //memcpy(bounds, mappedResource.pData, 2 * sizeof(float));

  //float* see = (float*)mappedResource.pData;

  //TRACE("MinMax %f %f\n", see[0], see[1]);

  //m_dxContext->Unmap(m_resultsFloatBuffer.m_bufferRead, 0);

  return;
}

void CEnginePostCalc::CalcDoseHistograms(void)
{
  // MLW todo:
  return;
}

void CEnginePostCalc::SumBeamDoses(CIonPlan* ionPlan)
{
  CString folder = g_configInstance->m_directoryBase + "\\AppResults\\BeamDoses";

  CDicomDose dicomDose;
  dicomDose.SumBeamDoses(ionPlan, folder);

  return;
}

float CEnginePostCalc::SumExternalDose(CIonPlan* ionPlan)
{
 
  SVoxGridGeometry* destGrid   = &m_dataGridExtrnl;
  SVoxGridGeometry* sourceGrid = &m_txtrGroupDosesPatient->m_volumeGrid;

  SResamplerParams params = {};

  params.destNumVox[0]      = destGrid->dims[0];
  params.destNumVox[1]      = destGrid->dims[1];
  params.destNumVox[2]      = destGrid->dims[2];
  params.destVoxOrigin[0]   = destGrid->voxelOrigin[0];
  params.destVoxOrigin[1]   = destGrid->voxelOrigin[1];
  params.destVoxOrigin[2]   = destGrid->voxelOrigin[2];
  params.destVoxSize[0]     = destGrid->voxelSize[0];
  params.destVoxSize[1]     = destGrid->voxelSize[1];
  params.destVoxSize[2]     = destGrid->voxelSize[2];
  params.destNumVoxSlice    = destGrid->dims[0] * destGrid->dims[1];
  params.destNumVoxVolume   = destGrid->numVoxels;
  params.sourceVolOrigin[0] = sourceGrid->voxelOrigin[0] - sourceGrid->voxelSize[0] / 2;
  params.sourceVolOrigin[1] = sourceGrid->voxelOrigin[1] - sourceGrid->voxelSize[1] / 2;
  params.sourceVolOrigin[2] = sourceGrid->voxelOrigin[2] - sourceGrid->voxelSize[2] / 2;
  params.sourceVolSize[0]   = sourceGrid->dims[0] * sourceGrid->voxelSize[0];
  params.sourceVolSize[1]   = sourceGrid->dims[1] * sourceGrid->voxelSize[1];
  params.sourceVolSize[2]   = sourceGrid->dims[2] * sourceGrid->voxelSize[2];


  m_dxBuffDoseSumParams.CreateConstantBufferDyn(sizeof(SResamplerParams), &params);

  SVoxGridGeometry* doseGrid = destGrid; // MLW 

  CDxTexture3D* dxBuffDoseSum = m_txtrGroupDosesExternal->ItemPtr(1);

  dxBuffDoseSum->CreateRWTextureAll3D(m_dataGridExtrnl.dims, DXGI_FORMAT_R32_FLOAT);

  ID3D11ShaderResourceView* resViews[] =
  {
    m_txtrGroupDosesPatient->ItemPtr(0)->m_view,
    m_txtrGroupDosesExternal->ItemPtr(0)->m_view
  };

  m_doseSumCS.BuildShader(g_doseSumCS, sizeof(g_doseSumCS));

  m_doseSumCS.SetConstantBuffers(&m_dxBuffDoseSumParams.m_buffer, sizeof(&m_dxBuffDoseSumParams.m_buffer));
  m_doseSumCS.SetSamplers(&m_textureSampler->m_sampler, sizeof(&m_textureSampler->m_sampler));
  m_doseSumCS.SetResourceViews(resViews, sizeof(resViews));
  m_doseSumCS.SetUAVViews(&dxBuffDoseSum->m_viewRW, sizeof(&dxBuffDoseSum->m_viewRW));

  m_doseSumCS.LoadResources();

  int numGroups = (destGrid->numVoxels / 1024) + 1;

  m_dxContext->Dispatch(numGroups, 1, 1);

  ClearComputeUAViews(0, 1);

  CalcTextureBounds(dxBuffDoseSum);

  return dxBuffDoseSum->m_dataMax;
}


void  CEnginePostCalc::SaveDicom(LPCTSTR fileName, LPCTSTR seriesDescr, CIonPlan* ionPlan, int saveType, int indexDose)
{
  CTextureGroup* saveGroup;
  int indexBeam;

  switch (saveType)
  {
  case SAVE_TYPE_PATIENT:
    saveGroup = m_txtrGroupDosesPatient;
    indexBeam = 0;
    break;

  case SAVE_TYPE_QA:
    saveGroup = m_txtrGroupDosesQA;
    indexBeam = indexDose + 1;
    break;

  case SAVE_TYPE_GAMMA:
    saveGroup = m_txtrGroupGammas;
    indexBeam = 0;
    break;

  default:
    return;
  }

  SaveDoseDicom(fileName, seriesDescr, ionPlan, &saveGroup->m_volumeGrid, saveGroup->ItemPtr(indexDose), indexBeam);

  return;
}

void CEnginePostCalc::SaveDoseDicom(LPCTSTR fileName, LPCTSTR seriesDescr, CIonPlan* ionPlan, 
  SVoxGridGeometry* grid, CDxTexture3D* doseTexture, int indexBeam)
{
  CDicomDose dicomDose;

  float scaleFactor = USHRT_MAX / doseTexture->m_dataMax;

  if (scaleFactor > 10000.0)
  {
    scaleFactor = 10000.0;
  }
  else if (scaleFactor > 1000.0)
  {
    scaleFactor = 1000.0;
  }
  else if (scaleFactor > 100.0)
  {
    scaleFactor = 100.0;
  }
  else if (scaleFactor > 10.0)
  {
    scaleFactor = 10.0;
  }
  else
  {
    scaleFactor = 1.0;
  }

  CUniArray<unsigned short> doseData(grid->numVoxels);

  D3D11_MAPPED_SUBRESOURCE mapResource;

  m_dxContext->CopyResource(doseTexture->m_textureRead, doseTexture->m_texture);
  m_dxContext->Map(doseTexture->m_textureRead, 0, D3D11_MAP_READ, 0, &mapResource);

  float* yEnd;
  float* yPtr;
  float* zPtr;
  float* xPtr;
  float* xEnd;
  float* zStart = (float*)mapResource.pData;
  float* zEnd   = zStart + grid->dims[2] * (mapResource.DepthPitch / sizeof(float));
  int zStep = mapResource.DepthPitch / sizeof(float);
  int yStep = mapResource.RowPitch / sizeof(float);
  float max = *zStart;
  unsigned short umax;
  unsigned short umax2 = 0;
  float max2 = 0;


  unsigned short* dosePtr = doseData(0);
  for (zPtr = zStart; zPtr < zEnd; zPtr += zStep)
  {
    yEnd = zPtr + grid->dims[1] * yStep;

    for (yPtr = zPtr; yPtr < yEnd; yPtr += yStep)
    {
      xEnd = yPtr + grid->dims[0];

      for (xPtr = yPtr; xPtr < xEnd; ++xPtr)
      {
        *dosePtr = (unsigned short) (*xPtr * scaleFactor);

        if (*xPtr > max)
        {
          max = *xPtr;
          umax = *dosePtr;
        }

        if (*dosePtr > umax2)
        {
          umax2 = *dosePtr;
          max2 = *xPtr;
        }

        ++dosePtr;
      }
    }
  }

  m_dxContext->Unmap(doseTexture->m_textureRead, 0);

  dicomDose.BuildDoseFile(ionPlan, doseData(0), grid, fileName, seriesDescr, (1.0F/scaleFactor), indexBeam);

  return;
}

void CEnginePostCalc::SaveQABeamDose(FILE* fd, SVoxGridGeometry* subGrid, int qaIndex)
{

  SaveDoseRaw(fd, m_txtrGroupDosesQA->ItemPtr(qaIndex), &m_txtrGroupDosesQA->m_volumeGrid, subGrid);

  return;
}

void  CEnginePostCalc::SaveQABeamGeometry(FILE* fd, int* baseGridDims, float* isocenterQA, int* doseGridDims)
{
  // Output in RTOG coordinates X, -Y, z
  fprintf(fd, "calc-vol \"348.00, 348.00, 348.00, ");
  fprintf(fd, " %6.2f, %6.2f, %6.2f,", isocenterQA[0], isocenterQA[2], isocenterQA[1]); // MLW todo: check with Nick
  fprintf(fd, " %d, %d, %d\"", doseGridDims[0], doseGridDims[2], doseGridDims[1]);

  return;
}

void CEnginePostCalc::SaveDoseRaw2(FILE* fd, CDxTexture3D* doseTexture, SVoxGridGeometry* subGrid, int* dims)
{

  D3D11_MAPPED_SUBRESOURCE mapResource;

  m_dxContext->CopyResource(doseTexture->m_textureRead, doseTexture->m_texture);
  m_dxContext->Map(doseTexture->m_textureRead, 0, D3D11_MAP_READ, 0, &mapResource);

  int stepZ = mapResource.DepthPitch / sizeof(unsigned short);
  int stepY = mapResource.RowPitch / sizeof(unsigned short);
  unsigned short* startZ = (unsigned short*)mapResource.pData;
  unsigned short* endZ   = startZ + dims[2] * stepZ;
  unsigned short* voxZ;
  unsigned short* voxY;
  unsigned short* endY;
  unsigned short* voxX;
  unsigned short* endX;

  unsigned short doseVal;
  unsigned short doseBigEnd;

  for (voxZ = startZ; voxZ < endZ; voxZ += stepZ)
  {
    endY = voxZ + dims[1] * stepY;

    for (voxY = voxZ; voxY < endY; voxY += stepY)
    {
      endX = voxY + dims[0];
      for (voxX = voxY; voxX < endX; ++voxX)
      {
        doseVal = *voxX * GY_2_CGY;
        doseBigEnd = (((doseVal) >> 24) | (((doseVal) & 0x00FF0000) >> 8) | (((doseVal) & 0x0000FF00) << 8) | ((doseVal) << 24));
        fwrite(&doseBigEnd, sizeof(unsigned short), 1, fd);
      }
    }
  }

  m_dxContext->Unmap(doseTexture->m_textureRead, 0);

  return;
}

void CEnginePostCalc::SaveDoseRaw(FILE* fd, CDxTexture3D* doseTexture, SVoxGridGeometry* baseGrid, SVoxGridGeometry* subGrid)
{

  D3D11_MAPPED_SUBRESOURCE mapResource;

  m_dxContext->CopyResource(doseTexture->m_textureRead, doseTexture->m_texture);
  m_dxContext->Map(doseTexture->m_textureRead, 0, D3D11_MAP_READ, 0, &mapResource);

  int stepMapZ = mapResource.DepthPitch / sizeof(float);
  int stepMapY = mapResource.RowPitch / sizeof(float);

  unsigned int shift[3];
  shift[0] = (unsigned int) (subGrid->voxelOrigin[0] - baseGrid->voxelOrigin[0]) / baseGrid->voxelSize[0];
  shift[1] = (unsigned int) (subGrid->voxelOrigin[1] - baseGrid->voxelOrigin[1]) / baseGrid->voxelSize[1];
  shift[2] = (unsigned int) (subGrid->voxelOrigin[2] - baseGrid->voxelOrigin[2]) / baseGrid->voxelSize[2];

  float* doseData = (float*)mapResource.pData;

  unsigned int numVoxSlice = subGrid->dims[2] * subGrid->dims[0];

  unsigned int  doseVal;
  unsigned int* dosePtr;
  unsigned int* doseBigEnd = new unsigned int[numVoxSlice];

  float* voxX;
  float* voxY;
  float* voxZ;

  float* startX;
  float* endX;
  float* startZ;
  float* endZ;
  float* startY = doseData + shift[1] * stepMapY;
  float* endY = startY + subGrid->dims[1] * stepMapY;

  unsigned int doseMax = 0;
  for(voxY = startY; voxY < endY; voxY += stepMapY)
  {
    dosePtr = doseBigEnd;

    startZ = voxY + shift[2] * stepMapZ;
    endZ   = startZ + (subGrid->dims[2]  - 1) * stepMapZ;

    for(voxZ = endZ; voxZ >= startZ; voxZ -= stepMapZ)
    {
      startX = voxZ + shift[0];
      endX   = startX + subGrid->dims[0];

      for(voxX = startX; voxX < endX; ++voxX)
      {
        doseVal = (unsigned int) (*voxX * GY_2_CGY + 0.5);
        *dosePtr = SWAP_ENDIAN(doseVal);

        if (doseVal > doseMax)
        {
          doseMax = doseVal;
        }
        ++dosePtr;
      }
    }
    fwrite(doseBigEnd, sizeof(unsigned int), numVoxSlice, fd);
  }

  m_dxContext->Unmap(doseTexture->m_textureRead, 0);
  TRACE("BEAMMAX %d\n", doseMax);

  delete[] doseBigEnd;

  return;
}

//void CEnginePostCalc::SaveDoseRaw(FILE* fd, CDxTexture3D* doseTexture, SVoxGridGeometry* baseGrid, SVoxGridGeometry* subGrid)
//{
//
//  D3D11_MAPPED_SUBRESOURCE mapResource;
//
//  m_dxContext->CopyResource(doseTexture->m_textureRead, doseTexture->m_texture);
//  m_dxContext->Map(doseTexture->m_textureRead, 0, D3D11_MAP_READ, 0, &mapResource);
//
//  int stepZ = mapResource.DepthPitch / sizeof(float);
//  int stepY = mapResource.RowPitch / sizeof(float);
//
//  CDataWrap<float> doseWrap((float*)mapResource.pData, stepZ * stepY);
//
//  unsigned int shift[3];
//
//  shift[0] = (subGrid->voxelOrigin[0] - baseGrid->voxelOrigin[0]) / baseGrid->voxelSize[0];
//  shift[1] = (subGrid->voxelOrigin[1] - baseGrid->voxelOrigin[1]) / baseGrid->voxelSize[1];
//  shift[2] = (subGrid->voxelOrigin[2] - baseGrid->voxelOrigin[2]) / baseGrid->voxelSize[2];
//
//  unsigned int startZ = shift[2] * stepZ;
//  unsigned int endZ   = startZ + subGrid->dims[2] * stepZ;
//  unsigned int startY = shift[1] * stepY;
//  unsigned int endY   = startY + subGrid->dims[1] * stepY;
//  unsigned int startX = shift[0];
//  unsigned int endX   = startX + subGrid->dims[0];
//  unsigned int numVoxSlice = subGrid->dims[1] * subGrid->dims[0];
//
//  unsigned int doseVal;
//  unsigned int* doseBigEnd = new unsigned int[numVoxSlice];
//  unsigned int* dosePtr;
//
//  float* voxX;
//  float* voxY;
//  float* voxZ;
//
//  int fileSize = 0;
//  int check = sizeof(unsigned int);
//
// // forwrapspansx(doseWrap, voxZ, startZ, endZ, stepZ)
//  doseWrap.m_loop0 = doseWrap.m_data + startZ;
//  doseWrap.m_loop1 = doseWrap.m_data + endZ;
//  for(voxZ = doseWrap.m_data; voxZ <= doseWrap.m_loop1; voxZ += stepZ)
//  {
//    voxY = voxZ;
//
//    dosePtr = doseBigEnd;
//    forwrapspansx(doseWrap, voxY, startZ, endY, stepY)
//    {
//      voxX = voxY;
//      
//      forwrapspansx(doseWrap, voxX, startX, endX, 1)
//      {
//        doseVal = ((unsigned int) (*voxX + 0.5)) * GY_2_CGY;
//        *dosePtr = BIG_ENDIAN(doseVal);
//        ++dosePtr;
//      }
//    }
//
//    fwrite(doseBigEnd, sizeof(unsigned int), numVoxSlice, fd);
//    fileSize += numVoxSlice * sizeof(unsigned int);
//  }
//
//  m_dxContext->Unmap(doseTexture->m_textureRead, 0);
//
//  delete[] doseBigEnd;
//
//  return;
//}

#define NUM_DOSE_BINS (100)

void CEnginePostCalc::CalcDoseHistogram(int doseType, FILE* fd, int flag)
{
  int bins[NUM_DOSE_BINS];
  int binNum;

  D3D11_MAPPED_SUBRESOURCE doseMap;

  m_dxContext->CopyResource(m_txtrGroupDosesPatient->GetTextureRead(doseType), m_txtrGroupDosesPatient->GetTexture(doseType));
  m_dxContext->Map(m_txtrGroupDosesPatient->GetTextureRead(doseType), 0, D3D11_MAP_READ, 0, &doseMap);


  float* doseData = (float*) doseMap.pData;
  float doseMax = m_txtrGroupDosesPatient->ItemPtr(doseType)->m_dataMax;

  float* xStart = doseData;
  float* end = xStart + m_txtrGroupDosesPatient->m_volumeGrid.numVoxels;

  int numPos = 0;
  for (float* voxelDose = doseData; voxelDose < end; ++voxelDose)
  {
    if (*voxelDose > 0.0)
    {
      binNum = (int)((*voxelDose / doseMax) * NUM_DOSE_BINS);
      if(binNum > -1)
      {
        ++bins[binNum];
      }
      ++numPos;
    }
  }

  CString fileName;
  flag ? fileName = "HistoSec.txt" : fileName = "HistoPrim.txt";

  fd = m_fileIO->OpenFile(FILE_DEST_DATA, fileName, "w");
  for (int bin = 0; bin < NUM_DOSE_BINS; ++bin)
  {
    fprintf(fd, "%3d %e\n", bin, (float)bins[bin] / numPos);
  }
  m_fileIO->CloseFile();

  return;
}

void CEnginePostCalc::CalcEnergy(SEnergies* energies)
{
  *energies = {};

  energies->energyCompDevices = CalcEnergyDevices();

  TRACE("EnergyDevices %6.4e\n", energies->energyCompDevices, 0.0);

  ClearComputeUAViews(0, 1);

  TRACE("GMC ENERGY PATIENT\n");
  energies->energyCompPatient = CalcEnergyPatientMulti(&m_txtrGroupDosesPatient->m_volumeGrid, m_txtrGroupDosesPatient->ItemPtr(0));

  if (m_txtrGroupDosesExternal->m_numItems)
  {
    TRACE("EXTERNAL ENERGY PATIENT\n");
    energies->energyExternalPatient = CalcEnergyPatientMulti(m_txtrGroupDosesExternal->GetGrid(), m_txtrGroupDosesExternal->ItemPtr(0));
  }


  // Alternative patient energy calc: CPU vs GPU
  CalcEnergyPatient();

  energies->energyCompTotal   = energies->energyCompDevices + energies->energyCompPatient;
  energies->energyPlanProtons = m_transportParams->energyPlan;

  TRACE("EnergyCalcTotal %6.4e\n", energies->energyCompTotal);
  TRACE("EnergyPlan %6.4e\n", energies->energyPlanProtons);

  return;
}

double CEnginePostCalc::CalcEnergyDevices(void)
{
  double energyDevices = 0.0;

  if (m_appState->m_compObjects.ionPlan->m_arrayDevices.m_numItems == 0)
  {
    return energyDevices;
  }

  ID3D11Buffer* buffers[]               = { m_bufferDispatch.m_buffer };
  ID3D11ShaderResourceView* views[]     = { m_buffRWProtonsAuxBase->m_view };
  ID3D11UnorderedAccessView* uavViews[] = { m_bufferThreads.m_viewRW };

  m_energyDevicesCS.SetConstantBuffers(buffers, sizeof(buffers));
  m_energyDevicesCS.SetResourceViews(views, sizeof(views));
  m_energyDevicesCS.SetUAVViews(uavViews, sizeof(uavViews));

  m_energyDevicesCS.LoadResources();

  // Run the Compute Shader
  unsigned int dispatchInfo[4] = {};
  dispatchInfo[1] = m_transportParams->numProtonsTransport;

  m_bufferDispatch.Upload(dispatchInfo);

  m_dxContext->Dispatch(1, 1, 1);

  D3D11_MAPPED_SUBRESOURCE mappedResource;

  m_dxContext->CopyResource(m_bufferThreads.m_bufferRead, m_bufferThreads.m_buffer);
  m_dxContext->Map(m_bufferThreads.m_bufferRead, 0, D3D11_MAP_READ, 0, &mappedResource);

  double* data = (double*)mappedResource.pData;

  for (int i = 0; i < 1024; ++data, ++i)
  {
    energyDevices += *data;
    ++data;
  }

  m_dxContext->Unmap(m_bufferThreads.m_bufferRead, 0);

  energyDevices *= m_transportParams->numCycles;

  return energyDevices;
}

double CEnginePostCalc::CalcEnergyPatient(void)
{
  double energyPatient = 0.0;

  SVoxGridGeometry* doseGrid = &m_txtrGroupDosesPatient->m_volumeGrid; // MLW todo: make general;

  ID3D11Buffer* buffers[]               = { m_bufferDispatch.m_buffer };
  ID3D11ShaderResourceView* views[]     = { m_txtrGroupImagesPatient->GetTextureView(IMG_TYPE_RSP), m_txtrGroupDosesPatient->GetTextureView(DOSE_TYPE_DOSE) };
  ID3D11UnorderedAccessView* uavViews[] = { m_bufferThreads.m_viewRW };

  m_energyCS.SetConstantBuffers(buffers, sizeof(buffers));
  m_energyCS.SetResourceViews(views, sizeof(views));
  m_energyCS.SetUAVViews(uavViews, sizeof(uavViews));

  m_energyCS.LoadResources();

  // Run the Compute Shader
  unsigned int dispatchInfo[4] = {};
  dispatchInfo[1] = doseGrid->numVoxels;

  m_bufferDispatch.Upload(dispatchInfo);

  m_dxContext->Dispatch(1, 1, 1);

  D3D11_MAPPED_SUBRESOURCE mappedResource;

  m_dxContext->CopyResource(m_bufferThreads.m_bufferRead, m_bufferThreads.m_buffer);
  m_dxContext->Map(m_bufferThreads.m_bufferRead, 0, D3D11_MAP_READ, 0, &mappedResource);

  double* data = (double*)mappedResource.pData;
  double numVox = 0.0;

  for (int i = 0; i < 1024; ++data, ++i)
  {
    energyPatient += *data;
    ++data;
    numVox += *data;
  }

  m_dxContext->Unmap(m_bufferThreads.m_bufferRead, 0);

  energyPatient *= doseGrid->voxelVolume * 1.0e9;

  TRACE("EnergyPatientBins %6.4e %f\n", energyPatient, numVox);

  return energyPatient;
}

struct SParams
{
  float originDose[4];
  float voxSizeDose[4];
  float extentImg[4];

  uint numVoxels;
  uint pad0[7];
};

double CEnginePostCalc::CalcEnergyPatientMulti(SVoxGridGeometry* gridDose, CDxTexture3D* doseTexture)
{

  SParams params;

  SVoxGridGeometry* gridImage = m_txtrGroupImagesPatient->GetGrid();

  for (int index = 0; index < 3; ++index)
  {
    params.voxSizeDose[index] = gridDose->voxelSize[index];
    params.originDose[index]  = gridDose->voxelOrigin[index] - gridImage->voxelOrigin[index];
    params.extentImg[index]   = gridImage->dims[index] * gridImage->voxelSize[index];
  }

  params.numVoxels = gridDose->numVoxels;

  CDxBuffConstant bufferParams;
  bufferParams.CreateConstantBufferDyn(sizeof(SParams));

  ID3D11Buffer* buffers[]               = { bufferParams.m_buffer };
  ID3D11ShaderResourceView* views[]     = { m_txtrGroupImagesPatient->GetTextureView(IMG_TYPE_RSP), doseTexture->m_view };
  ID3D11UnorderedAccessView* uavViews[] = { m_bufferThreads.m_viewRW };
  ID3D11SamplerState* samplers[]        = { m_textureSampler->m_sampler };

  m_energyPatientFreeCS.SetConstantBuffers(buffers, sizeof(buffers));
  m_energyPatientFreeCS.SetResourceViews(views, sizeof(views));
  m_energyPatientFreeCS.SetUAVViews(uavViews, sizeof(uavViews));
  m_energyPatientFreeCS.SetSamplers(samplers, sizeof(samplers));

  m_energyPatientFreeCS.LoadResources();

  bufferParams.Upload(&params);

  m_dxContext->Dispatch(1, 1, 1);

  D3D11_MAPPED_SUBRESOURCE mappedResource;

  m_dxContext->CopyResource(m_bufferThreads.m_bufferRead, m_bufferThreads.m_buffer);
  m_dxContext->Map(m_bufferThreads.m_bufferRead, 0, D3D11_MAP_READ, 0, &mappedResource);

  double* data = (double*)mappedResource.pData;

  double energyPatient = 0.0;
  double numVox = 0.0;

  for (int i = 0; i < 1024; ++data, ++i)
  {
    energyPatient += *data;
    ++data;
    numVox += *data;
  }

  m_dxContext->Unmap(m_bufferThreads.m_bufferRead, 0);

  energyPatient *= gridDose->voxelVolume * 1.0e9;

  TRACE("EnergyPatientMultiBins %6.4e %f\n", energyPatient, numVox);

  return energyPatient;
}

#include "tgif.h"

SGammaResults* CEnginePostCalc::CalcGamma(SGammaThresholds* gammaThresholds)
{

  //Maximum number of iterations in search for smallest distance to agreement
  //Use zero for no iterations and tricubic interpolation (faster and less accurate)
  //Use -1 for no iterations and linear interpolation (fastest and least accurate)
  int Nmax = 12;

  CDxTexture3D* txtrDoseGMC     = m_txtrGroupDosesPatient->ItemPtr(0);
  SVoxGridGeometry* gridDoseGMC = m_txtrGroupDosesPatient->GetGrid();
  CDxTexture3D* txtrDoseExt     = m_txtrGroupDosesExternal->ItemPtr(0);
  SVoxGridGeometry* gridDoseExt = m_txtrGroupDosesExternal->GetGrid();

  CUniArray<float> doseDataGMC(gridDoseGMC->numVoxels);
  CUniArray<float> doseDataExt(gridDoseExt->numVoxels);

  vector<float> x0, y0, z0, x1, y1, z1;
  x0.resize(gridDoseExt->dims[0]);
  y0.resize(gridDoseExt->dims[1]);
  z0.resize(gridDoseExt->dims[2]);
  x1.resize(gridDoseGMC->dims[0]);
  y1.resize(gridDoseGMC->dims[1]);
  z1.resize(gridDoseGMC->dims[2]);

  float* first;
  float* dimPtr;
  float* end;
  float  dimPos;

  first = x0.data();
  end = first + gridDoseExt->dims[0];
  dimPos = gridDoseExt->voxelOrigin[0];
  for (dimPtr = first; dimPtr < end; ++dimPtr)
  {
    *dimPtr = dimPos;
    dimPos += gridDoseExt->voxelSize[0];
  }

  first = y0.data();
  end = first + gridDoseExt->dims[1];
  dimPos = gridDoseExt->voxelOrigin[1];
  for (dimPtr = first; dimPtr < end; ++dimPtr)
  {
    *dimPtr = dimPos;
    dimPos += gridDoseExt->voxelSize[1];
  }

  first = z0.data();
  end = first + gridDoseExt->dims[2];
  dimPos = gridDoseExt->voxelOrigin[2];
  for (dimPtr = first; dimPtr < end; ++dimPtr)
  {
    *dimPtr = dimPos;
    dimPos += gridDoseExt->voxelSize[2];
  }

  first = x1.data();
  end = first + gridDoseGMC->dims[0];
  dimPos = gridDoseGMC->voxelOrigin[0];
  for (dimPtr = first; dimPtr < end; ++dimPtr)
  {
    *dimPtr = dimPos;
    dimPos += gridDoseGMC->voxelSize[0];
  }

  first = y1.data();
  end = first + gridDoseGMC->dims[1];
  dimPos = gridDoseGMC->voxelOrigin[1];
  for (dimPtr = first; dimPtr < end; ++dimPtr)
  {
    *dimPtr = dimPos;
    dimPos += gridDoseGMC->voxelSize[1];
  }

  first = z1.data();
  end = first + gridDoseGMC->dims[2];
  dimPos = gridDoseGMC->voxelOrigin[2];
  for (dimPtr = first; dimPtr < end; ++dimPtr)
  {
    *dimPtr = dimPos;
    dimPos += gridDoseGMC->voxelSize[2];
  }

  Matrix<float> D0;  // reference dose matrix
  Matrix<float> D1;  // evaluated dose matrix (interpolated and searched)

  matrix_create(D0, x0.size(), y0.size(), z0.size(), 1);
  matrix_create(D1, x1.size(), y1.size(), z1.size(), 1);

  memcpy(D0.M.data(), m_dataGridExtrnl.data, D0.M.size() * sizeof(float));
  CopyTextureData(txtrDoseGMC, D1.M.data());

  string unit0("mm");   //Spatial coordinate units of the D0 dose grid
  string unit1("mm");   //Spatial coordinate units of the D1 dose grid
  string unit_d("mm");  //gamma distance tolerance
  string unitD0("Gy");  //D0 dose units
  string unitD1("Gy");  //D1 dose units
  string unit_D("Gy");  //gamma dose tolerance
  string unit_th("Gy"); //mask threshold (determines voxels to include in the gamma calc)

  float d_gamma       = gammaThresholds->threshDistanceMM;          
  float D_gamma_const = (gammaThresholds->threshDosePrcnt/100.0) * vector_util_max(D0.M);
  float thresh        = (gammaThresholds->threshDoseMinPrcnt/100.0) * vector_util_max(D1.M);  

  CString logFile = g_configInstance->m_directoryBase + "\\gammaLog.txt";
  logfile_open(logFile);

  //Matrix<float> gamma = tgif(x0, y0, z0, unit0, D0, unitD0,
  //  x0, y0, z0, unit1, D0, unitD1,
  //  d_gamma, unit_d, D_gamma, unit_D,
  //  thresh, unit_th, Nmax);

  m_gammaResults = { };
  Matrix<float> D_gamma;
  Matrix<float> gammaVolume;
  matrix_create(D_gamma, D0.nx, D0.ny, D0.nz, D_gamma_const);
  tgif(x0, y0, z0, unit0, D0, unitD0,
    x1, y1, z1, unit1, D1, unitD1,
    d_gamma, unit_d, D_gamma, unit_D,
    thresh, unit_th, Nmax, gammaVolume, &m_gammaResults);

  float bounds[2];
  bounds[0] = vector_util_min(gammaVolume.M);
  bounds[1] = vector_util_max(gammaVolume.M);

  SVoxGridGeometry* geom = &m_txtrGroupGammas->m_volumeGrid;
  geom->dims[0] = gammaVolume.nx;
  geom->dims[1] = gammaVolume.ny;
  geom->dims[2] = gammaVolume.nz;
  geom->numVoxels = gammaVolume.nx * gammaVolume.ny * gammaVolume.nz;
  if (geom->numVoxels == D0.M.size())
  {
    geom->voxelOrigin[0] = x0[0];
    geom->voxelOrigin[1] = y0[0];
    geom->voxelOrigin[2] = z0[0];
    geom->voxelSize[0] = gridDoseExt->voxelSize[0];
    geom->voxelSize[1] = gridDoseExt->voxelSize[1];
    geom->voxelSize[2] = gridDoseExt->voxelSize[2];
  }
  else
  {
    geom->voxelOrigin[0] = x1[0];
    geom->voxelOrigin[1] = y1[0];
    geom->voxelOrigin[2] = z1[0];
    geom->voxelSize[0] = gridDoseGMC->voxelSize[0];
    geom->voxelSize[1] = gridDoseGMC->voxelSize[1];
    geom->voxelSize[2] = gridDoseGMC->voxelSize[2];
  }

  geom->voxelVolume = geom->voxelSize[0] * geom->voxelSize[1] * geom->voxelSize[2];

  logfile_close();

  CDxTexture3D* txtr = m_txtrGroupGammas->Alloc(1);

  txtr->CreateTexture3D(geom->dims, gammaVolume.M.data(), bounds);
  txtr->CreateTextureRead(geom->dims, DXGI_FORMAT_R32_FLOAT);
  m_gammaResults.gammaMax = txtr->m_dataMax;

  return &m_gammaResults;
}

void CEnginePostCalc::CopyTextureData(CDxTexture3D* inputTexture, float* data)
{
  D3D11_MAPPED_SUBRESOURCE textureMap;
  D3D11_TEXTURE3D_DESC desc;

  inputTexture->m_texture->GetDesc(&desc);

  m_dxContext->CopyResource(inputTexture->m_textureRead, inputTexture->m_texture);
  m_dxContext->Map(inputTexture->m_textureRead, 0, D3D11_MAP_READ, 0, &textureMap);

  float* xEnd;
  float* yEnd;
  float* xPtr;
  float* yPtr;
  float* zPtr;
  float* zStart = (float*)textureMap.pData;
  float* zEnd = zStart + desc.Depth * (textureMap.DepthPitch / sizeof(float));
  int zStep = textureMap.DepthPitch / sizeof(float);
  int yStep = textureMap.RowPitch / sizeof(float);

  for (zPtr = zStart; zPtr < zEnd; zPtr += zStep)
  {
    yEnd = zPtr + desc.Height * yStep;

    for (yPtr = zPtr; yPtr < yEnd; yPtr += yStep)
    {
      xEnd = yPtr + desc.Width;

      for (xPtr = yPtr; xPtr < xEnd; ++xPtr)
      {
        *data =  *xPtr;
        ++data;
      }
    }
  }

  m_dxContext->Unmap(inputTexture->m_textureRead, 0);

  return;
}

void CEnginePostCalc::CalcDoseProfiles(void)
{

  CalcDoseProfile(0, m_transportParams->numProtonsTransport);
  //float numProtons;

  //numProtons = m_ionPlan->m_beams->controlPts[1].scanSpots[0].scanSpotWeight;

  //numProtons = m_transportResults->numShaderProtons;
  //numProtons *= m_transportResults->protonScaleFactor;

  //if (m_doseInfoPrimary)
  //{
  //  CalcDoseProfile(m_doseInfoPrimary, m_transportParams->;
  //}

  //if (m_doseInfoSecondary)
  //{

  //  //numProtons = 1.0e9; // MLW hard coded for spot dose

  //  CalcDoseProfile(m_doseInfoSecondary, numProtons);

  //}
  return;
}


//StcShaderProton* proton = protons;
//for (StcShaderControlPt* cntrlPt = m_ionPlan->shaderControlPts; cntrlPt < m_ionPlan->shaderControlPts + m_ionPlan->m_numControlPts; ++cntrlPt)
//{

//  fileName.Format("..\\Data\\CntrlPt%d.csv", cntrlPt - m_ionPlan->shaderControlPts);
//  fd = fopen(fileName, "w");
//  fprintf(fd, "X, Y\n");

//  StcShaderSpot* endSpot = m_ionPlan->shaderSpots + cntrlPt->spotIndex + cntrlPt->numScanSpots;
//  for (StcShaderSpot* spot = m_ionPlan->shaderSpots + cntrlPt->spotIndex; spot < endSpot; ++spot )
//  {

//    for (unsigned int k = 0; k < spot->numProtons; ++k, ++proton)
//    {
//      fprintf(fd, "%f, %f\n", proton->pos[0], proton->pos[1]);
//    }
//    //fprintf(fd, "%d\n", proton->pos[0]);
//    //if (proton->pos[1] != 0.0)
//    //{
//    //  ++count2;
//    //}
//    //++count;

//    //fprintf(fd, "%f, %f %f %f %f %f\n", proton->pos[0], proton->pos[1], proton->pos[2], proton->dir[0], proton->dir[1], proton->dir[2]);
//  }

//  fclose(fd);
//}

const float boundaryX = -221.0; // MLW Note: hard coded for water phantom
const float boundaryY = -187.0;

// MLW Note: Assumes proton travel along x axis
void CEnginePostCalc::CalcDoseProfile(int doseType, float numProtons)
{
  CString depthFileName;
  CString crossFileName;
  CString angleFileName;
  CString crossFileStart;
  FILE* fd;

  D3D11_MAPPED_SUBRESOURCE doseMap;

  m_dxContext->CopyResource(m_txtrGroupDosesPatient->GetTextureRead(doseType), m_txtrGroupDosesPatient->GetTexture(doseType));
  m_dxContext->Map(m_txtrGroupDosesPatient->GetTextureRead(doseType), 0, D3D11_MAP_READ, 0, &doseMap);

  int yStep;
  int zStep;
  float* xStart;

  xStart = (float*) doseMap.pData;

  yStep = doseMap.RowPitch / sizeof(float);
  zStep = doseMap.DepthPitch / sizeof(float);

  SVoxGridGeometry* grid = &m_txtrGroupDosesPatient->m_volumeGrid;

  float* xEnd = xStart + grid->dims[0];
  float* yEnd;
  float* zEnd;
  float* ptrX;
  float* ptrY;
  float* ptrZ;
  float* xMax;
  float startX;
  float startY;
  int numPos;

  float dose = 0;
  float* dosePath = new float[grid->dims[0]];
  memset(dosePath, 0, grid->dims[0] * sizeof(float));
  float dosePathMax = 0.0;
  float doseSliceMax = 0.0;

  float* angle = new float[grid->dims[0]];
  memset(angle, 0, grid->dims[0] * sizeof(float));
  float angleMax = 0;
  float rangeMaxAngle = 0;
  float threshWidth;

  float* dosePtr = dosePath;
  int yMax;
  int zMax;

  for (ptrX = xStart; ptrX < xEnd; ++ptrX)
  {
    yEnd = ptrX + grid->dims[1] * yStep;

    doseSliceMax = 0.0;
    for (ptrY = ptrX; ptrY < yEnd; ptrY += yStep)
    {
      zEnd = ptrY + grid->dims[2] * zStep;

      for (ptrZ = ptrY; ptrZ < zEnd; ptrZ += zStep)
      {
        *dosePtr += *ptrZ;

        if (*ptrZ > doseSliceMax)
        {
          doseSliceMax = *dosePtr;
          yMax = (ptrY - ptrX) / yStep;
          zMax = (ptrZ - ptrY) / zStep;
        }
      }
    }

    if (*dosePtr > dosePathMax)
    {
      dosePathMax = *dosePtr;
      xMax = ptrX;
    }

    TRACE("%3d %3d %3d %f\n", (ptrX - xStart), yMax, zMax, doseSliceMax);

    ++dosePtr;
  }

  depthFileName  = "DepthProf.txt";
  crossFileStart = "CrossProf";
  angleFileName  = "Angle.txt";

  float doseScale = (float)(grid->voxelVolume * 1.0e9) / numProtons / grid->voxelSize[0]; // MLW dbg check

  startX = grid->voxelOrigin[0] - boundaryX;
  startY = grid->voxelOrigin[1] - boundaryY;

  fd = m_fileIO->OpenFile(FILE_DEST_DATA, depthFileName, "w");
  for (dosePtr = dosePath; dosePtr < dosePath + grid->dims[0] - 1; ++dosePtr)
  {
    fprintf(fd, "%f %f %f\n", startX + (dosePtr - dosePath) * grid->voxelSize[0],
      *dosePtr * doseScale, *dosePtr / dosePathMax);
  }
  m_fileIO->CloseFile();

  float* crossPath = new float[grid->dims[1]];

  float* angValue = angle;
  float* crossPtr;
  float crossMax = 0;
  int crossMaxIndex;

  for (ptrX = xStart; ptrX < xEnd; ++ptrX)
  {
    memset(crossPath, 0, grid->dims[1] * sizeof(float));
    crossMax = 0;

    crossPtr = crossPath;

    yEnd = ptrX + grid->dims[1] * yStep;

    for (ptrY = ptrX; ptrY < yEnd; ptrY += yStep)
    {
      zEnd = ptrY + grid->dims[2] * zStep;

      for (ptrZ = ptrY; ptrZ < zEnd; ptrZ += zStep)
      {
        *crossPtr += *ptrZ;
      }

      if (*crossPtr > crossMax)
      {
        crossMax = *crossPtr;
        crossMaxIndex = (ptrY - ptrX) / yStep;
      }

      ++crossPtr;
    }

    startX = grid->voxelOrigin[0];
    startY = grid->voxelOrigin[1];

    crossFileName.Format("%s%03d.txt", crossFileStart, (ptrX - xStart));
    fd = m_fileIO->OpenFile(FILE_DEST_DATA, crossFileName, "w");
    numPos = 0;
    for (crossPtr = crossPath; crossPtr < crossPath + grid->dims[1] - 1; ++crossPtr)
    {
      if (*crossPtr > 0.0)
      {
        fprintf(fd, "%f %f %f\n", startY + ((crossPtr - crossPath)) * grid->voxelSize[1],
          *crossPtr, *crossPtr / crossMax);
        ++numPos;
      }
    }
    fclose(fd);

    float width = 0.0;
    if (crossMax > 0.0)
    {

    threshWidth = (float)(crossMax * exp(-1.0));
    crossPtr = crossPath + crossMaxIndex;

    do
    {
      if (*(crossPtr + 1) < threshWidth)
      {
        width += grid->voxelSize[1] * (*crossPtr - threshWidth) / (*crossPtr - *(crossPtr + 1));
        break;
      }
      width += grid->voxelSize[1];
      ++crossPtr;
    } while (*crossPtr > 0.0);

    crossPtr = crossPath + crossMaxIndex;
    do
    {
      if (*(crossPtr - 1) < threshWidth)
      {
        width += grid->voxelSize[1] * (*crossPtr - threshWidth) / (*crossPtr - *(crossPtr - 1));
        break;
      }
      width += grid->voxelSize[1];
      ++crossPtr;
    } while (*crossPtr > 0.0);
    
    }

    *angValue = width;
    if (*angValue > angleMax)
    {
      angleMax = *angValue;
      rangeMaxAngle = (ptrX - xStart) * grid->voxelSize[0];
    }

    ++angValue;
  }

  fd = m_fileIO->OpenFile(FILE_DEST_DATA, angleFileName, "w");
  ptrX = xStart;
  for (angValue = angle; angValue < angle + grid->dims[0]; ++angValue)
  {
    fprintf(fd, " %f, %f, %f\n", startX + (ptrX - xStart) * grid->voxelSize[0]/rangeMaxAngle, *angValue, *angValue/angleMax);
    ++ptrX;
  }
  m_fileIO->CloseFile();

  m_dxContext->Unmap(m_txtrGroupDosesPatient->GetTextureRead(DOSE_TYPE_DOSE), 0);

  delete[] crossPath;
  delete[] dosePath;
  delete[] angle;

  return;
}

//bool CEnginePostCalc::ResampleDose(void)
//{
// CDxShaderCompute resampler3DCS;
//  resampler3DCS.BuildShader(g_resampler3DCS, sizeof(g_resampler3DCS));
// 
//  SVoxGridGeometry* destGrid   = m_externalDoseInfo.m_volumeGrid;
//  SVoxGridGeometry* sourceGrid = m_doseInfo.m_volumeGrid;
//
//  SResamplerParams params = {};
//
//  params.destNumVox[0]      = destGrid->dims[0];
//  params.destNumVox[1]      = destGrid->dims[1];
//  params.destNumVox[2]      = destGrid->dims[2];
//  params.destVoxOrigin[0]   = destGrid->voxelOrigin[0];
//  params.destVoxOrigin[1]   = destGrid->voxelOrigin[1];
//  params.destVoxOrigin[2]   = destGrid->voxelOrigin[2];
//  params.destVoxSize[0]     = destGrid->voxelSize[0];
//  params.destVoxSize[1]     = destGrid->voxelSize[1];
//  params.destVoxSize[2]     = destGrid->voxelSize[2];
//  params.destNumVoxSlice    = destGrid->dims[0] * destGrid->dims[1];
//  params.destNumVoxVolume   = destGrid->numVoxels;
//  params.sourceVolOrigin[0] = sourceGrid->voxelOrigin[0] - sourceGrid->voxelSize[0] / 2;
//  params.sourceVolOrigin[1] = sourceGrid->voxelOrigin[1] - sourceGrid->voxelSize[1] / 2;
//  params.sourceVolOrigin[2] = sourceGrid->voxelOrigin[2] - sourceGrid->voxelSize[2] / 2;
//  params.sourceVolSize[0]   = sourceGrid->dims[0] * sourceGrid->voxelSize[0];
//  params.sourceVolSize[1]   = sourceGrid->dims[1] * sourceGrid->voxelSize[1];
//  params.sourceVolSize[2]   = sourceGrid->dims[2] * sourceGrid->voxelSize[2];
//
//  D3D11_BUFFER_DESC bufferDesc;
//  ZeroMemory(&bufferDesc, sizeof(D3D11_BUFFER_DESC));
//
//  bufferDesc.Usage          = D3D11_USAGE_DYNAMIC;
//  bufferDesc.ByteWidth      = sizeof(SResamplerParams);
//  bufferDesc.BindFlags      = D3D11_BIND_CONSTANT_BUFFER;
//  bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
//
//  D3D11_SUBRESOURCE_DATA resData = {};
//  resData.pSysMem = &params;
//
//  if (FAILED(m_dxDevice->CreateBuffer(&bufferDesc, &resData, &m_resampledDoseInfo.paramsCBuffer)))
//  {
//    return false;
//  }
//
//  SVoxGridGeometry* doseGrid = m_externalDoseInfo.m_volumeGrid;
//
//  D3D11_TEXTURE3D_DESC txtrDesc;
//  ZeroMemory(&txtrDesc, sizeof(D3D11_TEXTURE3D_DESC));
//
//  txtrDesc.Width          = doseGrid->dims[0];
//  txtrDesc.Height         = doseGrid->dims[1];
//  txtrDesc.Depth          = doseGrid->dims[2];
//  txtrDesc.MipLevels      = 1;
//  txtrDesc.Format         = DXGI_FORMAT_R32_FLOAT;
//  txtrDesc.Usage          = D3D11_USAGE_DEFAULT;
//  txtrDesc.BindFlags      = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
//  txtrDesc.MiscFlags      = 0;
//
//  if (FAILED(m_dxDevice->CreateTexture3D(&txtrDesc, NULL, &m_resampledDoseInfo.texture)))
//  {
//    return false;
//  }
//
//  D3D11_UNORDERED_ACCESS_VIEW_DESC  uaViewDesc = {};
//  uaViewDesc.Format                = DXGI_FORMAT_R32_FLOAT;
//  uaViewDesc.ViewDimension         = D3D11_UAV_DIMENSION_TEXTURE3D;
//  uaViewDesc.Texture3D.MipSlice    = 0;
//  uaViewDesc.Texture3D.FirstWSlice = 0;
//  uaViewDesc.Texture3D.WSize       = doseGrid->dims[2];
//
//  if (FAILED(m_dxDevice->CreateUnorderedAccessView(m_resampledDoseInfo.texture, &uaViewDesc, &m_resampledDoseInfo.viewAV)))
//  {
//    return false;
//  }
//
//  ZeroMemory(&txtrDesc, sizeof(D3D11_TEXTURE3D_DESC));
//
//  txtrDesc.Width          = doseGrid->dims[0];
//  txtrDesc.Height         = doseGrid->dims[1];
//  txtrDesc.Depth          = doseGrid->dims[2];
//  txtrDesc.MipLevels      = 1;
//  txtrDesc.Format         = DXGI_FORMAT_R32_FLOAT;
//  txtrDesc.Usage          = D3D11_USAGE_STAGING;
//  txtrDesc.BindFlags      = 0;
//  txtrDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
//  txtrDesc.MiscFlags      = 0;
//
//  if (FAILED(m_dxDevice->CreateTexture3D(&txtrDesc, NULL, &m_resampledDoseInfo.readTexture)))
//  {
//    return false;
//  }
//
//  // Create the sampler
//  D3D11_SAMPLER_DESC sampDesc;
//  ZeroMemory(&sampDesc, sizeof(sampDesc));
//  sampDesc.Filter         = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
//  sampDesc.AddressU       = D3D11_TEXTURE_ADDRESS_CLAMP;
//  sampDesc.AddressV       = D3D11_TEXTURE_ADDRESS_CLAMP;
//  sampDesc.AddressW       = D3D11_TEXTURE_ADDRESS_CLAMP;
//  sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
//  sampDesc.MinLOD         = 0;
//  sampDesc.MaxLOD         = D3D11_FLOAT32_MAX;
//
//  if (FAILED(m_dxDevice->CreateSamplerState(&sampDesc, &m_textureResampler)))
//  {
//    return false;
//  }
//
//  D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
//  ZeroMemory(&srvDesc, sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC));
//
//  srvDesc.Format              = txtrDesc.Format;
//  srvDesc.ViewDimension       = D3D11_SRV_DIMENSION_TEXTURE3D;
//  srvDesc.Texture3D.MipLevels = 1;
//
//  if (FAILED(m_dxDevice->CreateShaderResourceView(m_resampledDoseInfo.texture, &srvDesc, &m_resampledDoseInfo.view))) // MLW view destroryed?
//  {
//    return false;
//  }
//
//  m_dxContext->CSSetConstantBuffers(0, 1, &m_resampledDoseInfo.paramsCBuffer);
//  m_dxContext->CSSetShaderResources(0, 1, &m_doseInfo.view);
//  m_dxContext->CSSetUnorderedAccessViews(0, 1, &m_resampledDoseInfo.viewAV, (UINT*)(&m_resampledDoseInfo.viewAV));
//  m_dxContext->CSSetSamplers(0, 1, &m_textureSampler);
//  m_dxContext->CSSetShader(m_resampler3DCS, NULL, 0);
//
//  int numGroups = (destGrid->numVoxels / 1024) + 1;
//
//  m_dxContext->Dispatch(numGroups, 1, 1);
//
//  m_dxContext->CSSetUnorderedAccessViews(0, 1, &m_nullViewAV, (UINT*)(&m_nullViewAV));
//
//  D3D11_MAPPED_SUBRESOURCE mapResource;
//
//  m_dxContext->CopyResource(m_resampledDoseInfo.readTexture, m_resampledDoseInfo.texture);
//  m_dxContext->Map(m_resampledDoseInfo.readTexture, 0, D3D11_MAP_READ, 0, &mapResource);
//
//  float* xEnd;
//  float* yEnd;
//  float* xPtr;
//  float* yPtr;
//  float* zPtr;
//  float* zStart = (float*)mapResource.pData;
//  float* zEnd   = zStart + doseGrid->dims[2] * (mapResource.DepthPitch / sizeof(float));
//  int zStep = mapResource.DepthPitch / sizeof(float);
//  int yStep = mapResource.RowPitch / sizeof(float);
//  float max = *zStart;
//  float min = *zStart;
//  float doseSum = 0.0;
//  int numDosePos = 0;
//
//  for (zPtr = zStart; zPtr < zEnd; zPtr += zStep)
//  {
//    yEnd = zPtr + doseGrid->dims[1] * yStep;
//
//    for (yPtr = zPtr; yPtr < yEnd; yPtr += yStep)
//    {
//      xEnd = yPtr + doseGrid->dims[0];
//
//      for (xPtr = yPtr; xPtr < xEnd; ++xPtr)
//      {
//
//        if (*xPtr > 0.0)
//        {
//          ++numDosePos;
//        }
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
//
//      }
//    }
//  }
//
//  doseGrid->doseMax = max;
//  doseGrid->doseMin = min;
//
//  TRACE("DoseResample %f %f %e %d\n", min, max, doseSum, numDosePos);
//
//  int doseBins[NUM_DOSE_BINS];
//  int binNum;
//  int numPos = 0;
//
//  memset(doseBins, 0, NUM_DOSE_BINS * sizeof(int));
//
//  for (zPtr = zStart; zPtr < zEnd; zPtr += zStep)
//  {
//    yEnd = zPtr + destGrid->dims[1] * yStep;
//
//    for (yPtr = zPtr; yPtr < yEnd; yPtr += yStep)
//    {
//      xEnd = yPtr + destGrid->dims[0];
//
//      for (xPtr = yPtr; xPtr < xEnd; ++xPtr)
//      {
//
//        binNum = (int)((*xPtr / m_doseInfo.m_volumeGrid->doseMax) * NUM_DOSE_BINS);
//        if(binNum > -1)
//        {
//          ++doseBins[binNum];
//        }
//        ++numPos;
//      }
//    }
//  }
//
//  FILE* fd = fopen("..\\Data\\HistResampled.txt", "w");
//
//  for (int bin = 0; bin < NUM_DOSE_BINS; ++bin)
//  {
//    fprintf(fd, "%3d %e\n", bin, (float)doseBins[bin] / numPos);
//  }
//
//  fclose(fd);
//
//  m_dxContext->Unmap(m_resampledDoseInfo.readTexture, 0);
//
//  m_resampledDoseInfo.m_volumeGrid = doseGrid;
//
//  OverlayDoseGrid(&m_resampledDoseInfo);
//
//  m_doseInfoSecondary = &m_resampledDoseInfo;
//
//  return true;
//}

