#pragma once

#ifndef __AFXWIN_H__
	#error "include 'stdafx.h' before including this file for PCH"
#endif

#include "resource.h"		// main symbols
#include "AppConfigInstance.h"

class CAppState;

class CAppGMC : public CWinApp
{
public:

	CAppGMC(void);
	~CAppGMC(void);

	BOOL InitInstance(void);
	bool RunCommandLine(CAppState* appState);

	CAppConfigInstance m_configInstance;

	DECLARE_MESSAGE_MAP()
};
