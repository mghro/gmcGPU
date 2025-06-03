#include "stdafx.h"
#include "afxdialogex.h"
#include "resource.h"
#include "Heapster.h"
#include "Workflow.h"
#include "PanelTabCtrl.h"
#include "PanelBase.h"
#include "PanelSetup.h"
#include "PanelResults.h"
#include "PanelConfig.h"
#include "AppState.h"
#include "DirectX.h"
#include "EngineTransport.h"
#include "EngineVis.h"
#include "AppDlgGMC.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#endif

class CAboutDlg : public CDialogEx
{
public:

	CAboutDlg();

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

protected:

	void DoDataExchange(CDataExchange* pDX);

	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()

BEGIN_MESSAGE_MAP(CAppDlgGCMC, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
END_MESSAGE_MAP()

CAppDlgGCMC::CAppDlgGCMC(CAppState* appState, CDirectX* directX) : CDialogEx(IDD_GMCGPU_DIALOG, NULL),
	m_appState      (appState),
	m_directX       (directX),
	m_panelTabCtrl  (NULL)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);

	return;
}

CAppDlgGCMC::~CAppDlgGCMC(void)
{
	SafeDelete(m_tabFaces[TAB_FACE_COMPUTE]);
	SafeDelete(m_tabFaces[TAB_FACE_VIEW]);
	SafeDelete(m_tabFaces[TAB_FACE_CONFIG]);

	SafeDelete(m_panelTabCtrl);
	SafeDelete(m_appState->m_workers.engineVis);
	// CView deletes itself

	return;
}

void CAppDlgGCMC::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);

	return;
}

BOOL CAppDlgGCMC::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	MoveWindow(GetSystemMetrics(SM_CXSCREEN) * 0.05, GetSystemMetrics(SM_CYSCREEN) * 0.05, GetSystemMetrics(SM_CXSCREEN) * 0.9, GetSystemMetrics(SM_CYSCREEN) * 0.9);

	CRect rect;
	CRect rectApp;

	GetClientRect(&rectApp);

	// Create tab control panel, position right side of screen
	m_panelTabCtrl = new CPanelTabCtrl(this);
	m_panelTabCtrl->GetWindowRect(&rect);
	m_panelTabCtrl->MoveWindow(rectApp.Width() - rect.Width(), 0, rect.Width(), rectApp.Height());
	m_panelTabCtrl->ShowWindow(SW_SHOW);

	// Create viewer, position left side of screen
	rectApp.DeflateRect(0, 0, rect.Width(), 0);
	m_appState->m_workers.viewer = new CViewer(this, &rectApp, WS_VSCROLL | WS_HSCROLL);
	m_appState->m_workers.viewer->MoveWindow(0, 0, rectApp.Width(), rectApp.Height());

	// Initialize DirectX, viewer needed for swapchain, dimensions
	if (m_directX->InitializeGraphics(GPU_VIS_3D2D, m_appState->m_workers.viewer) == false)
	{
		AfxMessageBox("Could not Initialize GPU card.\r\n              Exiting.");
		exit(0);
	}

	// Create GPU engines
	m_appState->m_workers.engineVis = new CEngineVis(); // MLW put in destructor of this not panels
	m_appState->m_workers.engineVis->Initialize();

	
	// Create tab panels, add to tab control
	m_tabFaces = new CPanelBase* [NUM_TAB_FACES];

	m_tabFaces[TAB_FACE_COMPUTE] = new CPanelSetup(m_panelTabCtrl);
	m_tabFaces[TAB_FACE_VIEW]    = new CPanelResults(m_panelTabCtrl);
	m_tabFaces[TAB_FACE_CONFIG]  = new CPanelConfig(m_panelTabCtrl);
		

	m_panelTabCtrl->AddPanels(m_tabFaces, NUM_TAB_FACES);

	// Initialize viewer
	CWnd** wnds = ((CPanelResults*)m_tabFaces[TAB_FACE_VIEW])->GetUIWnds();
	m_appState->m_workers.viewer->Initialize(m_appState->m_workers.engineVis, wnds);
	m_appState->m_workers.viewer->ShowWindow(SW_SHOW);
	
	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CAppDlgGCMC::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CAppDlgGCMC::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}

	return;
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CAppDlgGCMC::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

