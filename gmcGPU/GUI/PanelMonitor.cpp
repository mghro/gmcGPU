#include "stdafx.h"
#include "resource.h"
#include "PanelBase.h"
#include "Workflow.h"
#include "PanelMonitor.h"

#define FIFTEEN_MINUTES_SEC    (900)
#define QUERY_PERIOD_SEC       (FIFTEEN_MINUTES_SEC)

BEGIN_MESSAGE_MAP(CPanelPACSMonitor, CDialog)

  ON_WM_TIMER()

  ON_MESSAGE(MSG_WF_COMPUTE_DONE, OnPlanComputeDone)

END_MESSAGE_MAP()

CPanelPACSMonitor::CPanelPACSMonitor(CWnd* parent, CRect* rect) : CPanelBase(),
  m_monitoringEnabled (false),
  m_transportParams   (&m_appState->m_transportParams),
  m_rect              (rect)
{

  Create(IDD_PNL_MONITOR, parent);

  return;
}

CPanelPACSMonitor::~CPanelPACSMonitor(void)
{
  return;
}

BOOL CPanelPACSMonitor::OnInitDialog()
{
  CDialog::OnInitDialog();

  SetWindowPos(NULL, m_rect->left, m_rect->top, m_rect->Width(), m_rect->Height(), 0);

  // Set large font for title
  NONCLIENTMETRICS ncm;
  ncm.cbSize = sizeof(ncm);
  SystemParametersInfo(SPI_GETNONCLIENTMETRICS, ncm.cbSize, &ncm, 0);
  LOGFONT lfDlgFont = ncm.lfStatusFont;
  lfDlgFont.lfHeight = -24;
  m_titleFont.CreateFontIndirect(&lfDlgFont);

  CWnd* staticWnd = GetDlgItem(IDC_STATIC);
  staticWnd->SetFont(&m_titleFont);
  CenterWndHorz(staticWnd);

  return TRUE;
}

void CPanelPACSMonitor::ToggleStartStop(void)
{
  if (m_monitoringEnabled == false)
  {
    SetTimer(1, QUERY_PERIOD_SEC * 1000, NULL);
    //PostMessage(WM_TIMER, 1, NULL);

    GetDlgItem(IDC_BUTTON1)->SetWindowTextA("Start");
  }
  else
  {
    GetDlgItem(IDC_BUTTON1)->SetWindowTextA("Start");
  }

  m_monitoringEnabled = !m_monitoringEnabled;

  return;
}

void CPanelPACSMonitor::OnTimer(UINT_PTR nTimerID)
{

  StartPACSQuery();

  return;
}

void CPanelPACSMonitor::StartPACSQuery(void)
{

  m_workflow->ComputeNewPACSPlans(m_transportParams, this);

  return;
}

LRESULT CPanelPACSMonitor::OnPlanComputeDone(WPARAM wParam, LPARAM lParam)
{

  SetTimer(1, QUERY_PERIOD_SEC * 1000, NULL);

  return TRUE;
}