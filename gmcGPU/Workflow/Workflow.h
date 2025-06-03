#pragma once

#include "UniArray.h"
#include "EngineTransport.h"
#include "EnginePostCalc.h"
#include "PACSPipe.h"

struct STransportParams;
struct SGammaThresholds;
class CIonPlan;
class CPhantomQA;
class CAppState;



enum
{
  WF_MODE_USER = 0,
  WF_MODE_PACS_MON,

  NUM_WF_MODES
};

enum
{
  MSG_WF_LOAD_PLAN_DONE = WM_USER + 1,
  MSG_WF_COMPUTE_UPDATE,
  MSG_WF_COMPUTE_DONE,
  MSG_WF_CALC_GAMMA_DONE,
  MSG_WF_PACS_PROCESS_DONE
};

enum
{
  WF_TASK_FAIL = 0,
  WF_TASK_PASS,
};

enum
{
  QA_FORMAT_GAMMA_GMC       = 1,
  QA_FORMTAT_GAMMA_EXTERNAL = 2,
  QA_FORMAT_DICOM           = 4,
};

class CWorkflow
{
public:

  CWorkflow(void);
  ~CWorkflow(void);

  void ComputePlan(STransportParams* transportParams, CWnd* wndNotify, bool playerStatus = false);
  void ComputeRerun(STransportParams* transportParams);
  void ComputeGamma(SGammaThresholds* gammaThresholds, CWnd* wndNotify);

  void SaveDosesPatient(unsigned int bitmaskFields, LPCTSTR userDescr, bool pacsSend = false);
  void SaveDosesQA(unsigned int statusFormats, bool pacsSend = false);


  void ProcessNewPACSPlans(CWnd* wndNotify);

  inline void SetAppState(CAppState* appState)
  {
    m_appState        = appState;
    m_engineTransport = appState->m_workers.engineTransport;
    m_enginePostCalc  = appState->m_workers.enginePostCalc;
    m_pacsPipe        = appState->m_workers.pacsPipe;
  }

  void RunThreadProcCurrent(void);
  void RunCommandLine(STransportParams* transportParams, CWnd* wndNotify, bool playerStatus = false);

private:

  void ThreadProcEndtoEnd(void);
  void ThreadProcRerun(void);
  void ThreadProcPACSPlanSearch(void);
  void ThreadProcComputeGamma(void);

  void SendUpdateNotice(int msgUpdate);
  void SendErrorNotice();

  bool BuildJSON(void);
  bool LoadPlan(void);
  bool ReadPlanFiles(void);
  bool PrepCompute(void);
  bool ComputeTransport(void);
  bool RunQABeamsManual(void);

  bool SaveDoseFieldDicom(int saveType, int indexDose, LPCTSTR descr, int pacsSend);
  void SaveDosesQAGamma(void);
  void SaveDosesQADicom(int pacsSend);

  void (CWorkflow::*ThreadProcCurrent)(void);

  CEngineTransport* m_engineTransport;
  CEnginePostCalc*  m_enginePostCalc;

  CPACSPipe*        m_pacsPipe;

  STransportParams* m_transportParams;

  CIonPlan*   m_ionPlan;
  CPhantomQA* m_phantom;
  CWnd*       m_wndNotify;
  CAppState*  m_appState;

  bool        m_playerStatus;
  int         m_errorMsg;

  LPCTSTR m_descrPrefixCurrent;

  int m_modeWF;

  SGammaThresholds* m_gammaThresholds;

  COleDateTime m_prevPACSQueryTime;

};


