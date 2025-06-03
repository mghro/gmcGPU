#pragma once



class CPanelBase;
class CPanelTabCtrl;
class CPanelResults;
class CPanelSetup;
class CViewer;
class CEngineVis;
class CEngineTransport;
class CDirectX;
class CAppState;

class CAppDlgGCMC : public CDialogEx
{
public:

	CAppDlgGCMC(CAppState* appState, CDirectX* directX);
	~CAppDlgGCMC(void);

#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_GMCGPU_DIALOG };
#endif

protected:

	virtual void DoDataExchange(CDataExchange* pDX);

	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();

	CAppState* m_appState;
	CDirectX*  m_directX;

	CPanelTabCtrl*  m_panelTabCtrl;
	CPanelBase** m_tabFaces;


	DECLARE_MESSAGE_MAP()
};
