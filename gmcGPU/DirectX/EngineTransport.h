#pragma once

#include "DirectX.h"
#include "EngineBase.h"
#include "Transport.h"
#include "ShaderPlan.h"


class CIonPlan;
class CPhantomQA;
class CAppState;

struct SShaderPlan;
struct STransportParams;

// _____________Keep dispatch structs the same size (multiples of 16 bytes) via the padding_______________

struct SDispatchProtonPrep
{
  DISPATCH_SPOTPREP
};

struct SDispatchDevice
{
  unsigned int dispatchIndex;
  unsigned int numProtons;

  unsigned int pad0[6];
};

struct SDispatchTransport
{
  DISPATCH_PATIENT
};

struct SDispatchStats
{
  unsigned int dispatchIndex;
  unsigned int cycleIndex;
  unsigned int numCyclesTotal;

  int numVoxSlice;
  int numVoxRow;

  unsigned int pad0[3];
};

struct SObjectInfo
{
  float energyObject;
  unsigned int numProtonsCalc;
  float protonWeightObject;
};

//__________End Dispatch Structures_______________________________________


class CEngineTransport : public CEngineBase
{
public:

  CEngineTransport(void);
  ~CEngineTransport(void);
  
  // One time initialization
  void ClearResourcesPlan(void);
  void ClearResourcesProtons(void);
  void ClearResourcesResults(void);
  void ClearResourcesDij(void);

  // Prepare for
  bool LoadRTIonPlan(CIonPlan* ionPlan);
  bool LoadQAPhantom(CPhantomQA* phantom);
  bool PrepCompute(STransportParams* transportParams);

  void ComputePlanDose(STransportParams* transportParams);
  void ComputePlanSOBP(STransportParams* transportParams);
  void ComputeQABeamDoses(STransportParams* transportParams);

private:

  // One time initialization routines
  bool CreateShaders(void);
  bool CreatePersistentResources(void);

  bool CreatePlanBuffers(void);
  bool LoadImageGridParams(SVoxGridGeometry* grid);

  bool PrepDIJ(void);
  bool PrepSpotProtons(STransportParams* transportParams);
  bool CreateProtonBuffers(void);
  void SetTransportViews(STransportParams* transportParams);
  void CalcObjectsInfo(void);

  void InitializeProtonTrajectories(void);
  void InitializeProtonTrajectoriesSOBP(SDispatchProtonPrep* dispatchBuff);
  void ComputeDevices(void);
  void ComputePatient(void);
  void ComputePatientDij(void);
  void ComputeBeam(unsigned int indexProton, unsigned int numProtons);
  void ComputeStatistics(SDispatchStats* dispatchStats);

  void CheckDijCalc(void);
  void CalcTxtrFill(CDxTexture3D* txtr, unsigned int* numVoxFill, double* voxSum);

  void ReleaseShaderResources(void);

  // Dbg and Dev 
  void DebugTransport(void);
  void DebugTexture(CDxTexture3D* txtr);
  void DebugRand(void);
  void TestRand(void);

  void ComputeDeviceStats(STransportParams* transportParams);

  CDxShaderCompute m_cShaderSpotPrep;
  CDxShaderCompute m_cShaderSpotPrepSOBP;
  CDxShaderCompute m_cShaderDevices;
  CDxShaderCompute m_cShaderPatient;
  CDxShaderCompute m_cShaderDoseStats;

  CDxBuffConstant m_calcGridCBuffer;
  CDxBuffConstant m_dispatchCBuffer;

  CDxBuffResource m_beamsStructRes;
  CDxBuffResource m_controlPtsStructRes;
  CDxBuffResource m_spotsStructRes;
  CDxBuffResource m_bufferSpotScales;
  CDxBuffResource m_materialsStructRes;
  CDxBuffResource m_devicesStructRes;
  CDxBuffResource m_deviceIndexesRes;
  CDxBuffResource m_aperturesStructRes;
  CDxBuffResource m_aperturePtsStructRes;
  CDxBuffResource m_compensatorsStructRes;
  CDxBuffResource m_compensatorDepthsStructRes;
  CDxBuffResource m_sobpGridsRes;
  CDxBuffResource m_depthDoseStructRes;
  CDxBuffResource m_depthLETStructRes;
  CDxBuffResource m_alphaStructRes;

  CDxBuffRW m_buffRWProtonsPos;
  CDxBuffRW m_buffRWProtonsDir;
  CDxBuffRW m_buffRWProtonsAux;
  CDxBuffRW m_bufferRWrandSeeds;
  CDxBuffRW m_bufferRWsobpHalos;

  // Dij resources
  CDxBuffResource m_dijSpots;
  CDxBuffResource m_dijBins;
  CDxBuffRW       m_dijVoxels;
  CUniArray<SShaderDijSpot> m_arrayDijSpots;
  CUniArray<SShaderDijBin>  m_arrayDijBins;

#ifdef DBG_0 // DijSpots
  CDxBuffRW       m_dijBinFill;
#endif

  CIonPlan*           m_ionPlan;
  CPhantomQA*         m_phantom;
  SDijParams          m_paramsDij;

  SDispatchTransport  m_dispatchTransport;
  SDispatchStats      m_dispatchStats;

  CUniArray<SShaderSpotCalc> m_arraySpotCalc;
  unsigned int m_numProtonsPerDispatch;
  unsigned int m_numProtonsCalc;

  CUniArray<SObjectInfo> m_arrayInfoBeams;
  CUniArray<SObjectInfo> m_arrayInfoCtrlPts;

};