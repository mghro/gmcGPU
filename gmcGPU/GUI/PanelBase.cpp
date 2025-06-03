#include "stdafx.h"
#include "AppState.h"
#include "Workflow.h"
#include "PanelTabCtrl.h"
#include "PanelBase.h"

CAppState*      CPanelBase::m_appState            = NULL;

CPanelBase::CPanelBase(CWnd* parent) : CDialog(),
  m_parent (parent)
{

  return;
}

CPanelBase::~CPanelBase(void)
{

  return;
}

void CPanelBase::SetAppState(CAppState* appState)
{

  m_appState = appState;

  return;
}

CStaticCustom::CStaticCustom(void)
{
  m_brush.CreateStockObject(WHITE_PEN);

  return;
}

CStaticCustom::~CStaticCustom(void)
{

  return;
}

HBRUSH CStaticCustom::CtlColor(CDC* pDC, UINT nCtlColor)
{

  return (HBRUSH)m_brush;
}
