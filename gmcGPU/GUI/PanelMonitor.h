#pragma once

class CPanelBase;
struct STransportParams;

class CPanelPACSMonitor : public CPanelBase
{
public:

  CPanelPACSMonitor(CWnd* parent, CRect* rect);
  ~CPanelPACSMonitor(void);

private:

  BOOL OnInitDialog(void);

  afx_msg void OnTimer(UINT_PTR nTimerID);
  void ToggleStartStop(void);
  void StartPACSQuery(void);

  LRESULT OnPlanComputeDone(WPARAM wParam, LPARAM lParam);

  CRect* m_rect;
  CFont m_titleFont;

  bool m_monitoringEnabled;

  STransportParams* m_transportParams;

  DECLARE_MESSAGE_MAP()
};
