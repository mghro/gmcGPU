#include "stdafx.h"
#include "Logger.h"
#include "FileIO.h"
#include "VersionGMC.h"
#include "EngineTransport.h"
#include "EnginePostCalc.h"
#include "PanelBase.h"
#include "Workflow.h"
#include "AppState.h"
#include "AppDlgGMC.h"
#include "AppGMC.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

APPLOGFILE

CAppGMC theApp;
CAppConfigInstance* g_configInstance;


BEGIN_MESSAGE_MAP(CAppGMC, CWinApp)
	ON_COMMAND(ID_HELP, &CWinApp::OnHelp)
END_MESSAGE_MAP()

CAppGMC::CAppGMC(void)
{

	return;
}

CAppGMC::~CAppGMC(void)
{

}

BOOL CAppGMC::InitInstance(void)
{

	CWinApp::InitInstance();

	AfxEnableControlContainer();

	// Activate "Windows Native" visual manager for enabling themes in MFC controls
	CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerWindows));

	// Read in the config file
	if (m_configInstance.ParseJson() == false)
	{
		AfxMessageBox("Missing or Invalid GMCConfigInstance.json");
		exit(0);
	}

	g_configInstance = &m_configInstance;

	// Start Logging
	APPLOGINIT();
	APPLOG("GMC Version %s\n\n", VERSION_GMC);


	CDirectX directX;
	if (directX.Initialize(&g_configInstance->m_gpuName) == false)
	{
		CString msg = "Could not find GPU card " + g_configInstance->m_gpuName;
		AfxMessageBox(msg);
		return FALSE;
	}

	CAppState appState;
	CWorkflow workflow;
	CFileIO   fileIO;
	CPACSPipe pacsPipe;

	appState.m_workers.fileIO = &fileIO;

	CEngineBase::SetAppState(&appState);

	CEngineTransport engineTransport;
	CEnginePostCalc  enginePostCalc;

	engineTransport.Initialize();
	enginePostCalc.Initialize();

	appState.m_workers.engineTransport = &engineTransport;
	appState.m_workers.enginePostCalc  = &enginePostCalc;
	appState.m_workers.workflow        = &workflow;
	appState.m_workers.pacsPipe        = &pacsPipe;

	workflow.SetAppState(&appState);

	if (RunCommandLine(&appState))
	{
		return FALSE; // Ran from command line so exit now
	}

	// Create and initialize GUI
	CPanelBase::SetAppState(&appState);

	CAppDlgGCMC appDlg(&appState, &directX);
	m_pMainWnd = &appDlg;

	appDlg.DoModal();

	return FALSE;
}

bool CAppGMC::RunCommandLine(CAppState* appState)
{
	USES_CONVERSION;

	CString commandLine = ::GetCommandLine();

	int numArgs = 0;
	LPWSTR* pArgv = CommandLineToArgvW(CT2W(commandLine), &numArgs);

	if (numArgs <= 1)
	{
		return false;
	}

	CString fileName = W2CA(pArgv[1]);

	CStdioFile file;

	if (file.Open(fileName, CFile::modeRead, NULL) == FALSE)
	{
		CString msg = "Couldn not open exec file: " + fileName;
		AfxMessageBox(msg);
		return false;
	}

	CWorkflow* workflow      = appState->m_workers.workflow;
	STransportParams* params = &appState->m_transportParams;
	params->autoMode = true;

	CString stringProtons;
	bool playerStatus;

	while (file.ReadString(params->pathNamePlan))
	{
		file.ReadString(params->fileNamePlan);
		file.ReadString(stringProtons);
		sscanf(stringProtons, "%d %d", &params->numCycles, &params->protonScaleFactor);

		playerStatus = params->fileNamePlan.Find("json") ? false : true;

		workflow->RunCommandLine(params, CWnd::GetDesktopWindow(), playerStatus);
	}

	return true;
}


