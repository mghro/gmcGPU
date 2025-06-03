#include "stdafx.h"
#include "MsgBlinker.h"


BEGIN_MESSAGE_MAP(CMsgBlinker, CStatic)

	ON_WM_TIMER()

END_MESSAGE_MAP()

CMsgBlinker::CMsgBlinker(void)
{
	return;
}

CMsgBlinker::~CMsgBlinker(void)
{
	return;
}

void CMsgBlinker::StartTimer(void)
{
	ShowWindow(SW_SHOW);

	m_toggle = 0;

	m_gifTimer = SetTimer(0, 500, NULL);

	return;
}

afx_msg void CMsgBlinker::OnTimer(UINT_PTR nIDEvent)
{

	if (m_toggle)
	{
		ShowWindow(SW_SHOW);
		m_toggle = 0;
	}
	else
	{
		ShowWindow(SW_HIDE);
		m_toggle = 1;
	}

	m_gifTimer = SetTimer(0, 500, 0);

	return;
}

void CMsgBlinker::StopTimer(void)
{

	ShowWindow(SW_HIDE);

	KillTimer(0);

	return;
}