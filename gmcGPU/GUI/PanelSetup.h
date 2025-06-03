#pragma once

#include <DirectXMath.h>
#include "DirectX.h"
#include "MsgBlinker.h"

class CIonPlan;
class CPanelBase;
class CPhantomQA;
class CViewer;

struct STransportParams;

class CPanelSetup : public CPanelBase
{
public:

  CPanelSetup(CPanelTabCtrl* parent);
  ~CPanelSetup(void);

  BOOL OnInitDialog(void);
  void DoDataExchange(CDataExchange* pDX);

private:

  afx_msg void OnResetRerun(void);

  afx_msg void ToggleDoseAux(void);
  afx_msg void ToggleAutoMode(void);
  afx_msg void ToggleQADoses(void);
  afx_msg void ToggleDij(void);
  afx_msg void TogglePACSSave(void);

  void DisableConfigWnds(void);
  void ResetConfigWnds(void);
  void ClearGUIPlanInfo(void);

  LRESULT OnLoadPlanDone(WPARAM wParam, LPARAM lParam);
  LRESULT OnComputeUpdate(WPARAM wParam, LPARAM lParam);
  LRESULT OnComputeDone(WPARAM wParam, LPARAM lParam);

  void ExecutePostComputeManual(void);
  void ToggleStatistics(void);
  void NumCyclesChange(void);
  void GetProtonScaleFactor(void);
  void SaveApertures(void);
  void ResampleDose(void);

  void SelectPlanJSON(void);
  void SelectPlanPlayer(void);
  void SelectPlanPACS(void);
  void OnPlanSelected(bool runPlayer);


  afx_msg void SetProtonScalingType(UINT nID);

  CViewer*          m_viewer;
  CEngineTransport* m_engineTransport;
  CEngineVis*       m_engineVis;
  CWorkflow*        m_workflow;
  STransportParams* m_transportParams;

  int  m_numProtonsSpot;

  bool m_resetRun;

  CEdit editRBE;
  CEdit edit;
  float m_rbeAB;

  CMsgBlinker m_msgBlinker;

  DECLARE_MESSAGE_MAP()

};