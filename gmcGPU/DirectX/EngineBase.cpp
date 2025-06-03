#include "stdafx.h"
#include "Heapster.h"
#include "EngineBase.h"

CAppState* CEngineBase::m_appState = NULL;
CFileIO*   CEngineBase::m_fileIO   = NULL;

CDxBuffRW*     CEngineBase::m_buffRWProtonsAuxBase   = NULL;
CTextureGroup* CEngineBase::m_txtrGroupDosesPatient  = NULL;
CTextureGroup* CEngineBase::m_txtrGroupDosesQA       = NULL;
CTextureGroup* CEngineBase::m_txtrGroupDosesExternal = NULL;
CTextureGroup* CEngineBase::m_txtrGroupGammas        = NULL;
CTextureGroup* CEngineBase::m_txtrGroupDosesStats    = NULL;

CTextureGroup*     CEngineBase::m_txtrGroupImagesPatient = NULL;
CTextureGroup*     CEngineBase::m_txtrGroupImagesPhantom = NULL;
CDxTexture3D*      CEngineBase::m_txtrNumProtonsVox      = NULL;
CDxTextureSampler* CEngineBase::m_textureSampler         = NULL;

STransportParams*  CEngineBase::m_transportParams        = NULL;

CEngineBase::CEngineBase(void)
{
  return;
}

CEngineBase::~CEngineBase(void)
{

  return;
}

void CEngineBase::SetAppState(CAppState* appState)
{ 
  m_appState = appState; 
  m_fileIO   = appState->m_workers.fileIO;
  
  m_transportParams        = &m_appState->m_transportParams;

  m_txtrGroupDosesPatient  = m_appState->m_arrayTxtrGroupDoses(DOSE_GROUP_PATIENT);
  m_txtrGroupDosesQA       = m_appState->m_arrayTxtrGroupDoses(DOSE_GROUP_QA);
  m_txtrGroupDosesStats    = m_appState->m_arrayTxtrGroupDoses(DOSE_GROUP_STATS);
  m_txtrGroupDosesExternal = m_appState->m_arrayTxtrGroupDoses(DOSE_GROUP_EXTERNAL);
  m_txtrGroupGammas        = m_appState->m_arrayTxtrGroupDoses(DOSE_GROUP_GAMMAS);

  m_txtrGroupImagesPatient = m_appState->m_arrayTxtrGroupImageVols(IMAGE_GROUP_PATIENT);
  m_txtrGroupImagesPhantom = m_appState->m_arrayTxtrGroupImageVols(IMAGE_GROUP_PHANTOM);

  m_txtrNumProtonsVox = &m_appState->m_txtrNumProtonsVox;

  m_textureSampler = &m_appState->m_txtrSampler;

  return;
}

void CEngineBase::ClearComputeUAViews(int startIndex, int endIndex)
{
  for (int index = startIndex; index <= endIndex; ++index)
  {
    m_dxContext->CSSetUnorderedAccessViews(index, 1, &m_nullViewRW, 0);
  }

  return;
}

void CEngineBase::ClearComputeResourceViews(int startIndex, int endIndex)
{
  for (int index = startIndex; index <= endIndex; ++index)
  {
    m_dxContext->CSSetShaderResources(index, 1, &m_nullView);
  }

  return;
}

bool CEngineBase::Initialize(void)
{

  ReturnOnFalse(CreateShaders())

  ReturnOnFalse(CreatePersistentResources())

  return true;
}