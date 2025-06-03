#include "stdafx.h"
#include "Heapster.h"
#include "UniArray.h"
#include "PACSServer.h"
#include "AppState.h"
#include "AppConfigInstance.h"
#include "Transport.h"
#include "Logger.h"
#include "PacsMonitor.h"
#include "PhantomQA.h"
#include "IonPlan.h"
#include "VersionGMC.h"
#include "Workflow.h"

// Global
CString g_doseTypes[NUM_DOSE_TYPES] =
{
  "Dose",
  "LET",
  "BioDose",
  "DoseDivLET",
  "RBE_02",
  "RBE_03",
  "RBE_10",
};

static UINT ExecuteThreadProc(LPVOID pParam);

static CString s_seriesPrefix[NUM_WF_MODES] =
{
  "GMC_",
  "GMCpacs_"
};

enum
{
  WF_UPDATE_PACS,
  WF_UPDATE_JSON_BUILD,
  WF_UPDATE_LOAD_PLAN,
  WF_UPDATE_COMPUTE,
  WF_UPDATE_AUX,
  WF_UPDATE_QA,
  WF_UPDATE_SAVE_DISK,
  WF_UPDATE_SAVE_PACS

};
static CString s_messagesUpdates[] =
{
  "Retrieving DICOM PACS",
  "Building JSON",
  "Loading Plan",
  "Computing Transport",
  "Computing Aux Doses",
  "Computing QA Beams",
  "Saving to Disc",
  "Saving to PACS"
};

typedef enum
{
  WF_ERROR_DICOM_PLAN_PARSE = 0,
  WF_ERROR_JSON_BUILD,
  WF_ERROR_JSON_OPEN,
  WF_ERROR_JSON_PARSE,
  WF_ERROR_NPY,
  WF_ERROR_PLAN_LOAD,
  WF_ERROR_PROTONS_LOAD,
  WF_ERROR_QA_LOAD,

  WF_ERROR_NONE

} EErrorMsg;

static CString s_messagesErrors[] =
{
  "Error Parsing Dicom Plan. Check Modality.",
  "Error Converting Dicom to JSON.",
  "Error Opening Plan JSON file.",
  "Error Parsing Plan JSON file.",
  "Error Reading Image npy files.",
  "Error Loading Plan to GPU.",
  "Error Loading Protons to GPU.",
  "Error Loading QA Phantom to GPU."

};

CWorkflow::CWorkflow(void) :
  m_ionPlan         (NULL),
  m_phantom         (NULL),
  m_transportParams (NULL),
  m_gammaThresholds (NULL),
  m_wndNotify       (NULL),
  m_modeWF          (WF_MODE_USER)
{

  return;
}

CWorkflow::~CWorkflow(void)
{
  SafeDelete(m_ionPlan);
  SafeDelete(m_phantom);

  return;
}

static UINT ExecuteThreadProc(LPVOID pParam)
{
  CWorkflow* workflow = (CWorkflow*)pParam;

  workflow->RunThreadProcCurrent();

  return 0;
}

void CWorkflow::RunThreadProcCurrent(void)
{
  (this->*ThreadProcCurrent)();

  return;
}


/*_____________________________________________________________________________________________

   Routines called by GUI to start transport
_______________________________________________________________________________________________*/

void CWorkflow::ComputePlan(STransportParams* transportParams, CWnd* wndNotify, bool playerStatus)
{
  m_transportParams = transportParams;
  m_wndNotify       = wndNotify;
  m_playerStatus    = playerStatus;
  m_errorMsg        = WF_ERROR_NONE;

  m_modeWF = WF_MODE_USER;

  ThreadProcCurrent = &CWorkflow::ThreadProcEndtoEnd;

  AfxBeginThread(ExecuteThreadProc, this);

  return;
}

void CWorkflow::RunCommandLine(STransportParams* transportParams, CWnd* wndNotify, bool playerStatus)
{
  m_transportParams = transportParams;
  m_wndNotify       = wndNotify;
  m_playerStatus    = playerStatus;
  m_errorMsg        = WF_ERROR_NONE;

  m_modeWF = WF_MODE_USER;

  ThreadProcEndtoEnd();

  return;
}

void CWorkflow::ComputeRerun(STransportParams* transportParams)
{
  m_transportParams = transportParams;

  m_modeWF = WF_MODE_USER;

  ThreadProcCurrent = &CWorkflow::ThreadProcRerun;

  AfxBeginThread(ExecuteThreadProc, this);

  return;
}

void CWorkflow::ProcessNewPACSPlans(CWnd* wndNotify)
{
  m_wndNotify = wndNotify;

  if (m_modeWF != WF_MODE_PACS_MON)
  {
    m_modeWF = WF_MODE_PACS_MON;
    // C:\ProgramData\MIM\logs\services\dcmstore
    m_prevPACSQueryTime.SetDateTime(2023, 07, 07, 0, 0, 0);
  }

  ThreadProcCurrent = &CWorkflow::ThreadProcPACSPlanSearch;

  AfxBeginThread(ExecuteThreadProc, this);

  return;
}

void CWorkflow::ComputeGamma(SGammaThresholds* gammThresholds, CWnd* wndNotify)
{

  m_wndNotify       = wndNotify;
  m_gammaThresholds = gammThresholds;

  ThreadProcCurrent = &CWorkflow::ThreadProcComputeGamma;

  AfxBeginThread(ExecuteThreadProc, this);

  return;
}

/*_____________________________________________________________________________________________

   Processes for tranport to be run in threads separate from main app thread.
_______________________________________________________________________________________________*/

void CWorkflow::ThreadProcEndtoEnd(void)
{

  m_enginePostCalc->ClearResources();

  if (m_playerStatus)
  {

    if (BuildJSON() == false)
    {
      SendErrorNotice();
      return;
    }
  }

  if (LoadPlan() == false)
  {
    SendErrorNotice();
    return;
  }

  if (PrepCompute() == false)
  {
    SendErrorNotice();
    return;
  }

  m_wndNotify->PostMessage(MSG_WF_LOAD_PLAN_DONE, (WPARAM)m_ionPlan, NULL);

  if (ComputeTransport() == false)
  {
    SendErrorNotice();
    return;
  }

  if (m_transportParams->autoMode)
  {
    if (m_transportParams->doseQA)
    {
      SaveDosesQAGamma();
    }

    int pacsSave = (m_modeWF == WF_MODE_PACS_MON || m_transportParams->pacsSave) ? true : false;

    if (pacsSave)
    {
      SendUpdateNotice(WF_UPDATE_SAVE_PACS);
    }
    else
    {
      SendUpdateNotice(WF_UPDATE_SAVE_DISK);
    }

    unsigned int bitmaskFields;
    if (m_transportParams->doseAux)
    {
      bitmaskFields = (1 << NUM_DOSE_TYPES) - 1;
    }
    else
    {
      bitmaskFields = 1;
    }

    SaveDosesPatient(bitmaskFields, "Auto", pacsSave);
  }

  m_wndNotify->PostMessage(MSG_WF_COMPUTE_DONE, WF_TASK_PASS, NULL);

  return;
}

void CWorkflow::ThreadProcRerun(void)
{
  APPLOGT("Rerun")

  m_enginePostCalc->ClearResources();

  if (PrepCompute() == false)
  {
    SendErrorNotice();
    return;
  }

  if (ComputeTransport() == false)
  {
    SendErrorNotice();
  }
  else
  {
    m_wndNotify->PostMessage(MSG_WF_COMPUTE_DONE, WF_TASK_PASS, NULL);
  }

  return;
}

void CWorkflow::ThreadProcPACSPlanSearch(void)
{
  bool  status;
  char* uidIonPlan;

  CPacsMonitor pacsMonitor;
  STransportParams transportParams = {};

  COleDateTime planSearchTime = m_prevPACSQueryTime;
  m_prevPACSQueryTime.GetCurrentTime();

  if (pacsMonitor.OpenLogFile(g_configInstance->m_mimMonitorLogFile) == false)
  {
    APPLOG("Could not open PACS log file %s", g_configInstance->m_mimMonitorLogFile);

    return;
  }

  transportParams.numCycles         = 10;
  transportParams.protonScaleFactor = 10000;
  transportParams.dosePrimary       = true;
  transportParams.pacsSave          = true;
  transportParams.autoMode          = true;

  m_playerStatus = true;
  m_transportParams = &transportParams;

  while (uidIonPlan = pacsMonitor.ParseLogFile(&planSearchTime))
  {
    status = m_pacsPipe->ProcessPlanFileSet(uidIonPlan);

    SPlanInfo* plan = m_pacsPipe->GetPlan(0);

    transportParams.pathNamePlan = m_pacsPipe->GetPlanDirectory();
    transportParams.fileNamePlan = plan->fileNameBase;

    ThreadProcEndtoEnd();
  }

  m_wndNotify->PostMessage(MSG_WF_PACS_PROCESS_DONE, NULL, NULL);

  return;
}

void CWorkflow::ThreadProcComputeGamma(void)
{
  SGammaResults* gammaResults = m_enginePostCalc->CalcGamma(m_gammaThresholds);

  m_wndNotify->PostMessage(MSG_WF_CALC_GAMMA_DONE, (WPARAM)gammaResults, NULL);

  return;
}

/*_____________________________________________________________________________________________

   Routines to send status updates to GUI
_______________________________________________________________________________________________*/

void CWorkflow::SendUpdateNotice(int updateMsg)
{

  m_wndNotify->PostMessage(MSG_WF_COMPUTE_UPDATE, (WPARAM)(LPCTSTR)s_messagesUpdates[updateMsg], NULL);

  return;
}

void CWorkflow::SendErrorNotice(void)
{

  m_wndNotify->PostMessage(MSG_WF_COMPUTE_DONE, WF_TASK_FAIL, (LPARAM)(LPCTSTR)s_messagesErrors[m_errorMsg]);

  return;
}

/*_____________________________________________________________________________________________

   Routines to complete stages of the transport process
_______________________________________________________________________________________________*/

bool CWorkflow::BuildJSON(void)
{
  SPlanInfo planInfo;
  
  SendUpdateNotice(WF_UPDATE_JSON_BUILD);

  planInfo.fileNameFull = m_transportParams->pathNamePlan + "\\" + m_transportParams->fileNamePlan;
  
  if (m_pacsPipe->GetInfoPlan(&planInfo) == false)
  {
    m_errorMsg = WF_ERROR_DICOM_PLAN_PARSE;
    return false;
  }

  if (m_pacsPipe->ConvertStudyDicom2Json(m_transportParams->pathNamePlan, m_transportParams->fileNamePlan) == false)
  {
    m_errorMsg = WF_ERROR_JSON_BUILD;
    return false;
  }

  m_transportParams->pathNamePlan += "\\OUT";
  m_transportParams->fileNamePlan = planInfo.planName + ".json";

  return true;
}

bool CWorkflow::LoadPlan(void)
{

  SendUpdateNotice(WF_UPDATE_LOAD_PLAN);

  if (ReadPlanFiles() == false)
  {
    return false;
  }

  APPLOGT("Plan Read Complete");

  if (m_engineTransport->LoadRTIonPlan(m_ionPlan) == false)
  {
    m_errorMsg = WF_ERROR_PLAN_LOAD;
    return false;
  }

  return true;
}

bool CWorkflow::ReadPlanFiles(void)
{

  CString fileNameFull = m_transportParams->pathNamePlan + "\\" + m_transportParams->fileNamePlan;

  APPLOGT("Json Plan File: %s", fileNameFull);

  FILE* fd = fopen(fileNameFull, "rb");

  if (!fd)
  {
    m_errorMsg = WF_ERROR_JSON_OPEN;
    return false;
  }

  SafeDelete(m_ionPlan);
  m_ionPlan = new CIonPlan;
  m_appState->m_compObjects.ionPlan = m_ionPlan;

  bool status = m_ionPlan->ParseJSON(fd);

  fclose(fd);

  if (status == false)
  {
    m_errorMsg = WF_ERROR_JSON_PARSE;

    APPLOGT("Json Parse Error");
    return false;
  }

  status = m_ionPlan->ParseBlobs(m_transportParams->pathNamePlan); // MLW parse blobs inside parsejson
  if (status == false)
  {
    m_errorMsg = WF_ERROR_NPY;
    return false;
  }

  return true;
}

bool CWorkflow::PrepCompute(void)
{
  if (m_engineTransport->PrepCompute(m_transportParams) == false)
  {
    m_errorMsg = WF_ERROR_PROTONS_LOAD;
    return false;
  }

  return true;
}

bool CWorkflow::ComputeTransport(void)
{

  SendUpdateNotice(WF_UPDATE_COMPUTE);

  APPLOGT("GPU Load Complete");

  if (m_ionPlan->m_flagSOBP)
  {
    m_engineTransport->ComputePlanSOBP(m_transportParams);
  }
  else
  {
    m_engineTransport->ComputePlanDose(m_transportParams);
  }

  APPLOGT("\nTransport Complete")

  if (m_transportParams->doseQA)
  {
    SendUpdateNotice(WF_UPDATE_QA);

    RunQABeamsManual();
  }

  m_enginePostCalc->LoadTransportInfo();

  if (m_transportParams->doseAux)
  {
    SendUpdateNotice(WF_UPDATE_AUX);

    m_enginePostCalc->CalcAuxFields(m_ionPlan->m_numFractions);
  }

  return true;
}

bool CWorkflow::RunQABeamsManual(void)
{

  SafeDelete(m_phantom);
  m_phantom = new CPhantomQA();
  m_appState->m_compObjects.phantomQA = m_phantom;

  m_phantom->ParseJSON(NULL);

  if (m_engineTransport->LoadQAPhantom(m_phantom) == false)
  {
    m_errorMsg = WF_ERROR_QA_LOAD;
    return false;
  }

  m_engineTransport->ComputeQABeamDoses(m_transportParams);

  return true;
}

/*_____________________________________________________________________________________________

   Routines to save results to disk and/or PACS
_______________________________________________________________________________________________*/

void CWorkflow::SaveDosesPatient(unsigned int bitmaskFields, LPCTSTR userDescr, bool pacsSend)
{
  CString descr;
  CString gmcInfo = s_seriesPrefix[0] + VERSION_SYSTEM;

  for (int indexDose = 0; indexDose < NUM_DOSE_TYPES; ++indexDose)
  {
    if ((1 << indexDose) & bitmaskFields)
    {
      if (userDescr == NULL || userDescr[0] == 0)
      {
        descr.Format("%s_%s_%s", gmcInfo, g_doseTypes[indexDose], m_ionPlan->m_seriesDescription);
      }
      else
      {
        descr.Format("%s_%s_%s_%s", gmcInfo, g_doseTypes[indexDose], userDescr, m_ionPlan->m_seriesDescription);
      }
      
      SaveDoseFieldDicom(SAVE_TYPE_PATIENT, indexDose, (LPCTSTR)descr, pacsSend);
    }
  }

  if ((1 << DOSE_GAMMA) & bitmaskFields)
  {
    if (userDescr == NULL || userDescr[0] == 0)
    {
      descr.Format("%s_Gamma_%s", gmcInfo, m_ionPlan->m_seriesDescription);
    }
    else
    {
      descr.Format("%s_Gamma_%s_%s", gmcInfo, userDescr, m_ionPlan->m_seriesDescription);
    }

    SaveDoseFieldDicom(SAVE_TYPE_GAMMA, 0, (LPCTSTR)descr, pacsSend);
  }

  return;
}

void CWorkflow::SaveDosesQA(unsigned int statusFormats, bool pacsSend)
{
  if (statusFormats & QA_FORMAT_GAMMA_GMC)
  {
    SaveDosesQAGamma();
  }

  if (statusFormats & QA_FORMAT_DICOM)
  {
    SaveDosesQADicom(pacsSend);
  }

  return;
}

void CWorkflow::SaveDosesQADicom(int pacsSend)
{
  CString descr;
  CString gmcInfo = s_seriesPrefix[0] + VERSION_SYSTEM;

  // Save primary patient dose
  descr.Format("%s_%s_%s", gmcInfo, g_doseTypes[0], m_ionPlan->m_seriesDescription);
  SaveDoseFieldDicom(SAVE_TYPE_PATIENT, 0, (LPCTSTR)descr, pacsSend);

  for (unsigned int indexBeam = 0; indexBeam < m_ionPlan->m_arrayBeams.m_numItems; ++indexBeam)
  {
    descr.Format("%s_Beam_%02d_%s", gmcInfo, indexBeam, m_ionPlan->m_seriesDescription);
    SaveDoseFieldDicom(SAVE_TYPE_QA, indexBeam, (LPCTSTR)descr, pacsSend);
  }

  return;;
}

bool CWorkflow::SaveDoseFieldDicom(int saveType, int indexDose, LPCTSTR descr, int pacsSend)
{
  CString fileNameFull;
  fileNameFull.Format("%s\\%s_%s.dcm", (LPCTSTR)m_transportParams->pathNamePlan, descr, m_ionPlan->m_planName);

  m_enginePostCalc->SaveDicom(fileNameFull, (LPCTSTR)descr, m_ionPlan, saveType, indexDose);

  if (pacsSend)
  {
    m_pacsPipe->StoreFile(fileNameFull);
  }

  return true;
}

void CWorkflow::SaveDosesQAGamma(void)
{
  CString prelimFileName;
  CString fileName;

  int charIndex = m_ionPlan->m_studyDate.Find(' ');
  CString studyDateMod = m_ionPlan->m_studyDate.Left(charIndex);

  CString patientNameMod = m_ionPlan->m_patientName;
  patientNameMod.Remove('^');

  charIndex = patientNameMod.Find('^');
  if (patientNameMod.GetAllocLength() < 6) // entire patient name < 5 (6 for space)
  {
    patientNameMod.Remove('^');
  }
  else if (charIndex < 4) // Last name < 4 chars
  {
    if (charIndex > 0)
    {
      patientNameMod = patientNameMod.Left(6);
      patientNameMod.Remove('^');
    }
    else
    {
      patientNameMod = patientNameMod.Left(5);
    }
  }
  else // Get first 4 of last name + first initial of first name
  {
    patientNameMod = patientNameMod.Left(4);
    patientNameMod += m_ionPlan->m_patientName[charIndex + 1];
  }

  patientNameMod.MakeLower();

  CString patientIDMod = m_ionPlan->m_patientID;
  patientIDMod.Remove('-');

  CString planNameMod = m_ionPlan->m_planName;
  planNameMod.Replace(' ', '_');
  planNameMod.Replace('+', '-');

  prelimFileName.Format("%s%s__course_%s__%s__%s", patientNameMod, patientIDMod, studyDateMod, m_ionPlan->m_astroidID,
    planNameMod);

  LPCTSTR pathName;
  if (g_configInstance->m_pathNameQAsave.IsEmpty())
  {
    pathName = m_transportParams->pathNamePlan;
  }
  else
  {
    pathName = g_configInstance->m_pathNameQAsave;
  }

  for (unsigned int beamIndex = 0; beamIndex < m_ionPlan->m_arrayBeams.m_numItems; ++beamIndex)
  {
    FILE* fd;

    CString fractionNameMod = m_ionPlan->m_arrayFractionGroups(m_ionPlan->m_arrayMetaBeams(beamIndex)->fractionIndex)->description;
    fractionNameMod.Replace(' ', '_');
    fractionNameMod.Replace('+', '-');

    fileName.Format("%s\\%s__%s__%s.GMC.QA.dose", pathName, prelimFileName, fractionNameMod, m_ionPlan->m_arrayMetaBeams(beamIndex)->beamName);
    fd = fopen(fileName, "wb");

    m_enginePostCalc->SaveQABeamDose(fd, &m_phantom->m_voxGridCalc, beamIndex);

    fclose(fd);

    fileName.Format("%s\\%s__%s__%s.GMC.QA.set", pathName, prelimFileName, fractionNameMod, m_ionPlan->m_arrayMetaBeams(beamIndex)->beamName);
    fd = fopen(fileName, "wb");

    int controlPtIndex = m_ionPlan->m_arrayBeams(beamIndex)->indexControlPts;

    m_enginePostCalc->SaveQABeamGeometry(fd, m_phantom->m_voxGridRSP.dims, m_ionPlan->m_arrayControlPts(controlPtIndex)->isocenterQA, m_phantom->m_voxGridCalc.dims);

    fclose(fd);
  }

  return;
}


