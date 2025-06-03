#pragma once

#define MSG_TAB_CHANGE_CMD    (WM_USER + 1)

enum
{
  TAB_FACE_COMPUTE,
  TAB_FACE_VIEW,
  TAB_FACE_CONFIG,

  NUM_TAB_FACES
};

class CPanelBase;

class CPanelTabCtrl : public CDialog
{
public:

  CPanelTabCtrl(CWnd* parent);
  ~CPanelTabCtrl(void);

  void AddPanels(CPanelBase** panels, int numPanels);

protected:

  BOOL OnInitDialog(void);
  void DoDataExchange(CDataExchange* pDX);
  void OnSelChange(NMHDR* pNMHDR, LRESULT* pResult);

  afx_msg LRESULT ComputeDone(WPARAM wParam, LPARAM lParam);

  CTabCtrl m_tabCtrl;

  CPanelBase** m_tabPanels;

  int m_currentTabIndex;

  DECLARE_MESSAGE_MAP()
};