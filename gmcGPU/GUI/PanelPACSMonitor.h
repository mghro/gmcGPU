#pragma once

class CPanelBase;
class CWorkflow;

class CPanelPACSMonitor : public CPanelBase
{
public:

  CPanelPACSMonitor(CWnd* parent);
  ~CPanelPACSMonitor(void);

private:

  BOOL OnInitDialog(void);

  void OnTimer(UINT_PTR nTimerID);
  LRESULT OnComputePlanDone(WPARAM wParam, LPARAM lParam);
  LRESULT OnPACSDone(WPARAM wParam, LPARAM lParam);

  CWorkflow* m_workflow;

  DECLARE_MESSAGE_MAP()

};

