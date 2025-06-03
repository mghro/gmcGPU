#include "stdafx.h"
#include "resource.h"
#include "UniArray.h"
#include "DicomMsgParams.h"
#include "PACSServer.h"
#include "PACSPipe.h"
#include "AppConfigInstance.h"
#include "AppState.h"
#include "Viewer.h"
#include "PanelBase.h"
#include "PanelPACSMonitor.h"
#include "PanelConfig.h"

BEGIN_MESSAGE_MAP(CPanelConfig, CDialog)

  ON_CBN_SELCHANGE(IDC_COMBO1, SetServerQuery)
  ON_CBN_SELCHANGE(IDC_COMBO2, SetAEQuery)
  ON_CBN_SELCHANGE(IDC_COMBO3, SetServerSave)
  ON_CBN_SELCHANGE(IDC_COMBO4, SetAESave)
  ON_CBN_SELCHANGE(IDC_COMBO5, SetAEGMCQuery)
  ON_CBN_SELCHANGE(IDC_COMBO6, SetAEGMCSave)

  ON_BN_CLICKED(IDC_CHECK1, SetMIMMonitorStatus)

  ON_WM_TIMER()

END_MESSAGE_MAP()

CPanelConfig::CPanelConfig(CPanelTabCtrl* parent) : CPanelBase(parent),
  m_selectedServerQuery (NULL),
  m_selectedServerStore (NULL),
  m_panelPACSMonitor    (NULL)
{

  m_panelName = " Config ";

  Create(IDD_PNL_CONFIG, parent);

  return;
}

BOOL CPanelConfig::OnInitDialog(void)
{
  CDialog::OnInitDialog();

  m_serversQuery = &g_configInstance->m_serversQuery;
  m_serversStore = &g_configInstance->m_serversStore;

  CPACSPipe* pacsPipe = m_appState->m_workers.pacsPipe;

  pacsPipe->SetMsgParamsQuery(&m_dicomMsgParamsQuery);
  pacsPipe->SetMsgParamsStore(&m_dicomMsgParamsStore);


  LoadAEs(IDC_COMBO5, &g_configInstance->m_aeNamesGmc);
  m_dicomMsgParamsQuery.aeNameGMC = g_configInstance->m_aeNamesGmc.ItemFirst();

  LoadAEs(IDC_COMBO6, &g_configInstance->m_aeNamesGmc);
  m_dicomMsgParamsStore.aeNameGMC = g_configInstance->m_aeNamesGmc.ItemFirst();

  LoadServers(IDC_COMBO1);
  LoadAEs(IDC_COMBO2, &m_serversQuery->ItemPtr(0)->aeNames);
  SetServerQuery();

  LoadServers(IDC_COMBO3);
  LoadAEs(IDC_COMBO4, &m_serversStore->ItemPtr(0)->aeNames);
  SetServerSave();

  if (!g_configInstance->m_mimMonitorLogFile.IsEmpty())
  {
    GetDlgItem(IDC_STATIC2)->ShowWindow(SW_SHOW);
    GetDlgItem(IDC_CHECK1)->ShowWindow(SW_SHOW);
  }

  return TRUE;
}

CPanelConfig::~CPanelConfig(void)
{
  return;
}

void CPanelConfig::SetAEGMCQuery(void)
{
  SetAENameGmc(IDC_COMBO5);

  return;
}

void CPanelConfig::SetAEGMCSave(void)
{
  SetAENameGmc(IDC_COMBO6);

  return;
}

void CPanelConfig::SetAENameGmc(int indexCombo)
{
  int index = ((CComboBox*)GetDlgItem(indexCombo))->GetCurSel();

  m_dicomMsgParamsQuery.aeNameServer = m_selectedServerQuery->aeNames[index];

  return;
}

void CPanelConfig::SetServerQuery(void)
{

  int index = ((CComboBox*) GetDlgItem(IDC_COMBO1))->GetCurSel();
  SPACSServer* newServer = m_serversQuery->ItemPtr(index);

  if (newServer != m_selectedServerQuery)
  {
    m_selectedServerQuery = newServer;

    LoadAEs(IDC_COMBO2, &m_selectedServerQuery->aeNames);

    m_dicomMsgParamsQuery.serverIPAddress = m_selectedServerQuery->serverIPAddress;
    m_dicomMsgParamsQuery.serverPortNum   = m_selectedServerQuery->serverPort;
    m_dicomMsgParamsQuery.aeNameServer          = m_selectedServerQuery->aeNames[0];
  }
  
  return;
}

void CPanelConfig::SetAEQuery(void)
{
  int index = ((CComboBox*)GetDlgItem(IDC_COMBO2))->GetCurSel();

  m_dicomMsgParamsQuery.aeNameServer = m_selectedServerQuery->aeNames[index];

  return;
}

void CPanelConfig::SetServerSave(void)
{

  int index = ((CComboBox*) GetDlgItem(IDC_COMBO3))->GetCurSel();
  SPACSServer* newServer = m_serversStore->ItemPtr(index);

  if (newServer != m_selectedServerStore)
  {
    m_selectedServerStore = newServer;

    LoadAEs(IDC_COMBO4, &m_selectedServerStore->aeNames);

    m_dicomMsgParamsStore.serverIPAddress = m_selectedServerStore->serverIPAddress;
    m_dicomMsgParamsStore.serverPortNum   = m_selectedServerStore->serverPort;
    m_dicomMsgParamsStore.aeNameServer    = m_selectedServerStore->aeNames.Item(0);
  }

  return;
}

void CPanelConfig::SetAESave(void)
{
  int index = ((CComboBox*)GetDlgItem(IDC_COMBO4))->GetCurSel();

  m_dicomMsgParamsStore.aeNameServer = m_selectedServerStore->aeNames.Item(index);

  return;
}

void CPanelConfig::LoadServers(int wndIndex)
{
  CUniArray<SPACSServer>* serverArray;

  if (wndIndex == IDC_COMBO1)
  {
    serverArray = m_serversQuery;
  }
  else
  {
    serverArray = m_serversStore;
  }

  CComboBox* combo = (CComboBox*)GetDlgItem(wndIndex);
  combo->ResetContent();

  forPrray(serverArray, server)
  {
    combo->AddString(server->serverName);
  }

  combo->SetCurSel(0);

  return;
}

void CPanelConfig::LoadAEs(int wndIndex, CUniArray<CString>* aeNames)
{
   
  CComboBox* combo = (CComboBox*)GetDlgItem(wndIndex);
  combo->ResetContent();

  forPrray(aeNames, name)
  {
    combo->AddString(*name);
  }

  combo->SetCurSel(0);

  return;
}

void CPanelConfig::SetMIMMonitorStatus(void)
{
  CButton* button = (CButton*) GetDlgItem(IDC_CHECK1);

  CWnd* wnd = m_appState->m_workers.viewer;

  if (button->GetCheck())
  {
    CRect rect;
    wnd->GetWindowRect(&rect);
    wnd->ShowWindow(SW_HIDE);

    m_panelPACSMonitor = new CPanelPACSMonitor(AfxGetMainWnd());
    m_panelPACSMonitor->MoveWindow(0, 0, rect.Width(), rect.Height(), TRUE);

    m_panelPACSMonitor->PostMessage(WM_TIMER, 1, NULL);
  }
  else
  {
    wnd->ShowWindow(SW_SHOW);
  }

  return;
}