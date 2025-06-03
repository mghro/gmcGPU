#include "stdafx.h"
#include <memory>
#include <process.h>
#include "resource.h"
#include "UniArray.h"
#include "Logger.h"
#include "IonPlan.h"
#include "PhantomQA.h"
#include "DicomDose.h"
#include "BitmapUtils.h"
#include "DlgResultsSaveDicom.h"
#include "DlgSaveQABeams.h"
#include "DlgGammaThresholds.h"
#include "Workflow.h"
#include "Viewer.h"
#include "PanelBase.h"
#include "PanelResults.h"

#define SLIDER_DOSE_MAX   (1000)

extern CString g_doseTypes[NUM_DOSE_TYPES];

enum
{
  RESULT_STAT_DOSE_STAT = 0,
  RESULT_STAT_AVRG,
  RESULT_STAT_VARIANCE,

  NUM_RESULTS_STAT
};

static CString s_fieldLabelsStats[NUM_RESULTS_STAT] =
{
  "Dose",
  "DoseAvrg",
  "DoseVarnc"
};

enum
{
  LABEL_MIN_ABS = 0,
  LABEL_MAX_ABS,
  LABEL_MIN_PRCNT,
  LABEL_MAX_PRCNT,

  NUM_MINMAX_LABELS
};

CString s_minmaxLabels[NUM_MINMAX_LABELS] =
{
  "Vis Min Abs",
  "Vis Max Abs",
  "Vis Min %",
  "Vis Max %"
};

BEGIN_MESSAGE_MAP(CPanelResults, CDialog)

  ON_BN_CLICKED(IDC_BTN_ZOOM_UP,         ZoomUp)
  ON_BN_CLICKED(IDC_BTN_ZOOM_DOWN,       ZoomDown)
  ON_BN_CLICKED(IDC_BTN_DOSE_DICOM,      OpenDoseDicom)
  ON_BN_CLICKED(IDC_BTN_DICOM_SAVE,      SaveDoseDisk)
  ON_BN_CLICKED(IDC_BTN_PACS,            SaveDosePACS)
  ON_BN_CLICKED(IDC_BTN_DOSE_DIFF,       DisplayDoseDiff)
  ON_BN_CLICKED(IDC_BTN_ENERGY,          CalcEnergy)
  ON_BN_CLICKED(IDC_BTN_GAMMA,           CalcGamma)
  ON_BN_CLICKED(IDC_BTN_HISTOGRAMS,      CalcHistograms)
  ON_BN_CLICKED(IDC_BTN_PROFILE,         CalcProfiles)
  ON_BN_CLICKED(IDC_BTN_DOSE_SUM_BEAMS,  SumBeamDoses)
  ON_BN_CLICKED(IDC_BTN_DOSE_SUM_EXTRNL, SumExternalDose)
  ON_BN_CLICKED(IDC_BTN_STRUCTS,         DisplayStructs)

  ON_CBN_SELCHANGE(IDC_CMB_RESULTS_PRIM, SetResultsTypePrim)
  ON_CBN_SELCHANGE(IDC_CMB_RESULTS_SEC,  SetResultsTypeSec)

  ON_MESSAGE(MSG_WF_CALC_GAMMA_DONE, OnGammaDone)

  ON_WM_HSCROLL()

  ON_COMMAND_RANGE(IDC_BTN_TILE1, IDC_BTN_TILE4, OnTileConfigButton)
  ON_COMMAND_RANGE(IDC_RADIO1,    IDC_RADIO4,    SetMouseBehavior)
  ON_COMMAND_RANGE(IDC_RADIO5,    IDC_RADIO7,    SetDisplayBoundsType)
  ON_COMMAND_RANGE(IDC_RADIO8,    IDC_RADIO10,   SetViewAxis)
  ON_COMMAND_RANGE(IDC_RADIO11,   IDC_RADIO12,   SetImageType)

END_MESSAGE_MAP()

CPanelResults::CPanelResults(CPanelTabCtrl* parent) : CPanelBase(parent),
  m_viewer           (m_appState->m_workers.viewer),
  m_engineVis        (m_appState->m_workers.engineVis),
  m_enginePostCalc   (m_appState->m_workers.enginePostCalc),
  m_workflow         (m_appState->m_workers.workflow),
  m_transportParams  (&m_appState->m_transportParams),
  m_resultsStrings   (g_doseTypes),
  m_statusDoseExtrnl (false),
  m_gammaCalc        (false),
  m_displayDoseMax   (100.0),
  m_displayDoseMin   (0.0),
  m_boundsType       (DISPLAY_BOUNDS_PRIM),
  m_tileConfigIndex  (0),
  m_zoom             (1.0),
  m_ionPlan          (NULL)
{
  Create(IDD_PNL_RESULTS, parent);

  m_panelName = " Results ";

  return;
}

CPanelResults::~CPanelResults(void)
{
  return;
}

BOOL CPanelResults::OnInitDialog(void)
{

  CDialog::OnInitDialog();

  // Control Panel windows the viewer updates
  m_uiWnds[UI_WND_WIN_LEVEL_MIN]   = NULL;
  m_uiWnds[UI_WND_WIN_LEVEL_MAX]   = NULL;
  m_uiWnds[UI_WND_PT_DOSE_RESULTS] = GetDlgItem(IDC_EDIT1);
   
  // Init the dose slider
  CSliderCtrl* slider;
  slider = (CSliderCtrl*)GetDlgItem(IDC_SLIDER_DOSE);
  slider->SetRange(0, SLIDER_DOSE_MAX, TRUE);
  slider->SetPos(0);

  CButton* button;
  button = (CButton*) GetDlgItem(IDC_RADIO1);
  button->SetCheck(BST_CHECKED);

  button = (CButton*)GetDlgItem(IDC_RADIO5);
  button->SetCheck(BST_CHECKED);

  button = (CButton*)GetDlgItem(IDC_RADIO8);
  button->SetCheck(BST_CHECKED);

  button = (CButton*)GetDlgItem(IDC_RADIO11);
  button->SetCheck(BST_CHECKED);

  // Scroll bar on mouse move window visible
  GetDlgItem(IDC_EDIT1)->ShowScrollBar(SB_VERT, TRUE);

  GetDlgItem(IDC_EDT_MIN)->SetWindowText("0.0");

  // Set the dose color scale bitmap
  m_bitmap.LoadBitmap(IDB_BITMAP1);
  HBITMAP hmp = (HBITMAP)m_bitmap.GetSafeHandle();
  CStatic* stc = (CStatic*) GetDlgItem(IDC_STC_COLOR_SCALE);
  stc->ModifyStyle(SS_TYPEMASK, SS_BITMAP | SS_REALSIZECONTROL  | SS_REALSIZEIMAGE);
  stc->SetBitmap(HBITMAP(m_bitmap));

  SetWidgetVisSecondary(SW_HIDE);

  button = (CButton*)GetDlgItem(IDC_BTN_TILE1);
  LoadTransparentBitmap(button->GetDC(), IDB_BITMAP2, &m_bitmapTile1);
  button->SetBitmap(HBITMAP(m_bitmapTile1));

  button = (CButton*)GetDlgItem(IDC_BTN_TILE2);
  LoadTransparentBitmap(button->GetDC(), IDB_BITMAP3, &m_bitmapTile4);
  button->SetBitmap(HBITMAP(m_bitmapTile4));

  button = (CButton*)GetDlgItem(IDC_BTN_TILE3);
  LoadTransparentBitmap(button->GetDC(), IDB_BITMAP4, &m_bitmapTile2H);
  button->SetBitmap(HBITMAP(m_bitmapTile2H));

  button = (CButton*)GetDlgItem(IDC_BTN_TILE4);
  LoadTransparentBitmap(button->GetDC(), IDB_BITMAP5, &m_bitmapTile2V);
  button->SetBitmap(HBITMAP(m_bitmapTile2V));

  button = (CButton*)GetDlgItem(IDC_BTN_ZOOM_UP);
  LoadTransparentBitmap(button->GetDC(), IDB_BITMAP6, &m_bitmapZoomUp);
  button->SetBitmap(HBITMAP(m_bitmapZoomUp));

  button = (CButton*)GetDlgItem(IDC_BTN_ZOOM_DOWN);
  LoadTransparentBitmap(button->GetDC(), IDB_BITMAP7, &m_bitmapZoomDown);
  button->SetBitmap(HBITMAP(m_bitmapZoomDown));

  m_msgBlinker.SubclassDlgItem(IDC_STC_COMPUTING, this);
  m_msgBlinker.ShowWindow(SW_HIDE);

  return FALSE;
}

void CPanelResults::DoDataExchange(CDataExchange* pDX)
{
  CDialog::DoDataExchange(pDX);

  if(pDX->m_bSaveAndValidate)
  {
    DDX_Text(pDX, IDC_EDT_MIN, m_displayDoseMin);
    DDX_Text(pDX, IDC_EDT_MAX, m_displayDoseMax);
  }

  return;
}

void CPanelResults::OnTabEnter(int prevPanel, void* data)
{
  CUniArray<CString> fieldLabelsQA;

  m_ionPlan = m_appState->m_compObjects.ionPlan;

  if (m_transportParams->statsCalc)
  {
    m_resultsStrings  = s_fieldLabelsStats;
    m_numResultsTypes = NUM_RESULTS_STAT;
  }
  else if (m_transportParams->doseQA)
  {
    int numLabels = m_ionPlan->m_arrayBeams.m_numItems + 1;

    fieldLabelsQA.Alloc(numLabels);
    m_resultsStrings  = fieldLabelsQA(0);
    m_numResultsTypes = numLabels;

    *fieldLabelsQA.m_pItemFirst = g_doseTypes[0];
    int beamIndex = 0;

    forArrayIN(fieldLabelsQA, label, 1, m_ionPlan->m_arrayBeams.m_numItems)
    {
      label->Format("QA_Beam%2d", beamIndex);
      ++beamIndex;
    }
  }
  else
  {
    m_resultsStrings  = g_doseTypes;
    if (m_transportParams->doseAux)
    {
      m_numResultsTypes = NUM_DOSE_TYPES;
    }
    else
    {
      m_numResultsTypes = 1;
    }
  }

  CComboBox* comboPrim = (CComboBox*)GetDlgItem(IDC_CMB_RESULTS_PRIM);
  CComboBox* comboSec  = (CComboBox*)GetDlgItem(IDC_CMB_RESULTS_SEC);

  comboPrim->ResetContent();
  comboSec->ResetContent();

  for (int index = 0; index < m_numResultsTypes; ++index)
  {
    comboPrim->AddString(m_resultsStrings[index]);
    comboSec->AddString(m_resultsStrings[index]);
  }

  comboPrim->SetCurSel(0);
  comboSec->SetCurSel(1);

  m_statusDoseExtrnl = false;
  m_gammaCalc        = false;

  CString doseString;
  CWnd* wnd;

  m_displayDoseMax = m_engineVis->GetMaxDosePrimary();
  doseString.Format("Max Prim: %6.2f", m_displayDoseMax);
  wnd = GetDlgItem(IDC_STC_DOSE_MAX_PRIM);
  wnd->SetWindowTextA(doseString);

  doseString.Format("%5.2f", m_displayDoseMax);
  GetDlgItem(IDC_EDT_MAX)->SetWindowTextA(doseString);

  m_displayDoseMin = 0.0;
  doseString.Format("%5.2f", 0.0);
  GetDlgItem(IDC_EDT_MIN)->SetWindowTextA(doseString);

  CSliderCtrl* slider;
  slider = (CSliderCtrl*)GetDlgItem(IDC_SLIDER_DOSE);
  slider->SetPos(int (m_displayDoseMin * SLIDER_DOSE_MAX /m_displayDoseMax));

  GetDlgItem(IDC_STC_DOSE_MIN)->SetWindowTextA(s_minmaxLabels[LABEL_MIN_ABS]);
  GetDlgItem(IDC_STC_DOSE_MAX)->SetWindowTextA(s_minmaxLabels[LABEL_MAX_ABS]);

  GetDlgItem(IDC_EDIT2)->SetWindowTextA("");

  m_boundsType    = DISPLAY_BOUNDS_PRIM;
  CButton* button = (CButton*)GetDlgItem(IDC_RADIO5);
  button->SetCheck(BST_CHECKED);
  SetWidgetVisSecondary(SW_HIDE);

  m_tileConfigIndex = 0;
  m_viewer->SetNewVolume(m_ionPlan->m_voxGridHU.dims, m_ionPlan->m_voxGridHU.voxelSize);

  // Set mouse scroll radio to on, other mouse radio buttons to off (mouse functionality set in SetNewVolume)
  button = (CButton*)GetDlgItem(IDC_RADIO1);
  button->SetCheck(BST_CHECKED);
  for (int indexButton = IDC_RADIO2; indexButton <= IDC_RADIO4; ++indexButton)
  {
    button = (CButton*)GetDlgItem(indexButton);
    button->SetCheck(BST_UNCHECKED);
  }

  InitViewAxis();

  m_engineVis->Render();

  APPLOGT("Display Complete")

  return;
}

void CPanelResults::OnTileConfigButton(UINT buttonIndex)
{

  int configIndex = buttonIndex - IDC_BTN_TILE1;

  if ((configIndex > 1) && (m_statusDoseExtrnl == false))
  {
    AfxMessageBox("No Secondary Dose.");
    return;
  }

  SetTileConfig(configIndex);

  m_engineVis->Render();

  return;
}

void CPanelResults::SetTileConfig(UINT configIndex)
{

  m_tileConfigIndex = configIndex;

  m_viewer->SetTileConfig(m_tileConfigIndex);

  int visStatus = m_tileConfigIndex > 0 ? SW_SHOW : SW_HIDE;
  SetWidgetVisSecondary(visStatus);

  return;
}

void CPanelResults::SetWidgetVisSecondary(int visStatus)
{
  GetDlgItem(IDC_RADIO6)->ShowWindow(visStatus);
  GetDlgItem(IDC_RADIO7)->ShowWindow(visStatus);
  GetDlgItem(IDC_CMB_RESULTS_SEC)->ShowWindow(visStatus);

  GetDlgItem(IDC_STC_DOSE_MAX_SEC)->ShowWindow(visStatus);

  return;
}

void CPanelResults::InitViewAxis(void)
{
  //m_viewer->SetViewAxisPrimary(0);

  CButton* button;

  button = (CButton*)GetDlgItem(IDC_RADIO8);
  button->SetCheck(BST_CHECKED);
  button = (CButton*)GetDlgItem(IDC_RADIO9);
  button->SetCheck(BST_UNCHECKED);
  button = (CButton*)GetDlgItem(IDC_RADIO10);
  button->SetCheck(BST_UNCHECKED);

  return;
}

void CPanelResults::SetViewAxis(UINT position)
{

  int index = position - IDC_RADIO8;

  m_viewer->SetViewAxisPrimary(index);

  m_engineVis->Render();

  return;
}

void CPanelResults::SetResultsTypePrim(void)
{
  CComboBox* combo = (CComboBox*)GetDlgItem(IDC_CMB_RESULTS_PRIM);
  int index = combo->GetCurSel();

  m_engineVis->SetResultsType(0, index);
  m_displayDoseMax = m_engineVis->GetMaxDosePrimary();

  CString doseString;
  doseString.Format("Max Prim: %6.2f", m_displayDoseMax);
  GetDlgItem(IDC_STC_DOSE_MAX_PRIM)->SetWindowTextA(doseString);

  UpdateResultsView();

  return;
}

void CPanelResults::SetResultsTypeSec(void)
{
  CComboBox* combo = (CComboBox*)GetDlgItem(IDC_CMB_RESULTS_SEC);
 int index = combo->GetCurSel();

  m_engineVis->SetResultsType(1, index);
  m_displayDoseMax = m_engineVis->GetMaxDoseSecondary();

  CString doseString;
  doseString.Format("Max Sec: %6.2f", m_displayDoseMax);
  GetDlgItem(IDC_STC_DOSE_MAX_SEC)->SetWindowTextA(doseString);

  UpdateResultsView();

  return;
}

void CPanelResults::UpdateResultsView(void)
{
  CString doseString;

  if (m_boundsType == DISPLAY_BOUNDS_INDPNDNT)
  {
    doseString.Format("%5.2f", 100.0);

    GetDlgItem(IDC_STC_DOSE_MIN)->SetWindowTextA(s_minmaxLabels[LABEL_MIN_PRCNT]);
    GetDlgItem(IDC_STC_DOSE_MAX)->SetWindowTextA(s_minmaxLabels[LABEL_MAX_PRCNT]);
  }
  else
  {
    doseString.Format("%5.2f", m_displayDoseMax);

    GetDlgItem(IDC_STC_DOSE_MIN)->SetWindowTextA(s_minmaxLabels[LABEL_MIN_ABS]);
    GetDlgItem(IDC_STC_DOSE_MAX)->SetWindowTextA(s_minmaxLabels[LABEL_MAX_ABS]);
  }
  
  GetDlgItem(IDC_EDT_MAX)->SetWindowTextA(doseString);

  CSliderCtrl* slider;
  slider = (CSliderCtrl*)GetDlgItem(IDC_SLIDER_DOSE);
  slider->SetPos(int(m_displayDoseMin * SLIDER_DOSE_MAX / m_displayDoseMax));

  m_engineVis->Render();

  return;
}

void CPanelResults::SetDisplayBoundsType(UINT position)
{

  m_boundsType = position - IDC_RADIO5;

  m_engineVis->SetDisplayBoundsType(m_boundsType);

  switch (m_boundsType)
  {
  case DISPLAY_BOUNDS_PRIM:
    m_displayDoseMax = m_engineVis->GetMaxDosePrimary();

    GetDlgItem(IDC_STC_DOSE_MIN)->SetWindowTextA(s_minmaxLabels[LABEL_MIN_ABS]);
    GetDlgItem(IDC_STC_DOSE_MAX)->SetWindowTextA(s_minmaxLabels[LABEL_MAX_ABS]);
    break;

  case DISPLAY_BOUNDS_SEC:
    m_displayDoseMax = m_engineVis->GetMaxDoseSecondary();

    GetDlgItem(IDC_STC_DOSE_MIN)->SetWindowTextA(s_minmaxLabels[LABEL_MIN_ABS]);
    GetDlgItem(IDC_STC_DOSE_MAX)->SetWindowTextA(s_minmaxLabels[LABEL_MAX_ABS]);
    break;

  case DISPLAY_BOUNDS_INDPNDNT:
    m_displayDoseMax = 100.0;

    GetDlgItem(IDC_STC_DOSE_MIN)->SetWindowTextA(s_minmaxLabels[LABEL_MIN_PRCNT]);
    GetDlgItem(IDC_STC_DOSE_MAX)->SetWindowTextA(s_minmaxLabels[LABEL_MAX_PRCNT]);
    break;
  }

  CString doseString;
  doseString.Format("%5.2f", m_displayDoseMax);
  GetDlgItem(IDC_EDT_MAX)->SetWindowTextA(doseString);

  CSliderCtrl* slider;
  slider = (CSliderCtrl*)GetDlgItem(IDC_SLIDER_DOSE);
  slider->SetPos(int(m_displayDoseMin * SLIDER_DOSE_MAX / m_displayDoseMax));

  m_engineVis->Render();

  return;
}

void CPanelResults::OnOK(void)
{
  CWnd* wnd = GetFocus();

  if ( (wnd == GetDlgItem(IDC_EDT_MAX)) || (wnd == GetDlgItem(IDC_EDT_MIN)) )
  {
    UpdateData(TRUE);

    // If invalid entry set bounds to default
    if (m_displayDoseMin < 0.0 || m_displayDoseMin > m_displayDoseMax)
    {
      m_displayDoseMin = 0.0;

      switch (m_boundsType)
      {
      case DISPLAY_BOUNDS_PRIM:
        m_displayDoseMax = m_engineVis->GetMaxDosePrimary();
        break;
      case DISPLAY_BOUNDS_SEC:
        m_displayDoseMax = m_engineVis->GetMaxDoseSecondary();
        break;
      case DISPLAY_BOUNDS_INDPNDNT:
        m_displayDoseMax = 100.0;
        break;
      }

      m_displayDoseMax = m_engineVis->GetMaxDosePrimary();

      if (m_boundsType == DISPLAY_BOUNDS_INDPNDNT)
      {
        CString doseString;
        doseString.Format("%5.2f", m_displayDoseMin);
        GetDlgItem(IDC_EDT_MIN)->SetWindowTextA(doseString);

        doseString.Format("%5.2f", m_displayDoseMax);
        GetDlgItem(IDC_EDT_MAX)->SetWindowTextA(doseString);
      }
      else
      {
        CString doseString;

        doseString.Format("%5.2f", 100.0 * (m_displayDoseMin/m_displayDoseMax));
        GetDlgItem(IDC_EDT_MIN)->SetWindowTextA(doseString);

        doseString.Format("%5.2f", 100.0);
        GetDlgItem(IDC_EDT_MAX)->SetWindowTextA(doseString);
      }
    }

    CSliderCtrl* slider;
    slider = (CSliderCtrl*)GetDlgItem(IDC_SLIDER_DOSE);
    slider->SetPos(int(m_displayDoseMin * SLIDER_DOSE_MAX / m_displayDoseMax));

    m_engineVis->SetDoseBoundsUser(m_displayDoseMin, m_displayDoseMax);
    m_engineVis->Render();
  }

  return;
}

void CPanelResults::OnHScroll(UINT action, UINT pos, CScrollBar* scrollBar)
{
  int position = (int)pos;

  if (action == SB_ENDSCROLL)
  {
    return;
  }

  CSliderCtrl* slider = (CSliderCtrl*)scrollBar;

  int id = slider->GetDlgCtrlID();

  float level = ((float)position) / SLIDER_DOSE_MAX;

  m_displayDoseMin = m_displayDoseMax * level;

  m_engineVis->SetDoseBoundsUser(m_displayDoseMin, m_displayDoseMax);
  m_engineVis->Render();

  CString min;
  if (m_boundsType == DISPLAY_BOUNDS_INDPNDNT)
  {
    min.Format("%4.1f", level * 100.0);
  }
  else
  {
    min.Format("%4.1f", level * m_displayDoseMax);
  }
  
  GetDlgItem(IDC_EDT_MIN)->SetWindowText(min);

  slider->SetPos(pos);

  return;
}

void CPanelResults::ZoomDown(void)
{

  m_zoom *= 0.5;
  m_viewer->SetZoomFactor(m_zoom);
  m_engineVis->Render();

  return;
}

void CPanelResults::ZoomUp(void)
{

  m_zoom *= 2.0;
  m_viewer->SetZoomFactor(m_zoom);
  m_engineVis->Render();

  return;
}

void CPanelResults::SetImageType(UINT position)
{

  int viewImageType = position - IDC_RADIO11;

  m_viewer->SetWinLevelType(viewImageType);
  m_engineVis->SetImageType(viewImageType);
  m_engineVis->Render();

  return;
}

void CPanelResults::ToggleSynchStatus(void)
{
  m_viewer->ToggleSynchStatus();

  return;
}

void CPanelResults::SetMouseBehavior(UINT nID)
{
  m_viewer->SetMouseResponder(nID - IDC_RADIO1);

  m_engineVis->Render();

  return;
}

void CPanelResults::OpenDoseDicom(void)
{
  CFileDialog fileDlg(TRUE, "*.dcm", NULL, 0, "Dose (*.dcm) |*dcm|All Files (*.*)|*.*||");

  if (fileDlg.DoModal() != IDOK)
  {
    return;
  }

  float doseMax = m_enginePostCalc->LoadExternalDoseDicom(fileDlg.GetPathName());
  m_engineVis->LoadExternalDoseDicom();
  
  if (m_tileConfigIndex == 0)
  {
    SetTileConfig(1);
  }
  else
  {
    SetWidgetVisSecondary(SW_SHOW);
  }

  //InitViewAxis();

  m_engineVis->Render();

  CString doseString;
  doseString.Format("Max Sec: %6.2f", doseMax);
  CWnd* wnd = GetDlgItem(IDC_STC_DOSE_MAX_SEC);
  wnd->SetWindowTextA(doseString);

  if (m_statusDoseExtrnl == false)
  {
    wnd->ShowWindow(SW_SHOW);

    CComboBox* combo;
    combo = (CComboBox*)GetDlgItem(IDC_CMB_RESULTS_PRIM);
    combo->AddString("External");

    combo = (CComboBox*)GetDlgItem(IDC_CMB_RESULTS_SEC);
    combo->AddString("External");
    combo->SetCurSel(m_numResultsTypes);

    ++m_numResultsTypes;

    m_statusDoseExtrnl = true;
  }

  return;
}

void CPanelResults::CalcEnergy(void)
{
  CString results;
  SEnergies energies;

  m_enginePostCalc->CalcEnergy(&energies);

  results.Format("Energies\r\n\r\n%6.4e   Plan Protons\r\n%6.4e   GMC Total\r\n%6.4e   GMC Devices\r\n%6.4e   GMC Patient\r\n",
    energies.energyPlanProtons, energies.energyCompTotal, energies.energyCompDevices, energies.energyCompPatient);

  if (energies.energyExternalPatient > 0.0)
  {
    CString external;
    external.Format("%6.4e   External Patient", energies.energyExternalPatient);
    results += external;
  }

  GetDlgItem(IDC_EDIT2)->SetWindowTextA(results);

  return;
}

void CPanelResults::CalcGamma(void)
{

  if (m_statusDoseExtrnl == false)
  {
    AfxMessageBox("No External Dose Loaded.");
    return;
  }

  m_gammaThresholds = { 3.0, 3.0, 10.0 };

  CDlgGammaThresholds dlgGammaThresholds(&m_gammaThresholds);

  if (dlgGammaThresholds.DoModal() != IDCANCEL)
  {
    CString thresholds;
    thresholds.Format("  Gamma Thresholds\r\nDistance(mm)    %5.2f\r\nDose(%%)           %5.2f\r\nDoseMin(%%)      %5.2f\r\n\r\n",
      m_gammaThresholds.threshDistanceMM, m_gammaThresholds.threshDosePrcnt, m_gammaThresholds.threshDoseMinPrcnt);
    GetDlgItem(IDC_EDIT2)->SetWindowTextA(thresholds);

    m_msgBlinker.StartTimer();

    m_workflow->ComputeGamma(&m_gammaThresholds, this);

    m_gammaCalc = true;
  }

  return;
}

LRESULT CPanelResults::OnGammaDone(WPARAM wParam, LPARAM lParam)
{
  m_msgBlinker.StopTimer();

  SGammaResults* gammaResults = (SGammaResults*)wParam;

  // Display gamma results
  CWnd* wnd = GetDlgItem(IDC_EDIT2);
  CString thresholds;
  wnd->GetWindowText(thresholds);
  CString results;
  results.Format("  Gamma Results\r\nNum Iterations         %2d\r\nFraction Converged  %6.4f", 
    gammaResults->numIterations, gammaResults->fractionConverged);
  results = thresholds + results;

  GetDlgItem(IDC_EDIT2)->SetWindowTextA(results);

  CString stringGamma("Gamma");

  // Add gamma to results combo lists if not there
  CComboBox* combo = (CComboBox*)GetDlgItem(IDC_CMB_RESULTS_PRIM);
  if (combo->FindStringExact(0, stringGamma) == CB_ERR)
  {

    combo->AddString(stringGamma);

    combo = (CComboBox*)GetDlgItem(IDC_CMB_RESULTS_SEC);
    combo->AddString(stringGamma);
    combo->SetCurSel(m_numResultsTypes);

    ++m_numResultsTypes;
  }

  m_engineVis->LoadGamma();

  // Set secondary dose max to gamma max
  CString doseString;
  doseString.Format("Max Sec: %6.2f", gammaResults->gammaMax);
  wnd = GetDlgItem(IDC_STC_DOSE_MAX_SEC);
  wnd->SetWindowTextA(doseString);

  GetDlgItem(IDC_EDT_MAX)->SetWindowText("1.0");
  m_engineVis->SetDoseBoundsUser(0.0, 1.0);

  // Set dose display bounds type to independent, Render()
  ((CButton*)GetDlgItem(IDC_RADIO5))->SetCheck(false);
  ((CButton*)GetDlgItem(IDC_RADIO6))->SetCheck(false);
  ((CButton*)GetDlgItem(IDC_RADIO7))->SetCheck(true);
  SetDisplayBoundsType(IDC_RADIO7);

  return TRUE;
}

void CPanelResults::SaveDoseDisk(void)
{

  if (m_transportParams->doseQA)
  {
    SaveDoseDiskQA();
  }
  else
  {
    SaveFilesDicom(false);
  }

  return;
}

void CPanelResults::SaveDoseDiskQA(void)
{
  CDlgSaveQABeams dlgSaveQA;

  if (dlgSaveQA.DoModal() != IDOK)
  {
    return;
  }
  
  m_workflow->SaveDosesQA(dlgSaveQA.GetStatusFormats());

  return;
}

void CPanelResults::SaveDosePACS(void)
{
  SaveFilesDicom(true);

  return;
}

void CPanelResults::SaveFilesDicom(bool flagPacs)
{
  CDlgResultsSaveDicom dlgSave(m_transportParams->doseAux, m_gammaCalc);

  if (dlgSave.DoModal() != IDOK)
  {
    return;
  }

  int maskSaveFields = dlgSave.GetSaveFields();
  LPCTSTR userDescr  = dlgSave.GetDescription();

  m_workflow->SaveDosesPatient(maskSaveFields, userDescr, flagPacs);

  return;
}

void CPanelResults::DisplayDoseDiff(void)
{

  if (m_engineVis->DisplayDoseDiff())
  {
    m_engineVis->Render2();

    int pos = 0;
    CSliderCtrl* slider;
    slider = (CSliderCtrl*)GetDlgItem(IDC_SLIDER_DOSE);
    slider->SetRange(0, 100, TRUE);
    slider->SetPos(pos);
  }

  return;
}

void CPanelResults::CalcHistograms(void)
{

  m_enginePostCalc->CalcDoseHistograms();

  return;
}

void CPanelResults::CalcProfiles(void)
{

  m_enginePostCalc->CalcDoseProfiles();

  return;
}

void CPanelResults::SumBeamDoses(void)
{

  m_enginePostCalc->SumBeamDoses(m_ionPlan);

  return;
}

void CPanelResults::SumExternalDose(void)
{

  float doseMax = m_enginePostCalc->SumExternalDose(m_ionPlan);

  m_engineVis->LoadDoseSum();

  CString doseString;
  doseString.Format("Max Sec: %6.2f", doseMax);
  CWnd* wnd = GetDlgItem(IDC_STC_DOSE_MAX_SEC);
  wnd->SetWindowTextA(doseString);

  CComboBox* combo;
  combo = (CComboBox*)GetDlgItem(IDC_CMB_RESULTS_PRIM);
  combo->AddString("Dose Sum");

  combo = (CComboBox*)GetDlgItem(IDC_CMB_RESULTS_SEC);
  combo->AddString("Dose Sum");
  combo->SetCurSel(m_numResultsTypes);

  ++m_numResultsTypes;

  m_statusDoseExtrnl = true;

  return;
}

void CPanelResults::DisplayStructs(void)
{
  CFileDialog fileDlg(TRUE, "*.dcm", NULL, 0, "RTStruct (*.dcm) |*dcm|All Files (*.*)|*.*||");

  if (fileDlg.DoModal() != IDOK)
  {
    return;
  }

  float doseMax = m_enginePostCalc->LoadStructsDicom(fileDlg.GetPathName());

  return;
}
