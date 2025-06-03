#include "stdafx.h"
#include "AppState.h"


CAppState::CAppState(void) :
  m_workers         ({}),
  m_compObjects     ({}),
  m_transportParams ({})
{

  m_arrayTxtrGroupDoses.Alloc(NUM_DOSE_GROUPS);
  m_arrayTxtrGroupImageVols.Alloc(NUM_IMAGE_GROUPS);

  m_txtrSampler.CreateSamplerFloat();

  return;
}

CAppState::~CAppState(void)
{

  return;
}

