#include "stdafx.h"
#include "resource.h"
#include "Heapster.h"
#include "PanelBase.h"
#include "PanelTabCtrl.h"

BEGIN_MESSAGE_MAP(CPanelTabCtrl, CDialog)

  ON_NOTIFY(TCN_SELCHANGE, IDC_TAB1, OnSelChange)

  ON_MESSAGE(MSG_TAB_CHANGE_CMD, ComputeDone)

END_MESSAGE_MAP()

CPanelTabCtrl::CPanelTabCtrl(CWnd* parent) : CDialog(),
  m_tabPanels (NULL)
{
  Create(IDD_PNL_CTRL, parent);

  return;
}

CPanelTabCtrl::~CPanelTabCtrl(void)
{
  SafeDeleteArray(m_tabPanels);

  return;
}

void CPanelTabCtrl::DoDataExchange(CDataExchange* pDX)
{
  CDialog::DoDataExchange(pDX);

  DDX_Control(pDX, IDC_TAB1, m_tabCtrl);

  return;
}

BOOL CPanelTabCtrl::OnInitDialog(void)
{
  CDialog::OnInitDialog();

  CRect rect;
  GetClientRect(&rect);
  m_tabCtrl.MoveWindow(rect);

  return TRUE;
}

void CPanelTabCtrl::AddPanels(CPanelBase** panels, int numPanels)
{
  CRect rect;
  GetClientRect(&rect);

  m_tabPanels = panels;

  for (int index = 0; index < numPanels; ++index)
  {
    m_tabCtrl.InsertItem(index, m_tabPanels[index]->GetName());
  }

  // Get the client area of the CTabCtrl affected by previusly added tabs
  m_tabCtrl.AdjustRect(FALSE, rect);

  // Set size of all tab panels to client area
  for (int index = 0; index < numPanels; ++index)
  {
    m_tabPanels[index]->MoveWindow(&rect);
  }

  m_currentTabIndex = 0;
  m_tabPanels[0]->ShowWindow(SW_SHOW);

  m_tabCtrl.SetCurSel(0);

  return;
}

void CPanelTabCtrl::OnSelChange(NMHDR* pNMHDR, LRESULT* pResult)
{
  //*pResult = 0;

  if (m_currentTabIndex == m_tabCtrl.GetCurSel())
  {
    return;
  }

  m_tabPanels[m_currentTabIndex]->EnableWindow(FALSE);
  m_tabPanels[m_currentTabIndex]->ShowWindow(FALSE);

  m_currentTabIndex = m_tabCtrl.GetCurSel();
  m_tabPanels[m_currentTabIndex]->EnableWindow(TRUE);
  m_tabPanels[m_currentTabIndex]->ShowWindow(TRUE);

  return;
}

LRESULT CPanelTabCtrl::ComputeDone(WPARAM wParam, LPARAM lParam)
{
  int prevIndex = m_tabCtrl.GetCurSel();

  m_tabCtrl.SetCurSel(TAB_FACE_VIEW);

  m_tabPanels[TAB_FACE_VIEW]->OnTabEnter(prevIndex, (void*)wParam);

  OnSelChange(0, 0);

  return TRUE;
}