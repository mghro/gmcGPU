#include "stdafx.h"
#include "resource.h"
#include "AppState.h"
#include "Viewer.h"
#include "Workflow.h"
#include "PanelBase.h"
#include "PanelPACSMonitor.h"

#define QUERY_PERIOD_MINUTES      (15)
#define QUERY_PERIOD_MSEC         (QUERY_PERIOD_MINUTES * 60 * 1000)

BEGIN_MESSAGE_MAP(CPanelPACSMonitor, CDialog)

  ON_WM_TIMER()

  ON_MESSAGE(MSG_WF_COMPUTE_DONE,      OnComputePlanDone)
  ON_MESSAGE(MSG_WF_PACS_PROCESS_DONE, OnPACSDone)

END_MESSAGE_MAP()

CPanelPACSMonitor::CPanelPACSMonitor(CWnd* parent) : CPanelBase(parent),
  m_workflow  (m_appState->m_workers.workflow)
{
  Create(IDD_PNL_PACS_MONITOR, parent);

  return;
}

CPanelPACSMonitor::~CPanelPACSMonitor(void)
{
  return;
}

BOOL CPanelPACSMonitor::OnInitDialog(void)
{
  CDialog::OnInitDialog();

  
  SetWindowTheme(GetDlgItem(IDC_LIST1)->GetSafeHwnd(), L"Explorer", NULL);

  return FALSE;
}

void CPanelPACSMonitor::OnTimer(UINT_PTR nTimerID)
{
  KillTimer(nTimerID);

  m_workflow->ProcessNewPACSPlans(this);

  return;
}

LRESULT CPanelPACSMonitor::OnComputePlanDone(WPARAM wParam, LPARAM lParam)
{

  return TRUE;
}

LRESULT CPanelPACSMonitor::OnPACSDone(WPARAM wParam, LPARAM lParam)
{
  SetTimer(1, QUERY_PERIOD_MSEC, NULL);

  return TRUE;
}
