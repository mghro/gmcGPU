#include "stdafx.h"
#include <memory>
#include "resource.h"
#include "Heapster.h"
#include "Logger.h"
#include "DlgPlanGetPacs.h"
#include "IonPlan.h"
#include "Viewer.h"
#include "EngineTransport.h"
#include "EngineVis.h"
#include "Workflow.h"
#include "Utils.h"
#include "PanelBase.h"
#include "PanelSetup.h"

#define DBG_1             (1)
#define DEG_TO_RAD        (M_PI/180.0)
#define SCALE_FACTOR_MAX  (5)
#define SCALE_FACTOR_MIN  (3)
#define NUM_CYCLES        (10)


BEGIN_MESSAGE_MAP(CPanelSetup, CDialog)

  ON_BN_CLICKED(IDC_BTN_JSON,         SelectPlanJSON)
  ON_BN_CLICKED(IDC_BTN_PLAYER,       SelectPlanPlayer)
  ON_BN_CLICKED(IDC_BTN_PACS,         SelectPlanPACS)
  ON_BN_CLICKED(IDC_BTN_RUN_PLAN,     OnResetRerun)
  ON_BN_CLICKED(IDC_BTN_APERTURES,    SaveApertures)
  ON_BN_CLICKED(IDC_BTN_RESAMPLE,     ResampleDose)
  ON_BN_CLICKED(IDC_CHK_AUTO,         ToggleAutoMode)
  ON_BN_CLICKED(IDC_CHK_AUX,          ToggleDoseAux)
  ON_BN_CLICKED(IDC_CHK_QA,           ToggleQADoses)
  ON_BN_CLICKED(IDC_CHK_DIJ,          ToggleDij)
  ON_BN_CLICKED(IDC_CHK_PACS,         TogglePACSSave)
  ON_BN_CLICKED(IDC_CHECK2,           ToggleStatistics)

  ON_CBN_SELCHANGE(IDC_CMB_NUM_CYCLES, NumCyclesChange)

  ON_MESSAGE(MSG_WF_LOAD_PLAN_DONE,    OnLoadPlanDone)
  ON_MESSAGE(MSG_WF_COMPUTE_UPDATE,    OnComputeUpdate)
  ON_MESSAGE(MSG_WF_COMPUTE_DONE,      OnComputeDone)

  ON_COMMAND_RANGE(IDC_RADIO1, IDC_RADIO2, SetProtonScalingType)

END_MESSAGE_MAP()

CPanelSetup::CPanelSetup(CPanelTabCtrl* parent) : CPanelBase(parent),
  m_numProtonsSpot      (10000),
  m_rbeAB               (1.00),
  m_resetRun            (false),
  m_engineVis           (m_appState->m_workers.engineVis),
  m_viewer              (m_appState->m_workers.viewer),
  m_workflow            (m_appState->m_workers.workflow),
  m_transportParams     (&m_appState->m_transportParams)
{

  Create(IDD_PNL_SETUP, parent);

  m_panelName = " Setup ";

  return;
}

CPanelSetup::~CPanelSetup(void)
{

  return;
}

BOOL CPanelSetup::OnInitDialog(void)
{

  CDialog::OnInitDialog();

  m_transportParams->dosePrimary = true;

  // Assign viewer rect to app client rect to left of control panel
  CRect viewerRect;
  GetParent()->GetClientRect(&viewerRect);
  MoveWindow(viewerRect, TRUE);

  // Load ComboBox selection choices
  CString scaleFactor;
  CComboBox* combo = (CComboBox*)GetDlgItem(IDC_CMB_PROTON_SCALE);
  for (int i = SCALE_FACTOR_MAX; i >= SCALE_FACTOR_MIN; --i)
  {
    double f = pow(10.0, i);
    int j = (int)f;

    scaleFactor.Format("%d", j);
    combo->AddString(scaleFactor);
  }
  combo->SetCurSel(0);

  combo = (CComboBox*)GetDlgItem(IDC_CMB_NUM_CYCLES);
  for (int i = 1; i <= NUM_CYCLES; ++i)
  {
    scaleFactor.Format("%d", i);
    combo->AddString(scaleFactor);
  }
  combo->SetCurSel(NUM_CYCLES - 1);

  CWnd* wnd;
  wnd = GetDlgItem(IDC_CHECK2);
  wnd->ShowWindow(SW_HIDE);

  wnd = GetDlgItem(IDC_EDT_NUM_PROTONS_SPOT);
  wnd->ShowWindow(SW_HIDE);

  CButton* button = (CButton*)GetDlgItem(IDC_RADIO1);
  button->SetCheck(BST_CHECKED);

  //editRBE.Create(WS_CHILD | WS_VISIBLE | WS_BORDER |  WS_EX_STATICEDGE |   ES_AUTOHSCROLL, rect, this, IDC_EDIT5);
  //editRBE.SetWindowText("1.00");

  m_msgBlinker.SubclassDlgItem(IDC_STC_COMPUTING, this);
  m_msgBlinker.ShowWindow(SW_HIDE);

#ifdef DBG_0
  CUtils utils;
  utils.ArrayTest();
#endif

  return FALSE;
}

void CPanelSetup::DoDataExchange(CDataExchange* pDX)
{
  CDialog::DoDataExchange(pDX);

  DDX_Text(pDX, IDC_EDT_NUM_PROTONS_SPOT, m_numProtonsSpot);
  DDV_MinMaxInt(pDX, m_numProtonsSpot, 1, 100000);

  if(pDX->m_bSaveAndValidate)
  {
    int index;
    DDX_CBIndex(pDX, IDC_CMB_NUM_CYCLES, index);

    // Take care of 0 based combo box
    m_transportParams->numCycles = (unsigned int) index + 1;

    DDX_CBIndex(pDX, IDC_CMB_PROTON_SCALE, index);

    if (m_transportParams->spotProtonsConstant)
    {
      m_transportParams->protonScaleFactor = m_numProtonsSpot;
    }
    else
    {
      DDX_CBIndex(pDX, IDC_CMB_PROTON_SCALE, index);

      m_transportParams->protonScaleFactor = (int)pow(10.0, (float)(SCALE_FACTOR_MAX - index));

#ifdef DBG_0 // Proton scale factor
      m_transportParams->protonScaleFactor *= 10000;
#endif

    }
  }

  return;
}

/*_____________________________________________________________________________________________

  Setting Run Config by User Selection
_______________________________________________________________________________________________*/

void CPanelSetup::ToggleDoseAux(void)
{
  m_transportParams->doseAux = !m_transportParams->doseAux;

  // If Aux dose enabled, can't run QA
  if (m_transportParams->doseAux == true)
  {
    CButton* buttonQA = (CButton*)GetDlgItem(IDC_CHK_QA);

    buttonQA->SetCheck(BST_UNCHECKED);
    m_transportParams->doseQA = false;
  }

  return;
}

void CPanelSetup::ToggleDij(void)
{
  m_transportParams->dijCalc = !m_transportParams->dijCalc;

  // If Dij enabled, can't run QA
  if (m_transportParams->dijCalc == true)
  {
    CButton* buttonQA = (CButton*)GetDlgItem(IDC_CHK_QA);
    buttonQA->SetCheck(BST_UNCHECKED);
    m_transportParams->doseQA = false;
  }

  return;
}

void CPanelSetup::ToggleQADoses(void)
{
  m_transportParams->doseQA = !m_transportParams->doseQA;

  // If QA enabled, can't run Aux & Dij
  if (m_transportParams->doseQA == true)
  {
    CButton* button;

    button = (CButton*)GetDlgItem(IDC_CHK_AUX);
    button->SetCheck(BST_UNCHECKED);
    m_transportParams->doseAux = false;

    button = (CButton*)GetDlgItem(IDC_CHK_DIJ);
    button->SetCheck(BST_UNCHECKED);
    m_transportParams->dijCalc = false;

    //// QA doses don't get saved to PACS so disable
    //button = (CButton*)GetDlgItem(IDC_CHK_PACS);
    //button->SetCheck(BST_UNCHECKED);
    //button->EnableWindow(false);
    //m_transportParams->pacsSave = false;
  }
  else // QA mode off
  {
    // Renable PACS saving if auto mode enabled
    if (m_transportParams->autoMode == true)
    {
      CButton* button;

      button = (CButton*)GetDlgItem(IDC_CHK_PACS);
      button->EnableWindow(true);
    }
  }

  return;
}

void CPanelSetup::ToggleAutoMode(void)
{
  m_transportParams->autoMode = !m_transportParams->autoMode;

  CButton* buttonPACS = (CButton*)GetDlgItem(IDC_CHK_PACS);

  // If auto mode disabled can't run save to PACS
  if (m_transportParams->autoMode == false)
  {
    buttonPACS->EnableWindow(false);
    buttonPACS->SetCheck(BST_UNCHECKED);
    m_transportParams->pacsSave = false;
  }
  else
  {
    //if (m_transportParams->doseQA == false)
    {
      buttonPACS->EnableWindow(true);
    }
  }

  return;
}

void CPanelSetup::TogglePACSSave(void)
{
  m_transportParams->pacsSave = !m_transportParams->pacsSave;

  return;
}

int configWndIDs[] =
{
  IDC_CHK_AUX,
  IDC_CHK_DIJ,
  IDC_CHK_QA,
  IDC_CHK_AUTO,
  IDC_CHK_PACS,

  IDC_CMB_PROTON_SCALE,
  IDC_CMB_NUM_CYCLES,
  IDC_EDT_NUM_PROTONS_SPOT,
  IDC_RADIO1,
  IDC_RADIO2
};

void CPanelSetup::DisableConfigWnds(void)
{
  int numEntries = (sizeof(configWndIDs) / sizeof(int));

  for (int index = 0; index < numEntries; ++index)
  {
    GetDlgItem(configWndIDs[index])->EnableWindow(FALSE);
  }

  return;
}

void CPanelSetup::ResetConfigWnds(void)
{
  int numEntries = sizeof(configWndIDs) / sizeof(int);

  for (int index = 0; index < numEntries; ++index)
  {
    GetDlgItem(configWndIDs[index])->EnableWindow(TRUE);
  }

  CWnd* wnd = GetDlgItem(IDC_CHK_PACS);
  wnd->EnableWindow(FALSE);

  CComboBox* combo;
  combo = (CComboBox*)GetDlgItem(IDC_CMB_PROTON_SCALE);
  combo->SetCurSel(0);
  combo = (CComboBox*)GetDlgItem(IDC_CMB_NUM_CYCLES);
  combo->SetCurSel(NUM_CYCLES - 1);

  //// Enable primary and aux doses 

  //button = (CButton*)GetDlgItem(IDC_CHK_AUX);
  //button->SetCheck(BST_CHECKED);
  //m_transportParams->dosePrimary = true;
  //m_transportParams->doseAux = true;

  wnd = GetDlgItem(IDC_CHECK2);
  wnd->ShowWindow(SW_HIDE);

  wnd = GetDlgItem(IDC_EDT_NUM_PROTONS_SPOT);
  wnd->ShowWindow(SW_HIDE);

  CButton* button;
  button = (CButton*)GetDlgItem(IDC_RADIO1);
  button->SetCheck(BST_CHECKED);

  return;
}

void CPanelSetup::ClearGUIPlanInfo(void)
{
  CString empty = "";
  GetDlgItem(IDC_STC_ID)->SetWindowTextA(empty);
  GetDlgItem(IDC_STC_PLAN_NAME)->SetWindowTextA(empty);
  GetDlgItem(IDC_STC_SERIES_DESCR)->SetWindowTextA(empty);
  GetDlgItem(IDC_STC_PLAN)->SetWindowTextA(empty);
  GetDlgItem(IDC_STC_NUM_PROTONS_PLAN)->SetWindowTextA(empty);

  return;
}



/*_____________________________________________________________________________________________

  Input of data files from disk files or PACS
_______________________________________________________________________________________________*/

void CPanelSetup::SelectPlanJSON(void)
{

  CFileDialog fileDlg(TRUE, "*.json", NULL,
    OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, "JSON (*.json)|*.json||");

  UINT prevMouseResponder = m_viewer->SetMouseResponder(MOUSE_RSPNDR_NOOP);

  if (fileDlg.DoModal() == IDOK)
  {
    m_transportParams->pathNamePlan = fileDlg.GetFolderPath();
    m_transportParams->fileNamePlan = fileDlg.GetFileName();

    OnPlanSelected(false);
  }
  else
  {
    m_viewer->SetMouseResponder(prevMouseResponder);
  }


  return;
}

void CPanelSetup::SelectPlanPlayer(void)
{
  CFileDialog fileDlg(TRUE, "*", NULL,
    OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, "DICOM Plan (*)|*||");

  UINT prevMouseResponder = m_viewer->SetMouseResponder(MOUSE_RSPNDR_NOOP);

  if (fileDlg.DoModal() == IDOK)
  {
    m_transportParams->pathNamePlan = fileDlg.GetFolderPath();
    m_transportParams->fileNamePlan = fileDlg.GetFileName();

    OnPlanSelected(true);
  }
  else
  {
    m_viewer->SetMouseResponder(prevMouseResponder);
  }

  return;
}

void CPanelSetup::SelectPlanPACS(void)
{

  CDlgPlanGetPacs pacsDlg(m_appState->m_workers.pacsPipe);

  if (pacsDlg.DoModal() == IDOK)
  {
    m_transportParams->pathNamePlan = pacsDlg.GetPathNamePlan();
    m_transportParams->fileNamePlan = pacsDlg.GetFileNamePlan();

    OnPlanSelected(false);
  }

  return;
}

void CPanelSetup::OnPlanSelected(bool runPlayer)
{

  m_engineVis->ClearResources();

  UpdateData();

  // May or may not be visible but want hidden now
  GetDlgItem(IDC_BTN_RUN_PLAN)->ShowWindow(SW_HIDE);

  //GetProtonScaleFactor();

  m_resetRun = false;
  m_msgBlinker.StartTimer();

  m_workflow->ComputePlan(m_transportParams, this, runPlayer);

  DisableConfigWnds();

  return;
}

void CPanelSetup::OnResetRerun(void)
{

  //GetProtonScaleFactor();

  CWnd* wnd = GetDlgItem(IDC_BTN_RUN_PLAN);
  if (m_resetRun == false)
  {
    ResetConfigWnds();

    m_resetRun = true;
    wnd->SetWindowTextA("Rerun Plan");
  }
  else
  {

    UpdateData();

    wnd->ShowWindow(SW_HIDE);
    wnd->SetWindowTextA("Reset Config");

    m_msgBlinker.StartTimer();

    m_workflow->ComputeRerun(m_transportParams);
  }

  return;
}


LRESULT CPanelSetup::OnComputeUpdate(WPARAM wParam, LPARAM lParam)
{

  LPCTSTR msgUpdate = (LPCTSTR)wParam;

  m_msgBlinker.SetWindowText(msgUpdate);

  return TRUE;
}

LRESULT CPanelSetup::OnComputeDone(WPARAM wParam, LPARAM lParam)
{

  m_msgBlinker.StopTimer();

  if (wParam == WF_TASK_PASS)
  {
    GetDlgItem(IDC_BTN_RUN_PLAN)->ShowWindow(SW_SHOW);

    if (!m_transportParams->autoMode)
    {
      ExecutePostComputeManual();
    }
  }
  else
  {
    LPCTSTR msgError = (LPCTSTR) lParam;
    AfxMessageBox(msgError);

    ResetConfigWnds();
    ClearGUIPlanInfo();
  }

  m_resetRun = false;

  return TRUE;
}



/*_____________________________________________________________________________________________

  Input of data files from disk files or PACS
_______________________________________________________________________________________________*/


LRESULT CPanelSetup::OnLoadPlanDone(WPARAM wParam, LPARAM lParam)
{
  CIonPlan* ionPlan = (CIonPlan*)wParam;

  CString infoText;
  infoText.Format(" %s", ionPlan->m_patientID);
  GetDlgItem(IDC_STC_ID)->SetWindowTextA(infoText);

  infoText.Format(" %s", ionPlan->m_planName);
  GetDlgItem(IDC_STC_PLAN_NAME)->SetWindowTextA(infoText);

  infoText.Format(" %s", ionPlan->m_seriesDescription);
  GetDlgItem(IDC_STC_SERIES_DESCR)->SetWindowTextA(infoText);

  infoText.Format(" RSs %d   Apertures %d   Beams %d   Spots %d", ionPlan->m_arrayDevices.m_numItems - ionPlan->m_arrayApertures.m_numItems,
    ionPlan->m_arrayApertures.m_numItems, ionPlan->m_arrayBeams.m_numItems, ionPlan->m_arraySpots.m_numItems);
  GetDlgItem(IDC_STC_PLAN)->SetWindowTextA(infoText);

  infoText.Format("%e", ionPlan->m_numProtonsPlan);
  GetDlgItem(IDC_STC_NUM_PROTONS_PLAN)->SetWindowTextA(infoText);

  infoText.Format("# Protons/Cycle (Computed)\r\n  %e", (float)m_transportParams->numProtonsTransport);
  GetDlgItem(IDC_STC_NUM_PROTONS)->SetWindowTextA(infoText);

  return TRUE;
}


void CPanelSetup::ExecutePostComputeManual(void)
{

  m_engineVis->LoadTransportInfo();

  APPLOGT("Post Calc Complete")

  GetParent()->PostMessageA(MSG_TAB_CHANGE_CMD, (WPARAM) &m_transportParams, 0);

  return;
}

void CPanelSetup::ToggleStatistics(void)
{
  m_transportParams->statsCalc = !m_transportParams->statsCalc;

  return;
}

void CPanelSetup::NumCyclesChange(void)
{
  CButton* button = (CButton*)GetDlgItem(IDC_CHECK2);

  CComboBox* combo = (CComboBox*) GetDlgItem(IDC_CMB_NUM_CYCLES);

  int index = combo->GetCurSel();

  if (index == 0)
  {
    button->ShowWindow(SW_HIDE);
    button->SetCheck(BST_UNCHECKED);
    m_transportParams->statsCalc = false;
  }
  else
  {
    button->ShowWindow(SW_SHOW);
  }

  return;
}

void CPanelSetup::SetProtonScalingType(UINT nID)
{
  CWnd* wnd0 = GetDlgItem(IDC_EDT_NUM_PROTONS_SPOT);
  CWnd* wnd1 = GetDlgItem(IDC_CMB_PROTON_SCALE);
  CWnd* wnd2 = GetDlgItem(IDC_STATIC);

  if (nID == IDC_RADIO1)
  {
    wnd0->ShowWindow(SW_HIDE);
    wnd1->ShowWindow(SW_SHOW);
    wnd2->SetWindowTextA("Proton Scale");

    m_transportParams->spotProtonsConstant = false;
  }
  else
  {
    wnd0->ShowWindow(SW_SHOW);
    wnd1->ShowWindow(SW_HIDE);
    wnd2->SetWindowTextA("Protons/Spot");

    m_transportParams->spotProtonsConstant = true;
  }

  return;
}

void CPanelSetup::GetProtonScaleFactor(void)
{

  if (m_transportParams->spotProtonsConstant)
  {
    m_transportParams->protonScaleFactor = m_numProtonsSpot;
  }
  else
  {
    CComboBox* combo = (CComboBox*)GetDlgItem(IDC_CMB_PROTON_SCALE);

    int index = combo->GetCurSel();

    m_transportParams->protonScaleFactor = (int)pow(10.0, (float)(SCALE_FACTOR_MAX - index));
  }

  return;
}


/*_____________________________________________________________________________________________

  Development Routines - Proceed at your own risk
_______________________________________________________________________________________________*/

void CPanelSetup::SaveApertures(void)
{
  //ionPlan->SaveApertures();

  return;
}

void CPanelSetup::ResampleDose(void)
{
  // m_engineVis->ResampleDose();
  m_engineVis->Render();

  // m_engineTransport->CalcDoseProfile(NULL, m_engineVis->GetResampledDoseGrid());

  return;
}
