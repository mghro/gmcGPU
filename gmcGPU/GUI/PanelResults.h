#pragma once

#include "Viewer.h"
#include "GammaIO.h"
#include "MsgBlinker.h"

class  CPanelBase;
class  CIonPlan;
struct STransportParams;

class CPanelResults : public CPanelBase
{
public:

  CPanelResults(CPanelTabCtrl* parent);
  ~CPanelResults(void);

  inline CWnd** GetUIWnds(void)
  {
    return m_uiWnds;
  }

  void OnTabEnter(int prevPanel, void* data);

private:

  BOOL OnInitDialog(void);
  void DoDataExchange(CDataExchange* pDX);

  afx_msg void OnHScroll(UINT action, UINT pos, CScrollBar* scrollBar);
  afx_msg void OnOK(void);

  afx_msg void OnTileConfigButton(UINT button);
  afx_msg void SetDisplayBoundsType(UINT position);
  afx_msg void SetViewAxis(UINT position);
  afx_msg void SetImageType(UINT position);

  afx_msg void CalcEnergy(void);
  afx_msg void CalcGamma(void);
  afx_msg void CalcGammaDone(void);
  afx_msg void SumBeamDoses(void);
  afx_msg void SumExternalDose(void);
  afx_msg void DisplayStructs(void);

  LRESULT OnGammaDone(WPARAM wParam, LPARAM lParam);

  void InitViewAxis(void);
  void SetTileConfig(UINT tileConfigIndex);
  void SetWidgetVisSecondary(int visStatus);

  void ZoomUp(void);
  void ZoomDown(void);

  void SetResultsTypePrim(void);
  void SetResultsTypeSec(void);
  void UpdateResultsView(void);

  void ToggleSynchStatus(void);

  void OpenDoseDicom(void);

  void SaveDoseDisk(void);
  void SaveDosePACS(void);
  void SaveDoseDiskQA(void);
  void SaveFilesDicom(bool flagPacs);

  void DisplayDoseDiff(void);
  void CalcHistograms(void);
  void CalcProfiles(void);

  afx_msg void SetMouseBehavior(UINT nID);

  CViewer*          m_viewer;
  CEngineVis*       m_engineVis;
  CEnginePostCalc*  m_enginePostCalc;
  CWorkflow*        m_workflow;

  CMsgBlinker m_msgBlinker;

  CIonPlan* m_ionPlan;
  STransportParams* m_transportParams;

  float m_zoom;

  int m_tileConfigIndex;

  int m_numResultsTypes;
  CString* m_resultsStrings;
  bool m_statusDoseExtrnl;

  CWnd* m_uiWnds[NUM_UI_WNDS] = {};

  CBitmap m_bitmap;

  float m_displayDoseMin;
  float m_displayDoseMax;
  int   m_boundsType;
  bool  m_gammaCalc;

  SGammaThresholds m_gammaThresholds;

  CBitmap m_bitmapTile1;
  CBitmap m_bitmapTile4;
  CBitmap m_bitmapTile2H;
  CBitmap m_bitmapTile2V;
  CBitmap m_bitmapZoomUp;
  CBitmap m_bitmapZoomDown;

  DECLARE_MESSAGE_MAP()
};