#pragma once

#include "Transport.h"

enum
{
  DOSE_GROUP_PATIENT = 0,
  DOSE_GROUP_QA,
  DOSE_GROUP_STATS,
  DOSE_GROUP_EXTERNAL,
  DOSE_GROUP_GAMMAS,

  NUM_DOSE_GROUPS
};

enum
{
  IMAGE_GROUP_PATIENT = 0,
  IMAGE_GROUP_PHANTOM,

  NUM_IMAGE_GROUPS
};

class CEngineTransport;
class CEnginePostCalc;
class CEngineVis;
class CFileIO;
class CViewer;
class CIonPlan;
class CPhantomQA;
class CPACSPipe;
class CWorkflow;

struct SWorkers
{
  CEngineTransport* engineTransport = NULL;
  CEnginePostCalc*  enginePostCalc  = NULL;
  CEngineVis*       engineVis       = NULL;
  CFileIO*          fileIO          = NULL;
  CViewer*          viewer          = NULL;
  CPACSPipe*        pacsPipe        = NULL;
  CWorkflow*        workflow        = NULL;
};

struct SCompObjects
{
  CIonPlan*   ionPlan   = NULL;
  CPhantomQA* phantomQA = NULL;
};

class CAppState
{
public:

  CAppState(void);
  ~CAppState(void);

  SWorkers          m_workers;
  SCompObjects      m_compObjects;
  STransportParams  m_transportParams;


  // Results
  CUniArray<CTextureGroup> m_arrayTxtrGroupDoses;
  CUniArray<CTextureGroup> m_arrayTxtrGroupImageVols;
  CDxTexture3D             m_txtrNumProtonsVox;

  // Tools
  CDxTextureSampler        m_txtrSampler;

};

