#include "stdafx.h"
#include "D3D11.h"
#include "DirectXMath.h"
#include "Logger.h"
#include "SpotPrepCS.hxx"
#include "SpotPrepSOBPCS.hxx"
#include "TransportDevicesCS.hxx"
#include "TransportPatientCS.hxx"
//#include "PatientMFPCS.hxx"
#include "DoseStatsCS.hxx"
#include "FileIO.h"
#include "IonPlan.h"
#include "PhantomQA.h"
#include "DicomDose.h"
#include "DeviceMaterials.h"
#include "ShaderRandom.h"
#include "ShaderPlan.h"
#include "Transport.h"
#include "EngineTransport.h"

#define NUM_BIN_VOX  (3100)

CEngineTransport::CEngineTransport(void) :
  m_ionPlan               (NULL),
  m_phantom               (NULL),
  m_numProtonsCalc        (0),
  m_numProtonsPerDispatch (0),
  m_dispatchTransport     ({})
{

  m_buffRWProtonsAuxBase = &m_buffRWProtonsAux;

  return;
}

CEngineTransport::~CEngineTransport(void)
{

  ReleaseShaderResources();

  return;
}

void CEngineTransport::ReleaseShaderResources(void)
{

  if (m_dxContext) 
  {
    m_dxContext->ClearState();
  }

  return;
}

void CEngineTransport::ClearResourcesPlan(void)
{
  // Constant Buffers
  // Constant buffers not cleared, same size for all patient plans

  // Structured Buffers
  m_beamsStructRes.Release();
  m_controlPtsStructRes.Release();
  m_spotsStructRes.Release();
  m_bufferSpotScales.Release();
  m_materialsStructRes.Release();
  m_devicesStructRes.Release();
  m_deviceIndexesRes.Release();
  m_aperturesStructRes.Release();
  m_aperturePtsStructRes.Release();
  m_compensatorsStructRes.Release();
  m_compensatorDepthsStructRes.Release();
  m_sobpGridsRes.Release();
  m_bufferRWsobpHalos.Release();
  m_depthDoseStructRes.Release();
  m_depthLETStructRes.Release();
  m_alphaStructRes.Release();

  forPrray(m_txtrGroupImagesPatient, texture)
  {
    texture->Release();
  }

  forPrray(m_txtrGroupImagesPhantom, texture)
  {
    texture->Release();
  }

  return;
}

void CEngineTransport::ClearResourcesProtons(void)
{

  m_buffRWProtonsPos.Release();
  m_buffRWProtonsDir.Release();
  m_buffRWProtonsAux.Release();

  return;
}

void CEngineTransport::ClearResourcesResults(void)
{

  forPrray(m_txtrGroupDosesPatient, texture)
  {
    texture->Release();
  }
  // Need to clear Patient dose because auxDose can be set or not
  m_txtrGroupDosesPatient->Clear();

  forPrray(m_txtrGroupDosesQA, texture)
  {
    texture->Release();
  }
  // Need to clear QA beams Array because number varies across plans & across run for QA set or not
  m_txtrGroupDosesQA->Clear();

  forPrray(m_txtrGroupDosesStats, texture)
  {
    texture->Release();
  }

  m_txtrNumProtonsVox->Release();

  return;
}

void CEngineTransport::ClearResourcesDij(void)
{

  m_dijSpots.Release();
  m_dijBins.Release();
  m_dijVoxels.Release();

  m_arrayDijSpots.Clear();

  m_paramsDij = {};

  return;
}

bool CEngineTransport::CreateShaders(void)
{

  ReturnOnFalse(m_cShaderSpotPrep.BuildShader(g_spotPrepCS, sizeof(g_spotPrepCS)));
  ReturnOnFalse(m_cShaderSpotPrepSOBP.BuildShader(g_spotPrepSOBPCS, sizeof(g_spotPrepSOBPCS)));

  ReturnOnFalse(m_cShaderDevices.BuildShader(g_transportDevicesCS, sizeof(g_transportDevicesCS)));

  ReturnOnFalse(m_cShaderPatient.BuildShader(g_transportPatientCS, sizeof(g_transportPatientCS)));
  //ReturnOnFalse(m_cShaderPatient.BuildShader(g_patientMFPCS, sizeof(g_patientMFPCS)));

  //ReturnOnFalse(m_cShaderDoseStats.BuildShader(g_doseStatsCS, sizeof(g_doseStatsCS)));

  return true;
}

bool CEngineTransport::CreatePersistentResources(void)
{
  CUniArray<unsigned int> seeds(NUM_GROUP_THREADS * 4);

  INITSEEDS128(123456, 234567, 3456789, 4567890);

  forArray(seeds, seedItem)
  {
    NEWSEED128(*seedItem);
  }

  m_bufferRWrandSeeds.CreateBuffRWStruct(NUM_GROUP_THREADS, 4 * sizeof(unsigned int), seeds(0));

  m_dispatchCBuffer.CreateConstantBufferDyn(sizeof(SDispatchTransport));
  m_calcGridCBuffer.CreateConstantBufferDyn(sizeof(SShaderCalcGrid));

  m_txtrGroupImagesPatient->Alloc(NUM_IMG_TYPES);
  m_txtrGroupImagesPhantom->Alloc(NUM_IMG_TYPES);

  return true;
}

bool CEngineTransport::LoadRTIonPlan(CIonPlan* plan)
{

  ClearResourcesPlan();

  m_ionPlan = plan;

  if (CreatePlanBuffers() == false)
  {
    APPLOG("FAIL: CreatePlanBuffers\n");
    return false;
  }

  LoadImageGridParams(&plan->m_voxGridRSP);

  m_txtrGroupDosesPatient->m_volumeGrid  = plan->m_voxGridRSP;
  m_txtrGroupImagesPatient->m_volumeGrid = plan->m_voxGridRSP;

  SDataGrid* dataGrid;
  CDxTexture3D* txtr;

  dataGrid = &plan->m_voxGridRSP;
  txtr = m_txtrGroupImagesPatient->ItemPtr(IMG_TYPE_RSP);
  txtr->CreateTexture3D(dataGrid->dims, dataGrid->data, dataGrid->bounds);

  dataGrid = &plan->m_voxGridHU;
  txtr = m_txtrGroupImagesPatient->ItemPtr(IMG_TYPE_HU);
  txtr->CreateTexture3D(dataGrid->dims, dataGrid->data, dataGrid->bounds);

  return true;
}

bool CEngineTransport::LoadQAPhantom(CPhantomQA* phantom)
{

  m_phantom = phantom;

  LoadImageGridParams(&phantom->m_voxGridRSP);

  SDataGrid* gridRSP = &phantom->m_voxGridRSP;
  SDataGrid* gridHU = &phantom->m_voxGridHU;

  CDxTexture3D* txtr;
  txtr = m_txtrGroupImagesPhantom->ItemPtr(IMG_TYPE_RSP);
  txtr->CreateTexture3D(gridRSP->dims, gridRSP->data);

  txtr = m_txtrGroupImagesPhantom->ItemPtr(IMG_TYPE_HU);
  txtr->CreateTexture3D(gridHU->dims, gridHU->data);

  m_txtrGroupDosesQA->Alloc(m_ionPlan->m_arrayBeams.m_numItems);

  forPrray(m_txtrGroupDosesQA, txtrItem)
  {
    txtrItem->CreateRWTextureDual3D(gridRSP->dims, DXGI_FORMAT_R32_UINT, DXGI_FORMAT_R32_FLOAT);
  }

  return true;
}

bool CEngineTransport::LoadImageGridParams(SVoxGridGeometry* grid)
{

  SShaderCalcGrid params;

  params.orientation = (m_ionPlan->m_flagProne ? -1.0F : 1.0F);

  params.dims[0] = grid->dims[0];
  params.dims[1] = grid->dims[1];
  params.dims[2] = grid->dims[2];
  params.numSliceVoxels = grid->dims[0] * grid->dims[1];

  params.voxSize[0]  = grid->voxelSize[0];
  params.voxSize[1]  = grid->voxelSize[1];
  params.voxSize[2]  = grid->voxelSize[2];
  params.voxelVolume = grid->voxelVolume;

  params.volOrigin[0] = grid->voxelOrigin[0] - params.orientation * grid->voxelSize[0] / 2;
  params.volOrigin[1] = grid->voxelOrigin[1] - params.orientation * grid->voxelSize[1] / 2;
  params.volOrigin[2] = grid->voxelOrigin[2] - grid->voxelSize[2] / 2;
  params.volSize[0]   = grid->dims[0] * grid->voxelSize[0];
  params.volSize[1]   = grid->dims[1] * grid->voxelSize[1];
  params.volSize[2]   = grid->dims[2] * grid->voxelSize[2];

  m_calcGridCBuffer.Upload(&params);

  return true;
}

bool CEngineTransport::CreatePlanBuffers(void)
{

  m_beamsStructRes.CreateStructuredBufferFixed(m_ionPlan->m_arrayBeams);

  m_controlPtsStructRes.CreateStructuredBufferFixed(m_ionPlan->m_arrayControlPts);
  
  m_spotsStructRes.CreateStructuredBufferFixed(m_ionPlan->m_arraySpots);

  if (m_ionPlan->m_arrayDevices.m_numItems)
  {
    m_devicesStructRes.CreateStructuredBufferFixed(m_ionPlan->m_arrayDevices);

    m_deviceIndexesRes.CreateResourceBufferFixed(m_ionPlan->m_arrayDeviceIndexes, DXGI_FORMAT_R32_SINT);

    m_materialsStructRes.CreateStructuredBufferFixed(NUM_MATERIALS, sizeof(SDeviceMaterial), materialList);

    if (m_ionPlan->m_arrayApertures.m_numItems)
    {
      m_aperturesStructRes.CreateStructuredBufferFixed(m_ionPlan->m_arrayApertures);
      m_aperturePtsStructRes.CreateStructuredBufferFixed(m_ionPlan->m_arrayAperturePts);
    }

    if (m_ionPlan->m_arrayCompensators.m_numItems)
    {
      m_compensatorsStructRes.CreateStructuredBufferFixed(m_ionPlan->m_arrayCompensators);
      m_compensatorDepthsStructRes.CreateResourceBufferFixed(m_ionPlan->m_arrayCompensatorDepths, DXGI_FORMAT_R32_FLOAT);
    }
  }

  if (m_ionPlan->m_arraySOBPGrids.m_numItems)
  {
    m_sobpGridsRes.CreateStructuredBufferFixed(m_ionPlan->m_arraySOBPGrids);
  }

  m_depthDoseStructRes.CreateResourceBufferFixed(m_ionPlan->m_arrayDepthDoses);

  m_alphaStructRes.CreateResourceBufferFixed(m_ionPlan->m_arrayAlphas);

  m_depthLETStructRes.CreateResourceBufferFixed(m_ionPlan->m_arrayLETs);

  return true;
}

bool CEngineTransport::PrepCompute(STransportParams* transportParams)
{
#ifdef DBG_0 // Modify proton scale factor-
  transportParams->protonScaleFactor *= 10000;
#endif

  ClearResourcesProtons();
  ClearResourcesResults();

  int* gridDims = m_ionPlan->m_voxGridRSP.dims;

  if (transportParams->doseAux)
  {
    m_txtrGroupDosesPatient->Alloc(NUM_DOSE_TYPES);

    m_txtrNumProtonsVox->CreateRWTexture3D(gridDims, DXGI_FORMAT_R32_UINT);
    m_txtrNumProtonsVox->CreateResView(DXGI_FORMAT_R32_FLOAT);
    m_txtrNumProtonsVox->CreateTextureRead(gridDims, DXGI_FORMAT_R32_FLOAT);
  }
  else
  {
    m_txtrGroupDosesPatient->Alloc(1);
  }

  // Create dose result textures
  forPrray(m_txtrGroupDosesPatient, doseTxtr)
  {
    doseTxtr->CreateRWTextureDual3D(gridDims, DXGI_FORMAT_R32_UINT, DXGI_FORMAT_R32_FLOAT);
  }

  ReturnOnFalse(PrepSpotProtons(transportParams));

  ReturnOnFalse(CreateProtonBuffers());

  if (transportParams->dijCalc)
  {
    ReturnOnFalse(PrepDIJ());
  }

  SetTransportViews(transportParams);

#ifdef DBG_0
  CalcObjectsInfo();
#endif

  return true;
}

bool CEngineTransport::PrepSpotProtons(STransportParams* transportParams)
{
  SShaderBeam* beam;
  SShaderControlPt* controlPt;
  unsigned int seed = m_ionPlan->m_arraySpots.m_numItems;
  float energyPlan = 0.0;

  m_numProtonsCalc = 0;

  m_arraySpotCalc.Alloc(m_ionPlan->m_arraySpots.m_numItems);

  SShaderSpotCalc* spotScale = m_arraySpotCalc(0);
  forArray(m_ionPlan->m_arraySpots, spot)
  {
    controlPt = m_ionPlan->m_arrayControlPts(spot->indexControlPt);

    beam = m_ionPlan->m_arrayBeams(controlPt->indexBeam);

    spotScale->randSeed = seed;

    if (spot->weight == 0.0)
    {
      spotScale->numProtonsCompute = 0;
      spotScale->doseScale = 0.0;
    }
    else
    {

      if (transportParams->spotProtonsConstant)
      {
        spotScale->numProtonsCompute = transportParams->protonScaleFactor;
      }
      else
      {
        spotScale->numProtonsCompute = 
          unsigned int(NUM_GROUP_THREADS * ceil((spot->weight / transportParams->protonScaleFactor) / NUM_GROUP_THREADS));
      }

      spotScale->indexProtonsCompute = m_numProtonsCalc;
      m_numProtonsCalc += spotScale->numProtonsCompute;

      seed += spotScale->numProtonsCompute;

      spotScale->doseScaleQA = (spot->weight / ((float)spotScale->numProtonsCompute * transportParams->numCycles));

      spotScale->doseScale = spotScale->doseScaleQA * beam->numFractions;

    }

    ++spotScale;

#ifdef DBG_0 // Limit plan energy calc
    if (controlPt->indexBeam == 0)
      //if(spot == m_ionPlan->m_arraySpots.m_pItemFirst) // DBG_0
#endif
    energyPlan += spot->weight * m_ionPlan->m_arrayDepthDoses[controlPt->indexDepthDoses + controlPt->numDepthDoses - 1] * beam->numFractions;

  }

  transportParams->energyPlan = energyPlan;
  transportParams->numProtonsTransport = m_numProtonsCalc;

  APPLOG("NumProtonsPerCycle %d\n", m_numProtonsCalc);

  UINT64 numProtonBytes = m_numProtonsCalc * sizeof(float[3]);
  if (numProtonBytes > m_dxNumBufferBytesMax)
  {
    APPLOG("Requested proton buffer too large. %Iu %Iu %f\n", numProtonBytes, m_dxNumBufferBytesMax, ((float) numProtonBytes)/ m_numProtonsCalc);

    return false;
  }

  m_numProtonsPerDispatch = unsigned int(NUM_GROUP_THREADS * ceil(float(m_numProtonsCalc) / NUM_GROUP_THREADS));

  // DBG_0
  //DebugRand();

#ifdef DBG_0
  TestRand();
#endif

  return true;
}

bool CEngineTransport::CreateProtonBuffers(void)
{
  m_bufferSpotScales.CreateStructuredBufferFixed(m_arraySpotCalc);

  m_buffRWProtonsPos.CreateBuffRWStruct(m_numProtonsCalc, 3*sizeof(float), NULL);
  m_buffRWProtonsPos.CreateBufferRead();
  m_buffRWProtonsDir.CreateBuffRWStruct(m_numProtonsCalc, 3*sizeof(float), NULL);
  m_buffRWProtonsDir.CreateBufferRead();
  m_buffRWProtonsAux.CreateBuffRWStruct(m_numProtonsCalc, sizeof(SShaderProtonsAux), NULL);
  m_buffRWProtonsAux.CreateBufferRead();

  if (m_ionPlan->m_flagSOBP)
  {
    m_bufferRWsobpHalos.CreateBuffRWTyped(m_numProtonsCalc, DXGI_FORMAT_R32_FLOAT, NULL);
  }

  ID3D11Buffer* buffers[] = { m_dispatchCBuffer.m_buffer };

  ID3D11UnorderedAccessView* uavViews[] =
  {
    m_bufferRWrandSeeds.m_viewRW,
    m_buffRWProtonsPos.m_viewRW,
    m_buffRWProtonsDir.m_viewRW,
    m_buffRWProtonsAux.m_viewRW,
    m_bufferRWsobpHalos.m_viewRW
  };

  ID3D11ShaderResourceView* views[] =
  {
    m_beamsStructRes.m_view,
    m_controlPtsStructRes.m_view,
    m_spotsStructRes.m_view,
    m_bufferSpotScales.m_view,
    m_sobpGridsRes.m_view
  };

  if (m_ionPlan->m_flagSOBP)
  {
    m_cShaderSpotPrepSOBP.SetConstantBuffers(buffers, sizeof(buffers));
    m_cShaderSpotPrepSOBP.SetResourceViews(views, sizeof(views));
    m_cShaderSpotPrepSOBP.SetUAVViews(uavViews, sizeof(uavViews));
  }
  else
  {
    m_cShaderSpotPrep.SetConstantBuffers(buffers, sizeof(buffers));
    m_cShaderSpotPrep.SetResourceViews(views, sizeof(views));
    m_cShaderSpotPrep.SetUAVViews(uavViews, sizeof(uavViews));
  }

  return true;
}

bool CEngineTransport::PrepDIJ(void)
{
  float dirPtnt[3];
  float depth;
  float dirMax;
  int   dirAxis;
  unsigned int numBinsSpot;
  unsigned int numDoseBin;

  ClearResourcesDij();

  float* voxSize = m_ionPlan->m_voxGridRSP.voxelSize;

  XMVECTOR dirLinac;
  XMVECTOR dirBeam = XMVectorSet( 0.0, 0.0, -1.0, 0.0 );
  XMMATRIX xform;
  XMFLOAT3 tmp;

  m_arrayDijSpots.Alloc(m_ionPlan->m_arraySpots.m_numItems);

  int spotCurrentStart = 0;
  unsigned int maxBinsDepth = 0;
  forArray(m_ionPlan->m_arrayControlPts, controlPt)
  {
    depth = controlPt->maxDepthDoseDepth;

    xform = XMLoadFloat3x3(&controlPt->beamMatrix);

    dirLinac = XMVector3Transform(dirBeam, XMMatrixTranspose(xform));

    XMStoreFloat3(&tmp, dirLinac);
    
    dirPtnt[0] =  tmp.x;
    dirPtnt[1] = -tmp.z;
    dirPtnt[2] =  tmp.y;

    dirMax = fabs(dirPtnt[0]);
    dirAxis = 0;
    if (dirMax < fabs(dirPtnt[1]))
    {
      dirMax = fabs(dirPtnt[1]);
      dirAxis = 1;
    }
    if (dirMax < fabs(dirPtnt[2]))
    {
      dirMax = fabs(dirPtnt[2]);
      dirAxis = 2;
    }

    // All spots in control point have same number of bins (based on depth dose)
    numBinsSpot = (unsigned int)(depth / voxSize[dirAxis] + 0.5) + 1;
    if (numBinsSpot > maxBinsDepth)
    {
      maxBinsDepth = numBinsSpot;
    }

    forArrayIN(m_arrayDijSpots, spot, spotCurrentStart, controlPt->numSpots)
    {
      spot->numBins     = numBinsSpot;
      spot->indexBin    = m_paramsDij.numBins;
      spot->mainDirAxis = dirAxis;

      m_paramsDij.numBins += numBinsSpot;
    }

    //numDoseBin = NUM_BIN_VOX * (controlPt->spotSize[0] * controlPt->spotSize[1] / 100.0) * (m_ionPlan->m_voxGridRSP.voxelVolume / 8.0);
    numDoseBin = NUM_BIN_VOX; // MLW check
    m_paramsDij.numVoxels += controlPt->numSpots * numBinsSpot * numDoseBin;

    spotCurrentStart += controlPt->numSpots;
  }

  m_arrayDijBins.Alloc(m_paramsDij.numBins);

  unsigned int indexVoxel = 0;

  forArray(m_arrayDijSpots, spot)
  {
    forArrayIN(m_arrayDijBins, bin, spot->indexBin, spot->numBins)
    {
      bin->indexVoxel = indexVoxel;
      bin->numVoxels  = numDoseBin;
      indexVoxel     += bin->numVoxels;
    }
  } 

  TRACE("DIJ PARAMS %u %u %u\n", maxBinsDepth, m_paramsDij.numBins, m_paramsDij.numVoxels);

  UINT64 numDijBinBytes = m_paramsDij.numBins * sizeof(SShaderDijBin);
  if (numDijBinBytes > m_dxNumBufferBytesMax)
  {
    APPLOG("Requested Dij bin buffer too large. %Iu %Iu\n", numDijBinBytes, m_dxNumBufferBytesMax);
  }

  m_dijSpots.CreateStructuredBufferFixed(m_ionPlan->m_arraySpots.m_numItems, sizeof(SShaderDijSpot), m_arrayDijSpots(0));
  m_dijBins.CreateStructuredBufferFixed(m_paramsDij.numBins, sizeof(SShaderDijBin), m_arrayDijBins(0));

  m_dijVoxels.CreateBuffRWStruct(m_paramsDij.numVoxels, sizeof(SShaderDijVoxel));
  m_dijVoxels.CreateBufferRead();

#ifdef DBG_0 // DijSpots
  m_dijBinFill.CreateBuffRWTyped(m_paramsDij.numBins, DXGI_FORMAT_R32_UINT);
  m_dijBinFill.CreateBufferRead();
#endif

  return true;
}

void CEngineTransport::SetTransportViews(STransportParams* transportParams)
{

  memset(&m_dispatchTransport, 0, sizeof(m_dispatchTransport));

  if (transportParams->statsCalc)
  {
    m_dispatchStats = {};

    int* dims = m_ionPlan->m_voxGridRSP.dims;

    //m_transportResults.dosesStatistics.txtrArray  = m_arrayTxtrDoseStatistics;
    //m_transportResults.dosesStatistics.m_volumeGrid = &m_ionPlan->m_voxGridRSP;

    m_txtrGroupDosesStats->m_volumeGrid = m_ionPlan->m_voxGridRSP;

    m_dispatchStats.numCyclesTotal = transportParams->numCycles;

    m_dispatchStats.numVoxSlice = dims[0] * dims[1];
    m_dispatchStats.numVoxRow = dims[0];

  }
  else if (transportParams->dijCalc)
  {
    m_dispatchTransport.flagDij = 1;
  }
  else if (transportParams->doseAux)
  {
    m_dispatchTransport.flagLET = 1;
  }

  ID3D11Buffer* buffers[] = { m_dispatchCBuffer.m_buffer };
  m_cShaderDevices.SetConstantBuffers(buffers, sizeof(buffers));

  ID3D11ShaderResourceView* views[] =
  {
    m_beamsStructRes.m_view,
    m_controlPtsStructRes.m_view,
    m_spotsStructRes.m_view,
    m_materialsStructRes.m_view,
    m_devicesStructRes.m_view,
    m_deviceIndexesRes.m_view,
    m_aperturesStructRes.m_view,
    m_aperturePtsStructRes.m_view,
    m_compensatorsStructRes.m_view,
    m_compensatorDepthsStructRes.m_view,
    m_sobpGridsRes.m_view
  };
  m_cShaderDevices.SetResourceViews(views, sizeof(views));

  ID3D11UnorderedAccessView* uavViews2[] = { m_buffRWProtonsPos.m_viewRW, m_buffRWProtonsDir.m_viewRW, m_buffRWProtonsAux.m_viewRW };
  m_cShaderDevices.SetUAVViews(uavViews2, sizeof(uavViews2));

  // Set TransportCS 
  ID3D11Buffer* buffers2[] =
  {
    m_dispatchCBuffer.m_buffer,
    m_calcGridCBuffer.m_buffer
  };
  m_cShaderPatient.SetConstantBuffers(buffers2, sizeof(buffers2)); // MLW move to constant over lifetime

  CDxTexture3D* imgTxtr = m_txtrGroupImagesPatient->ItemPtr(IMG_TYPE_RSP);

  ID3D11ShaderResourceView* viewsPatient[] =
  {
    imgTxtr->m_view,
    m_beamsStructRes.m_view,
    m_controlPtsStructRes.m_view,
    m_spotsStructRes.m_view,
    m_bufferSpotScales.m_view,
    m_depthDoseStructRes.m_view,
    m_alphaStructRes.m_view,
    m_depthLETStructRes.m_view,

    m_dijSpots.m_view,
    m_dijBins.m_view,

    m_bufferRWsobpHalos.m_view
  };
  m_cShaderPatient.SetResourceViews(viewsPatient, sizeof(viewsPatient));

  // 
  ID3D11UnorderedAccessView* letView = m_transportParams->doseAux ? m_txtrGroupDosesPatient->ItemPtr(DOSE_TYPE_LET)->m_viewRW : NULL;

  ID3D11UnorderedAccessView* uavViews3[] =
  {
    m_buffRWProtonsPos.m_viewRW,
    m_buffRWProtonsDir.m_viewRW,
    m_buffRWProtonsAux.m_viewRW,
    m_txtrNumProtonsVox->m_viewRW,

    m_txtrGroupDosesPatient->ItemPtr(DOSE_TYPE_DOSE)->m_viewRW,
    letView,
    m_dijVoxels.m_viewRW,

#ifdef DBG_0 // DijSpots
    m_dijBinFill.m_viewRW,
#endif
  };
  m_cShaderPatient.SetUAVViews(uavViews3, sizeof(uavViews3));

  if (transportParams->statsCalc)
  {
    ID3D11Buffer* buffers3[] = { m_dispatchCBuffer.m_buffer };
    m_cShaderDoseStats.SetConstantBuffers(buffers3, sizeof(buffers3));


    ID3D11UnorderedAccessView* uavViews3[] =
    {
      m_txtrGroupDosesPatient->ItemPtr(0)->m_viewRW,
      m_txtrGroupDosesPatient->ItemPtr(1)->m_viewRW,
      m_txtrGroupDosesPatient->ItemPtr(2)->m_viewRW
    };
    m_cShaderDoseStats.SetUAVViews(uavViews3, sizeof(uavViews3));
  }

  return;
}

/*_____________________________________________________________________________________________________________________

  Compute routines used in actual proton tranport from linac through devices through patient
_______________________________________________________________________________________________________________________*/

void CEngineTransport::ComputePlanDose(STransportParams* transportParams)
{
  DWORD tCount;

  SDispatchProtonPrep dispatchProtonPrep = {};


  dispatchProtonPrep.numSpots = m_ionPlan->m_arraySpots.m_numItems;
  dispatchProtonPrep.numProtons = m_numProtonsCalc;


#ifdef DBG_0
  transportParams->numCycles = 2;
#endif

  for (unsigned int cycleIndex = 0; cycleIndex < transportParams->numCycles; ++cycleIndex)
    for (unsigned int cycleIndex = 0; cycleIndex < 1; ++cycleIndex)
  {
    tCount = GetTickCount();

    dispatchProtonPrep.randSeedCycle += 17737;

    m_dispatchCBuffer.Upload(&dispatchProtonPrep);

    m_cShaderSpotPrep.LoadResources();

    InitializeProtonTrajectories();

#ifdef DBG_0 // Post Trajectories (Plan)
    DebugTransport();
    //DebugTexture(m_txtrGroupDosesPatient->ItemPtr(0));
#endif

    // DBG_0
    if (m_ionPlan->m_arrayDevices.m_numItems)
    {
      ComputeDevices();
    }

#ifdef DBG_0 // Post Devices (Plan)
    DebugTransport();
    //DebugTexture(m_txtrGroupDosesPatient->ItemPtr(0));
#endif

    ComputePatient();

#ifdef DBG_0 // Post Patient (Plan)
    //DebugTransport();
    DebugTexture(m_txtrGroupDosesPatient->ItemPtr(0));
#endif

    APPLOG("Transport Cycle Time %d ms\n", (GetTickCount() - tCount));
    
    if (transportParams->statsCalc)
    {
      m_dispatchStats.cycleIndex = cycleIndex;

      ComputeStatistics(&m_dispatchStats);
    }

#ifdef DBG_0
    //DebugTexture(&m_doseTexture);
    //DebugTexture(&m_letTexture);
    //DebugTexture(&m_auxTexture);
#endif
  }

  if (m_dispatchTransport.flagDij)
  {
    CheckDijCalc();
  }

#ifdef DBG_0 // Post Cycles (Plan)
  DebugTransport();
  //DebugTexture(m_txtrGroupDosesPatient->ItemPtr(0));
#endif

  // Clear the Compute Shader output textures so they can be used as graphics inputs
  ClearComputeUAViews(2, 5);

  return;
}

void CEngineTransport::ComputePlanSOBP(STransportParams* transportParams)
{
  DWORD tCount;

  SDispatchProtonPrep dispatchSOBP = {};

  dispatchSOBP.numSpots   = m_ionPlan->m_arraySpots.m_numItems;
  dispatchSOBP.numProtons = m_numProtonsCalc;

#ifdef DBG_0
  transportParams->numCycles = 10;
#endif

  for (unsigned int cycleIndex = 0; cycleIndex < transportParams->numCycles; ++cycleIndex)
  {
    tCount = GetTickCount();

    dispatchSOBP.randSeedCycle += 17737;

    m_cShaderSpotPrepSOBP.LoadResources();

    InitializeProtonTrajectoriesSOBP(&dispatchSOBP);

#ifdef DBG_0 // Post Trajectories (SOBP)
    DebugTransport();
#endif

    // DBG_0 turn off compdevsobp
    if (m_ionPlan->m_arrayDevices.m_numItems)
    {
      ComputeDevices();
    }

#ifdef DBG_0 // Post Devices (SOBP)
    DebugTransport();
#endif

    ComputePatient();

    ClearComputeResourceViews(10, 10);

#ifdef DBG_0 // Post Patient (SOBP)
    DebugTransport();
    //DebugTexture(m_txtrGroupDosesPatient->ItemPtr(0));
#endif

    APPLOG("Transport Cycle Time %d ms\n", (GetTickCount() - tCount));

  }

#ifdef DBG_0 // Post Cycles (SOBP)
  DebugTransport();
  //DebugTexture(m_txtrGroupDosesPatient->ItemPtr(0));
#endif

  // Clear the Compute Shader output textures so they can be used as graphics inputs
  ClearComputeUAViews(2, 5);

  return;
}

void CEngineTransport::ComputeQABeamDoses(STransportParams* transportParams)
{

  memset(&m_dispatchTransport, 0, sizeof(m_dispatchTransport));

  CDxTexture3D* imgTxtr = m_txtrGroupImagesPhantom->ItemPtr(IMG_TYPE_RSP);

  ID3D11ShaderResourceView* views2[] =
  {
    imgTxtr->m_view,
    m_beamsStructRes.m_view,
    m_controlPtsStructRes.m_view,
    m_spotsStructRes.m_view,
    m_bufferSpotScales.m_view,
    m_depthDoseStructRes.m_view,
    m_alphaStructRes.m_view,
    m_depthLETStructRes.m_view,
  };
  m_cShaderPatient.SetResourceViews(views2, sizeof(views2));

  SDispatchProtonPrep dispatchProtonPrep = {};

  dispatchProtonPrep.numSpots      = m_ionPlan->m_arraySpots.m_numItems;
  dispatchProtonPrep.flagQA        = 1;
  dispatchProtonPrep.randSeedCycle = 0;

  for (unsigned int cycleIndex = 0; cycleIndex < transportParams->numCycles; ++cycleIndex)
  {

    dispatchProtonPrep.randSeedCycle += 17737;

    m_dispatchCBuffer.Upload(&dispatchProtonPrep);

    InitializeProtonTrajectories();

    TRACE("PrepDone\n");
    APPLOG("Proton Prep Pass\n");

    if (m_ionPlan->m_arrayDevices.m_numItems)
    {
      ComputeDevices();
      TRACE("DevicesDone QA\n");
    }

#ifdef DBG_0
    DebugTransport(); // QA devices
#endif

    m_cShaderPatient.LoadResources();

    int spotIndex;
    unsigned int indexProton;
    unsigned int numProtons;

    forPrrayI(m_txtrGroupDosesQA, txtr, indexBeam)
    {
      spotIndex    = m_ionPlan->m_arrayBeams[indexBeam].indexSpots;
      indexProton  = m_arraySpotCalc(spotIndex)->indexProtonsCompute;
      spotIndex   += (m_ionPlan->m_arrayBeams[indexBeam].numSpots - 1);
      numProtons   = m_arraySpotCalc(spotIndex)->indexProtonsCompute + m_arraySpotCalc(spotIndex)->numProtonsCompute
                      - indexProton;

      m_cShaderPatient.LoadUAView(txtr->m_viewRW, 2);

      ComputeBeam(indexProton, numProtons);
    }
  }

#ifdef DBG_0 // QA patient
  DebugTransport();
#endif

  m_txtrGroupDosesQA->m_volumeGrid       = m_phantom->m_voxGridRSP;
  m_txtrGroupImagesPhantom->m_volumeGrid = m_phantom->m_voxGridRSP;

  // Clear the CS output textures so they can be used as graphics inputs
  ClearComputeUAViews(3, 5);

  return;
}

void CEngineTransport::InitializeProtonTrajectories(void)
{
  CDxTimer timer;

  timer.Build();
  timer.Start();

  int numThreadGroups = (int)ceil(m_ionPlan->m_arraySpots.m_numItems / 1024.0);

  m_dxContext->Dispatch(1, 1, 1);

  TRACE("InitProtons DxTime %f\n\n", m_numProtonsPerDispatch, timer.End());

  ClearComputeUAViews(1, 3);

  return;
}

void CEngineTransport::InitializeProtonTrajectoriesSOBP(SDispatchProtonPrep* dispatchBuff)
{
  CDxTimer timer;

  timer.Build();
  timer.Start();

  dispatchBuff->dispatchIndex = (m_numProtonsPerDispatch % NUM_DISPATCH_THREADS) / NUM_GROUP_THREADS;

  m_dispatchCBuffer.Upload(dispatchBuff);

  m_dxContext->Dispatch(1, 1, 1);

  TRACE("InitProtonsSOBP DxTime %f\n\n", m_numProtonsPerDispatch, timer.End());

  ClearComputeUAViews(1, 4);

  return;
}

void CEngineTransport::ComputeDevices(void)
{
  m_cShaderDevices.LoadResources();

  SDispatchDevice dispatchDevice = {};

  dispatchDevice.numProtons       =  m_numProtonsCalc;
  unsigned int numFullDispatches  =  m_numProtonsPerDispatch / NUM_DISPATCH_THREADS;
  unsigned int numRemainderGroups = (m_numProtonsPerDispatch % NUM_DISPATCH_THREADS) / NUM_GROUP_THREADS;

  for (unsigned int dispatch = 0; dispatch < numFullDispatches; ++dispatch)
  {
    dispatchDevice.dispatchIndex = dispatch * NUM_DISPATCH_THREADS;

    m_dispatchCBuffer.Upload(&dispatchDevice);

    m_dxContext->Dispatch(NUM_DISPATCH_GROUPS, 1, 1);

    TRACE("DeviceComp %d %d\n", dispatch, numFullDispatches);
  }

  dispatchDevice.dispatchIndex = numFullDispatches * NUM_DISPATCH_THREADS;

  m_dispatchCBuffer.Upload(&dispatchDevice);

  m_dxContext->Dispatch(numRemainderGroups, 1, 1);

  return;
}

#define NUM_DOSE_BINS     (100)

void CEngineTransport::ComputePatient(void)
{
  CDxTimer timer;

  timer.Build();

  m_cShaderPatient.LoadResources();

  int numFullDispatches  = m_numProtonsPerDispatch / NUM_DISPATCH_THREADS;
  int numRemainderGroups = (m_numProtonsPerDispatch % NUM_DISPATCH_THREADS) / NUM_GROUP_THREADS;

  m_dispatchTransport.numProtons    = m_numProtonsCalc;
  m_dispatchTransport.seed          = 17337;
  m_dispatchTransport.dispatchIndex = 0;
  m_dispatchTransport.flagQA        = 0;
  m_dispatchTransport.flagSOBP      = m_ionPlan->m_flagSOBP;
  m_dispatchTransport.flagHCL       = m_ionPlan->m_flagHCL;

  timer.Start();
  for (int dispatch = 0; dispatch < numFullDispatches; ++dispatch)
  {
    
    m_dispatchTransport.dispatchIndex = dispatch * NUM_DISPATCH_THREADS;

    m_dispatchCBuffer.Upload(&m_dispatchTransport);

    m_dxContext->Dispatch(NUM_DISPATCH_GROUPS, 1, 1);

    m_dispatchTransport.seed += 17337;
    m_dispatchTransport.dispatchIndex += NUM_DISPATCH_THREADS;

    TRACE("Patient %d of %d cycles done;\n", dispatch, numFullDispatches);
  }

  m_dispatchCBuffer.Upload(&m_dispatchTransport);

  m_dxContext->Dispatch(numRemainderGroups, 1, 1);

  float tm = timer.End();
  TRACE("Protons %d DxTime %f\n\n", m_numProtonsPerDispatch, tm);

  return;
}

void CEngineTransport::ComputePatientDij(void)
{
  CDxTimer timer;

  timer.Build();

  m_cShaderPatient.LoadResources();

  int numSpots = m_ionPlan->m_arraySpots.m_numItems;
  int numSpotDispatches  = numSpots/ NUM_GROUP_THREADS;
  int numRemainderGroups = numSpots % NUM_GROUP_THREADS;

  int numDispatches = numSpots * m_arraySpotCalc(0)->numProtonsCompute / NUM_GROUP_THREADS; // MLW 
  m_dispatchTransport.numProtons        = m_numProtonsCalc;
  m_dispatchTransport.flagQA            = 0;
  m_dispatchTransport.seed              = 17337;
  m_dispatchTransport.dispatchIndex     = 0;

  DWORD tCount = GetTickCount();
  timer.Start();

  for (int dispatch = 0; dispatch < numDispatches; ++dispatch)
  {

    m_dispatchTransport.dispatchIndex = dispatch * NUM_DISPATCH_THREADS;

    m_dispatchCBuffer.Upload(&m_dispatchTransport);

    m_dxContext->Dispatch(1, 1, 1);

    m_dispatchTransport.seed += 17337;
    m_dispatchTransport.dispatchIndex += NUM_GROUP_THREADS;

  }

  float tm = timer.End();

  tCount = GetTickCount() - tCount;
  TRACE("TickCount %d Protons %d DxTime %f\n\n", tCount, m_numProtonsPerDispatch, tm);

  APPLOG("Transport Pass\n");

  return;
}

void CEngineTransport::ComputeBeam(unsigned int indexProton, unsigned int numProtons)
{

  int numFullDispatches  =  numProtons / NUM_DISPATCH_THREADS;
  int numRemainderGroups = (numProtons % NUM_DISPATCH_THREADS) / NUM_GROUP_THREADS;

  SDispatchTransport dispatchBuff;
  memset(&dispatchBuff, 0, sizeof(SDispatchTransport));

  dispatchBuff.numProtons    = indexProton + numProtons;
  dispatchBuff.seed          = indexProton;
  dispatchBuff.flagQA        = 1; // MLW generalize beyond qa beam
  dispatchBuff.dispatchIndex = indexProton;

  for (int dispatch = 0; dispatch < numFullDispatches; ++dispatch)
  {
    m_dispatchCBuffer.Upload(&dispatchBuff);

    m_dxContext->Dispatch(NUM_DISPATCH_GROUPS, 1, 1);

    dispatchBuff.seed += 17337;
    dispatchBuff.dispatchIndex += NUM_DISPATCH_THREADS;

    TRACE("Patient %d of %d cycles done;\n", dispatch, numFullDispatches);
  }

  m_dispatchCBuffer.Upload(&dispatchBuff);

  m_dxContext->Dispatch(numRemainderGroups, 1, 1);

#ifdef DBG_0 // QA beam
  DebugTransport();
#endif

  return;
}

void CEngineTransport::ComputeStatistics(SDispatchStats* dispatchStats)
{
 
  ClearComputeUAViews(1, 2);

  m_cShaderDoseStats.LoadResources();

  int numFullDispatches  = m_ionPlan->m_voxGridRSP.numVoxels / NUM_DISPATCH_THREADS;
  int numRemainderGroups = (m_ionPlan->m_voxGridRSP.numVoxels % NUM_DISPATCH_THREADS) / NUM_GROUP_THREADS;

  for (int dispatch = 0; dispatch < numFullDispatches; ++dispatch)
  {
    dispatchStats->dispatchIndex = dispatch * NUM_DISPATCH_THREADS;

    m_dispatchCBuffer.Upload(&dispatchStats);

    m_dxContext->Dispatch(NUM_DISPATCH_GROUPS, 1, 1);

  }

  dispatchStats->dispatchIndex = numFullDispatches * NUM_DISPATCH_THREADS;

  m_dispatchCBuffer.Upload(&dispatchStats);

  m_dxContext->Dispatch(numRemainderGroups, 1, 1);

  ClearComputeUAViews(0, 2);

  return;
}

void CEngineTransport::CheckDijCalc(void)
{
  D3D11_MAPPED_SUBRESOURCE mapVoxDij;

  SVoxGridGeometry* geom     = &m_ionPlan->m_voxGridRSP;
  unsigned int numVoxelSlice = geom->dims[0] * geom->dims[1];

  CUniArray<unsigned int> voxelArray(geom->numVoxels);
  voxelArray.ZeroMem();

  m_dxContext->CopyResource(m_dijVoxels.m_bufferRead, m_dijVoxels.m_buffer);
  m_dxContext->Map(m_dijVoxels.m_bufferRead, 0, D3D11_MAP_READ, 0, &mapVoxDij);

  SShaderDijVoxel* voxelStart = (SShaderDijVoxel*)mapVoxDij.pData;
  SShaderDijVoxel* voxelEnd   = voxelStart + m_paramsDij.numVoxels;

  unsigned int voxX;
  unsigned int voxY;
  unsigned int voxZ;
  unsigned int indexVoxel;


  unsigned int numDoseVoxDij = 0;
  unsigned int numKeyVox     = 0;
  double sumDoseDij          = 0.0;
  for (SShaderDijVoxel* voxel = voxelStart; voxel < voxelEnd; ++voxel)
  {

    if (voxel->dose > 0)
    {
      sumDoseDij += voxel->dose;
      ++numDoseVoxDij;
    }

    if (voxel->key)
    {
      ++numKeyVox;

      voxX = (voxel->key >> 20);
      voxY = (voxel->key >> 10) & 0x3FF;
      voxZ = (voxel->key & 0x1FF) - 1;
      indexVoxel = voxZ * numVoxelSlice + voxY * geom->dims[0] + voxX;
      voxelArray[indexVoxel] += 1;
    }
  }

  TRACE("DIJ VOX %f %f %u %u\n", sumDoseDij, numDoseVoxDij, numKeyVox);

  int numKeyUnique = 0;
  forArray(voxelArray, voxel)
  {
    if (*voxel)
    {
      ++numKeyUnique;
    }
  }

  m_dxContext->Unmap(m_dijVoxels.m_bufferRead, 0);

  TRACE("DIJ VOX UNIQUE %d\n", numKeyUnique);

  int numBinsFilled = 0;
  SShaderDijVoxel* voxel;
  forArray(m_arrayDijSpots, spot)
  {
    forArray(m_arrayDijBins, bin)
    {
      voxel = voxelStart + bin->indexVoxel + bin->numVoxels - 1;

      if (voxel->key)
      {
        ++numBinsFilled;
      }
    }
  }

  TRACE("DIJ FILLED BINS %d\n", numBinsFilled);

  unsigned int numDoseVoxPlan;
  double       sumDosePlan;

  CalcTxtrFill(m_txtrGroupDosesPatient->ItemPtr(0), &numDoseVoxPlan, &sumDosePlan);

  return;
}

void CEngineTransport::CalcTxtrFill(CDxTexture3D* txtr, unsigned int* numVoxFill, double* voxSum)
{
  D3D11_MAPPED_SUBRESOURCE mapTxtr;

  m_dxContext->CopyResource(txtr->m_textureRead, txtr->m_texture);
  m_dxContext->Map(txtr->m_textureRead, 0, D3D11_MAP_READ, 0, &mapTxtr);

  SVoxGridGeometry* grid = &m_ionPlan->m_voxGridRSP;

  float* xEnd;
  float* yEnd;
  float* xPtr;
  float* yPtr;
  float* zPtr;
  float* zStart = (float*)mapTxtr.pData;
  float* zEnd = zStart + grid->dims[2] * (mapTxtr.DepthPitch / sizeof(float));
  int zStep = mapTxtr.DepthPitch / sizeof(float);
  int yStep = mapTxtr.RowPitch / sizeof(float);

  *voxSum     = 0.0;
  *numVoxFill = 0;

  for (zPtr = zStart; zPtr < zEnd; zPtr += zStep)
  {
    yEnd = zPtr + grid->dims[1] * yStep;

    for (yPtr = zPtr; yPtr < yEnd; yPtr += yStep)
    {
      xEnd = yPtr + grid->dims[0];

      for (xPtr = yPtr; xPtr < xEnd; ++xPtr)
      {
        // Assume no neg dose
        if (*xPtr > 0.0) 
        {
          *voxSum += *xPtr; 
          ++(*numVoxFill);
        }
      }
    }
  }

  TRACE("DBGTXTR %f %u\n", *voxSum, *numVoxFill);

  m_dxContext->Unmap(txtr->m_textureRead, 0);

  return;
}

void CEngineTransport::DebugTransport(void)
{

  D3D11_MAPPED_SUBRESOURCE mapResourcePos;
  D3D11_MAPPED_SUBRESOURCE mapResourceDir;
  D3D11_MAPPED_SUBRESOURCE mapResourceAux;

  m_dxContext->CopyResource(m_buffRWProtonsPos.m_bufferRead, m_buffRWProtonsPos.m_buffer);
  m_dxContext->CopyResource(m_buffRWProtonsDir.m_bufferRead, m_buffRWProtonsDir.m_buffer);
  m_dxContext->CopyResource(m_buffRWProtonsAux.m_bufferRead, m_buffRWProtonsAux.m_buffer);

  m_dxContext->Map(m_buffRWProtonsPos.m_bufferRead, 0, D3D11_MAP_READ, 0, &mapResourcePos);
  m_dxContext->Map(m_buffRWProtonsDir.m_bufferRead, 0, D3D11_MAP_READ, 0, &mapResourceDir);
  m_dxContext->Map(m_buffRWProtonsAux.m_bufferRead, 0, D3D11_MAP_READ, 0, &mapResourceAux);

  float* protonsPos = (float*)mapResourcePos.pData;
  float* protonsDir = (float*)mapResourceDir.pData;
  SShaderProtonsAux* protonsAux = (SShaderProtonsAux*)mapResourceAux.pData;

  float max = -1.0;
  float min = 100000.0;
  unsigned int bins[101] = {};
  int indexBin;

  FILE* fd;
  CString fileName;
  fileName.Format("%s\\AppResults\\Protons.txt", g_configInstance->m_directoryBase);

  fd = fopen(fileName, "w");
  float* protonPos = protonsPos;
  float* protonDir = protonsDir;
  float* protonEnd = protonsPos + 3*m_arraySpotCalc.m_pItemFirst->numProtonsCompute;
  for (float* proton = protonsPos; proton < protonEnd; proton += 3)
  {
    //if (proton->scaledPathLength > 0.0)
    {
      //fprintf(fd, "%4d %f %f %f %f %f %f %f\n", i, proton->pos[0], proton->pos[1], proton->pos[2],
      //  proton->dir[0], proton->dir[1], proton->dir[2], proton->scaledPathLength);
      //fprintf(fd, "%f %f %f %f %f %f %f\n", proton->pos[0], proton->pos[1], proton->pos[2],
      //  proton->dir[0], proton->dir[1], proton->dir[2], proton->scaledPathLength);
      //fprintf(fd, "%f %f\n", proton->pos[1], proton->pos[2]);
      if (protonDir[2] != 314159)
      {
        int i = 5;
      }


      //indexBin = proton->pos[0] * 100;
      //++bins[indexBin];

    }
  }

    TRACE("Rand Rails %f %f\n", min, max);

  fclose(fd);
  
  m_dxContext->Unmap(m_buffRWProtonsPos.m_bufferRead, 0);
  m_dxContext->Unmap(m_buffRWProtonsDir.m_bufferRead, 0);
  m_dxContext->Unmap(m_buffRWProtonsAux.m_bufferRead, 0);  
  return;

  fileName.Format("%s\\Data\\VoxDiff.txt", g_configInstance->m_directoryBase);

  fd = fopen(fileName, "w");


  //unsigned int sum1 = 0;
  //unsigned int sum2 = 0;
  //double dose1 = 0.0;
  //double dose2 = 0.0;
  //for (proton = protons; proton < protonEnd; ++proton)
  //{
  //  //if (proton->pos[1] != proton->pos[2])
  //  //{
  //  //  fprintf(fd, "%d %f %f %f %f %f\n", proton - protons, proton->pos[0], proton->pos[1], proton->pos[2], proton->dir[0], proton->dir[1]);
  //  //}

  //  //if (proton->pos[0] == 998.0)
  //  //{
  //  //  int j = 3;
  //  //}

  //  //if (proton->pos[0] == 999.0)
  //  //{
  //  //  int j = 4;
  //  //}

  //  sum1 += (unsigned int) proton->pos[1];
  //  sum2 += (unsigned int) proton->pos[2];

  //  dose1 += proton->dir[0];
  //  dose2 += proton->dir[1];
  //}

  fclose(fd);

  //TRACE("VOXSUM %u %u %f %f\n", sum1, sum2, dose1, dose2);


  return;
}

void CEngineTransport::DebugTexture(CDxTexture3D* txtr)
{

  float bounds[8];
  D3D11_MAPPED_SUBRESOURCE doseMap;

  m_dxContext->CopyResource(txtr->m_textureRead, txtr->m_texture);
  m_dxContext->Map(txtr->m_textureRead, 0, D3D11_MAP_READ, 0, &doseMap);

  SVoxGridGeometry* grid = &m_ionPlan->m_voxGridRSP;

  float* xEnd;
  float* yEnd;
  float* xPtr;
  float* yPtr;
  float* zPtr;
  float* zStart = (float*)doseMap.pData;
  float* zEnd   = zStart + grid->dims[2] * (doseMap.DepthPitch / sizeof(float));
  int zStep = doseMap.DepthPitch / sizeof(float);
  int yStep = doseMap.RowPitch / sizeof(float);

  bounds[0] = *zStart;
  bounds[1] = *zStart;

  double sumDose = 0.0;
  int numDoseVox = 0;

  for (zPtr = zStart; zPtr < zEnd; zPtr += zStep)
  {
    yEnd = zPtr + grid->dims[1] * yStep;

    for (yPtr = zPtr; yPtr < yEnd; yPtr += yStep)
    {
      xEnd = yPtr + grid->dims[0];

      for (xPtr = yPtr; xPtr < xEnd; ++xPtr)
      {

        sumDose += *xPtr;

        if (*xPtr > bounds[1])
        {
          bounds[1] = *xPtr;
        }
        else if (*xPtr < bounds[0])
        {
          bounds[0] = *xPtr;
        }

        if (*xPtr > 0.0)
        {
          ++numDoseVox;
        }
      }
    }
  }

  TRACE("DBGTXTR %f %f %f %d\n", bounds[0], bounds[1], sumDose, numDoseVox);

  m_dxContext->Unmap(txtr->m_textureRead, 0);

  return;
}

#include "EnginePostCalc.h"
void CEngineTransport::ComputeDeviceStats(STransportParams* transportParams)
{

  if (m_ionPlan->m_arrayDevices.m_numItems == 0)
  {
    return;
  }

  ID3D11Buffer* buffers[] = { m_dispatchCBuffer.m_buffer };
  m_cShaderDevices.SetConstantBuffers(buffers, sizeof(buffers));

  ID3D11ShaderResourceView* views[] =
  {
    m_beamsStructRes.m_view,
    m_controlPtsStructRes.m_view,
    m_spotsStructRes.m_view,
    m_materialsStructRes.m_view,
    m_devicesStructRes.m_view,
    m_aperturesStructRes.m_view,
    m_aperturePtsStructRes.m_view ,
    m_compensatorsStructRes.m_view,
    m_compensatorDepthsStructRes.m_view
  };
  m_cShaderDevices.SetResourceViews(views, sizeof(views));

  ID3D11UnorderedAccessView* uavViews2[] = { m_buffRWProtonsPos.m_viewRW, m_buffRWProtonsDir.m_viewRW, m_buffRWProtonsAux.m_viewRW };
  m_cShaderDevices.SetUAVViews(uavViews2, sizeof(uavViews2));

  SDispatchProtonPrep dispatchProtonPrep = {};

  dispatchProtonPrep.numSpots = m_ionPlan->m_arraySpots.m_numItems;
  dispatchProtonPrep.flagQA = 0;
  dispatchProtonPrep.randSeedCycle = 0;

  int numCycles = 8;

  for (int cycleIndex = 0; cycleIndex < numCycles; ++cycleIndex)
  {

    dispatchProtonPrep.randSeedCycle += 17737;
    m_dispatchCBuffer.Upload(&dispatchProtonPrep);

    InitializeProtonTrajectories();

    ComputeDevices();

    ClearComputeUAViews(0, 1);

  }

  return;
}

void CEngineTransport::CalcObjectsInfo(void)
{
  float energySpot;
  SShaderBeam* beam;
  SShaderControlPt* controlPt;
  SObjectInfo info = { 0.0, 0, 0.0 };

  m_arrayInfoBeams.Init(m_ionPlan->m_arrayBeams.m_numItems, info);
  m_arrayInfoCtrlPts.Init(m_ionPlan->m_arrayControlPts.m_numItems, info);

  FILE* fd;
  fd = m_fileIO->OpenFile(FILE_DEST_DATA, "InfoSpots.txt", "w");

  SShaderSpotCalc* spotScale = m_arraySpotCalc(0);
  forArrayI(m_ionPlan->m_arraySpots, spot, spotIndex)
  {
    controlPt = m_ionPlan->m_arrayControlPts(spot->indexControlPt);

    beam = m_ionPlan->m_arrayBeams(controlPt->indexBeam);

    energySpot = spot->weight * m_ionPlan->m_arrayDepthDoses[controlPt->indexDepthDoses + controlPt->numDepthDoses - 1] * beam->numFractions;

    m_arrayInfoBeams[controlPt->indexBeam].energyObject   += energySpot;
    m_arrayInfoCtrlPts[spot->indexControlPt].energyObject += energySpot;

    m_arrayInfoBeams[controlPt->indexBeam].protonWeightObject   += spot->weight;
    m_arrayInfoCtrlPts[spot->indexControlPt].protonWeightObject += spot->weight;

    m_arrayInfoBeams[controlPt->indexBeam].numProtonsCalc   += spotScale->numProtonsCompute;
    m_arrayInfoCtrlPts[spot->indexControlPt].numProtonsCalc += spotScale->numProtonsCompute;

    fprintf(fd, "%4d %2d %2d %6d %8d %6.4e %6.4e\n", spotIndex, controlPt->indexBeam, spot->indexControlPt, spotScale->indexProtonsCompute,
      spotScale->numProtonsCompute, spot->weight, energySpot);

    ++spotScale;
  }
  m_fileIO->CloseFile();

  fd = m_fileIO->OpenFile(FILE_DEST_DATA, "InfoBeams.txt", "w");

  beam = m_ionPlan->m_arrayBeams(0);
  forArrayI(m_arrayInfoBeams, beamInfo, count)
  {
    fprintf(fd, "%2d %2d %2d %4d %d %d %6.4e %6.4e\n", count, beam->indexControlPts, beam->numControlPts,
      beam->indexSpots, beam->numSpots, beamInfo->numProtonsCalc, beamInfo->protonWeightObject, beamInfo->energyObject);

    ++beam;
  }
  m_fileIO->CloseFile();

  fd = m_fileIO->OpenFile(FILE_DEST_DATA, "InfoControlPts.txt", "w");

  controlPt = m_ionPlan->m_arrayControlPts(0);
  forArrayI(m_arrayInfoCtrlPts, cntrlPtInfo, count2)
  {
    fprintf(fd, "%3d %2d %4d %3d %6d %6.4e %6.4e\n", count2, controlPt->indexBeam, controlPt->indexSpots, 
      controlPt->numSpots, cntrlPtInfo->numProtonsCalc, cntrlPtInfo->protonWeightObject, cntrlPtInfo->energyObject);

    ++controlPt;
  }
  m_fileIO->CloseFile();

  return;

}
#define uint4 unsigned long

//#define INITRAND1280(seed0, seed1, seed2, seed3) \
//  uint4 s[4];   \
//  uint4 valInt; \
//  uint4 t128;    \
//  uint4 sum128;  \
//  float scale = 2.32830644e-10; \
//  s[0] = seed0; \
//  s[1] = seed1; \
//  s[2] = seed2; \
//  s[3] = seed3;
//
//#define NEWRAND1280(value)                           \
//  sum128 = s[0] + s[3];                             \
//  valInt = ((sum128 << 7) | (sum128 >> 25)) + s[0]; \
//  value = valInt * scale;                           \
//  t128 = s[1] << 9; \
//  s[2] ^= s[0];     \
//  s[3] ^= s[1];     \
//  s[1] ^= s[2];     \
//  s[0] ^= s[3];     \
//  s[2] ^= t128;     \
//  s[3] = (s[3] << 11) | (s[3] >> 21);
//
//void CEngineTransport::DebugRand(void)
//{
//
//  float randVal1;
//
//  int bins[100] = {};
//  int indexBin;
//
//  INITRAND1280(83452, 2756, 83, 0);
//  float max = -1;
//  float min = 10000;
//
//  for (int index = 0; index < 1000000; ++index)
//  {
//    NEWRAND1280(randVal1);
//
//    if (randVal1 > max)
//    {
//      max = randVal1;
//    }
//    else if (randVal1 < min)
//    {
//      min = randVal1;
//    }
//
//    indexBin = randVal1 * 100;
//    ++bins[indexBin];
//    
//  }
//
//  TRACE("Rand Rails %f %f\n", min, max);
//
//  return;
//}

#define TWO_PI   (6.28318530718)
#define QA_SAD   (10000000.0)
#define MAX_UINT (4294967295.0)

void CEngineTransport::TestRand(void)
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
  float max = -10000;
  float min =  10000;
  int bins[100] = {};
  int zeros[NUM_GROUP_THREADS] = {};
  int indexBin;
  int indexSpot;

  unsigned int dispatchIndex = 0;
  SShaderProton* ubProtons = new SShaderProton[m_transportParams->numProtonsTransport];
  SShaderProton* proton = ubProtons;

  CUniArray<unsigned int> seeds(NUM_GROUP_THREADS * 4);

  INITSEEDS128(123456, 234567, 3456789, 4567890);

  forArrayI(seeds, seedItem, item)
  {
    NEWSEED128(*seedItem);
  }

  unsigned int* ubRandSeeds = seeds.m_pItemFirst;

  unsigned int indexProton = 0;
  unsigned int numDispatches = m_transportParams->numProtonsTransport / NUM_GROUP_THREADS;

  for (unsigned int indexThread = 0; indexThread < NUM_GROUP_THREADS; ++indexThread, ubRandSeeds += 4)
  //for (unsigned int indexThread = 0; indexThread < 1; ++indexThread)
  {
    INITRAND128(ubRandSeeds);

    for (unsigned int indexDispatch = 0; indexDispatch < numDispatches; ++indexDispatch, ++proton, ++indexProton)
    {
      for (int index = m_ionPlan->m_arraySpots.m_numItems - 1; index >= 0; --index)
      {
        if (indexProton >= m_arraySpotCalc[index].indexProtonsCompute)
        {
          indexSpot = index;
          break;
        }
      }

      int indexControlPt = m_ionPlan->m_arraySpots[indexSpot].indexControlPt;
      int indexBeam = m_ionPlan->m_arrayControlPts[indexControlPt].indexBeam;

      sadScale = m_ionPlan->m_arrayBeams[indexBeam].sadScale;
      protonInitZ = m_ionPlan->m_arrayBeams[indexBeam].protonInitZ;

      proton->pos[1] = seedCurrent[0];
      proton->pos[2] = seedCurrent[3];

      NEWRAND128(randVal1);
      //randVal1 = sqrt(randVal1);

      // DBG_0
      proton->pos[0] = randVal1;
      proton->dir[0] = sum128;
      proton->dir[1] = valInt;
      //proton->[1] = valInt;
      //proton->[2] = sum128;
      //proton->dir[0] = scale;
      //STORESEED128(ubRandSeeds[groupThreadID[0]])
      // 
      if (proton->pos[0] > max)
      {
        max = proton->pos[0];
      }
      else if (proton->pos[0] < min)
      {
        min = proton->pos[0];
      }

      indexBin = randVal1 * 100;
      ++bins[indexBin];
      if (indexBin == 0)
      {
        ++zeros[indexThread];
      }
      continue;

      //NEWRAND(randVal2);
      NEWRAND128(randVal2);
      randVal2 *= TWO_PI;

      // Find proton pos in isocenter plane using Guassian around spot center
      //isoPlanePosX = rbSpots[indexSpot].spotCenterX + rbControlPts[indexControlPt].spotSize[0] * randVal1 * cos(randVal2);
      //isoPlanePosY = rbSpots[indexSpot].spotCenterY + rbControlPts[indexControlPt].spotSize[1] * randVal1 * sin(randVal2);

      // DBG_0
      isoPlanePosX = m_ionPlan->m_arrayControlPts[indexControlPt].spotSize[0] * randVal1 * cos(randVal2);
      isoPlanePosY = m_ionPlan->m_arrayControlPts[indexControlPt].spotSize[1] * randVal1 * sin(randVal2);



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

      float spotSigma = 36.0;

      // Smear location of proton in emittance plane
      proton->pos[0] = isoPlanePosX + spotSigma * sqrtVal1 * cos(randVal2);
      proton->pos[1] = isoPlanePosY + spotSigma * sqrtVal1 * sin(randVal2);
      proton->pos[2] = protonInitZ;

      // Calc proton trajectory
      dirX = isoPlanePosX - proton->pos[0];
      dirY = isoPlanePosY - proton->pos[1];

      inverseDirNorm = 1.0 / sqrt(dirX * dirX + dirY * dirY + proton->pos[2] * proton->pos[2]);

      // Normalize proton trajectory
      proton->dir[0] = dirX * inverseDirNorm;
      proton->dir[1] = dirY * inverseDirNorm;
      proton->dir[2] = -proton->pos[2] * inverseDirNorm;

      proton->indexSpot = indexSpot;
      proton->scaledPathLength = 0.0;

    }

    //RESEED128(ubRandSeeds[indexThread])
  }

  TRACE("Rand Rails %f %f\n", min, max);

  return;
}
