#include "stdafx.h"
#include "resource.h"
#include "Heapster.h"
#include "AppConfigInstance.h"
#include "DlgPlanGetPacs.h"


#define MSG_THREAD_DONE (WM_APP + 1)

enum
{
	THREAD_TYPE_NONE,
	THREAD_TYPE_PLANS,
	THREAD_TYPE_STUDY
};

BEGIN_MESSAGE_MAP(CDlgPlanGetPacs, CDialogEx)

	ON_BN_CLICKED(IDC_BTN_QUERY,     RetrievePlans)
	ON_BN_CLICKED(IDC_BTN_RETRIEVE,  RetrieveStudy)

	ON_WM_TIMER()

	ON_MESSAGE(MSG_THREAD_DONE, OnThreadComplete)

	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST1, &CDlgPlanGetPacs::OnLvnItemchangedList1)

END_MESSAGE_MAP()

CDlgPlanGetPacs::CDlgPlanGetPacs(CPACSPipe* pacsPipe) : CDialogEx(IDD_DLG_PLAN_GET_PACS, NULL),
	m_pacsPipe   (pacsPipe),
	m_threadType (THREAD_TYPE_NONE),
	m_gif        (NULL)
{

	return;
}

CDlgPlanGetPacs::~CDlgPlanGetPacs()
{

	//SafeDelete(m_gif);

	return;
}

void CDlgPlanGetPacs::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_LIST1, m_listCntrl);

	if (pDX->m_bSaveAndValidate)
	{
		DDX_Text(pDX, IDC_EDIT1, m_patientID);
	}

	return;
}

BOOL CDlgPlanGetPacs::OnInitDialog(void)
{
	CDialog::OnInitDialog();

	m_listCntrl.InsertColumn(0, _T("Date"),               LVCFMT_LEFT, -1, 0);
	m_listCntrl.InsertColumn(1, _T("Series Description"), LVCFMT_LEFT, -1, 1);
	m_listCntrl.InsertColumn(2, _T("Plan Name"),          LVCFMT_LEFT, -1, 1);
	m_listCntrl.InsertColumn(3, _T("Plan Description"),   LVCFMT_LEFT, -1, 1);

	m_listCntrl.SetColumnWidth(0, 72);
	m_listCntrl.SetColumnWidth(1, 256);
	m_listCntrl.SetColumnWidth(2, 256);
	m_listCntrl.SetColumnWidth(3, 304);

	m_listCntrl.SetExtendedStyle(m_listCntrl.GetExtendedStyle() |
		LVS_EX_DOUBLEBUFFER | LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

	InitGif();

	return FALSE;
}

void CDlgPlanGetPacs::InitGif(void)
{

	Gdiplus::GdiplusStartupInput startup_input;
	Gdiplus::GdiplusStartup(&m_gdiToken, &startup_input, 0);

	CStringW fileName = g_configInstance->m_directoryBase + L"\\Resources\\Wait.gif";

	m_gif = new Gdiplus::Image(fileName);

	m_gifWidth  = m_gif->GetWidth();
	m_gifHeight = m_gif->GetHeight();
	// MLW if no or bad gif.

	CRect wndRect;
	CRect clientRect;
	CWnd* wnd = GetDlgItem(IDC_STC_GIF);
	wnd->GetWindowRect(&wndRect);
	wnd->GetClientRect(&clientRect);
	int diffX = m_gifWidth - clientRect.Width();
	int diffY = m_gifHeight - clientRect.Height();

	ScreenToClient(&wndRect);
	int newX = wndRect.left - diffX / 2;
	int newY = wndRect.top - diffY / 2;

	wndRect.SetRect(newX, newY, newX + wndRect.Width() + diffX, newY + wndRect.Height() + diffY);
	wnd->MoveWindow(&wndRect, TRUE);

	GUID dimension_id;
	m_gif->GetFrameDimensionsList(&dimension_id, 1);
	m_numFrames = m_gif->GetFrameCount(&dimension_id);
	UINT frame_delay_size = m_gif->GetPropertyItemSize(PropertyTagFrameDelay);

	m_frameDelayData.Alloc(frame_delay_size);
	Gdiplus::PropertyItem* frame_delay_item = reinterpret_cast<Gdiplus::PropertyItem*>(m_frameDelayData(0));
	m_gif->GetPropertyItem(PropertyTagFrameDelay, frame_delay_size, frame_delay_item);
	m_frameDelays = reinterpret_cast<const UINT*>(frame_delay_item->value);

	return;
}

UINT ThreadProc(LPVOID pParam)
{
	CDlgPlanGetPacs* pacsDlg = (CDlgPlanGetPacs*)pParam;

	pacsDlg->ExecutePACSThread();

	return 0;
}

void CDlgPlanGetPacs::ExecutePACSThread(void)
{
	if (m_threadType == THREAD_TYPE_PLANS)
	{
		m_numIonPlans = m_pacsPipe->GetInfoPatientPlans(m_patientID);
	}
	else
	{
		if (m_pacsPipe->GetPlanFileSet(m_planIndex))
		{
			GetDlgItem(IDC_STC_STATUS)->SetWindowTextA("Reading Plan Files");
			m_pacsPipe->ConvertStudyDicom2Json(m_planIndex);
		}
		else
		{
			AfxMessageBox("Could not create patient folder.");
		}
	}

	PostMessage(MSG_THREAD_DONE, 0, 0);

	return;
}

LRESULT CDlgPlanGetPacs::OnThreadComplete(WPARAM wParam, LPARAM lParam)
{
	if (m_threadType == THREAD_TYPE_PLANS)
	{
		PostThreadPlans();
	}
	else
	{
		PostThreadStudy();
	}

	return 0;
}

void CDlgPlanGetPacs::RetrievePlans(void)
{

	m_threadType = THREAD_TYPE_PLANS;

	UpdateData();

	if (m_patientID.IsEmpty())
	{
		AfxMessageBox("No Patient ID Entered.");

		return;
	}

	AfxBeginThread(ThreadProc, this);

	GetDlgItem(IDC_BTN_QUERY)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_STC_STATUS)->SetWindowTextA("Retrieving Patient Plans");

	GetDlgItem(IDC_STC_GIF)->ShowWindow(SW_SHOW);
	StartTimer();

	return;
}

void CDlgPlanGetPacs::PostThreadPlans(void)
{

	GetDlgItem(IDC_STC_GIF)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_STC_STATUS)->SetWindowTextA("");

	if (m_numIonPlans > 0)
	{
		CUniArray<SPlanInfo>* arrayPlans = m_pacsPipe->GetPlans();

		int indexPlan = 0;
		forPrrayN(arrayPlans, plan, m_numIonPlans)
		{
			m_listCntrl.InsertItem(indexPlan, plan->planDate);

			m_listCntrl.SetItemText(indexPlan, 1, plan->seriesDescr);
			m_listCntrl.SetItemText(indexPlan, 2, plan->planName);
			m_listCntrl.SetItemText(indexPlan, 3, plan->planDescr);
			++indexPlan;
		}

		GetDlgItem(IDC_LIST1)->ShowWindow(SW_SHOW);
		GetDlgItem(IDC_BTN_RETRIEVE)->ShowWindow(SW_SHOW);
		GetDlgItem(IDC_STC_SELECT)->ShowWindow(SW_SHOW);
		GetDlgItem(IDC_BTN_RETRIEVE)->ShowWindow(SW_SHOW);
	}
	else
	{
		if (m_numIonPlans == 0)
		{
			AfxMessageBox("No RTIon plans found.");
		}
		else
		{
			AfxMessageBox("PACS communication error.");
		}

		GetDlgItem(IDC_BTN_QUERY)->ShowWindow(SW_SHOW);

		m_threadType = THREAD_TYPE_NONE;
	}

	return;
}

void CDlgPlanGetPacs::RetrieveStudy(void)
{

	m_threadType = THREAD_TYPE_STUDY;

	POSITION pos = m_listCntrl.GetFirstSelectedItemPosition();

	if (pos == NULL)
	{
		AfxMessageBox("No Plan Selected.");

		return;
	}

	m_planIndex = m_listCntrl.GetNextSelectedItem(pos);

	SPlanInfo* plan = m_pacsPipe->GetPlan(m_planIndex);
	m_fileNameBasePlan = plan->planName + ".json";

	AfxBeginThread(ThreadProc, this);

	GetDlgItem(IDC_LIST1)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_STC_SELECT)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_BTN_RETRIEVE)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_STC_STATUS)->SetWindowTextA("Retrieving Study");

	GetDlgItem(IDC_STC_GIF)->ShowWindow(SW_SHOW);
	StartTimer();

	return;
}

void CDlgPlanGetPacs::PostThreadStudy(void)
{
	KillTimer(0);
	Gdiplus::GdiplusShutdown(m_gdiToken);

	m_pathNamePlan.Format("%s\\OUT", m_pacsPipe->GetPlanDirectory());

	CDialog::OnOK();

	return;
}

void CDlgPlanGetPacs::StartTimer(void)
{

	m_frameIndex = 0;
	m_gifTimer = SetTimer(0, m_frameDelays[m_frameIndex], NULL);

	return;
}

afx_msg void CDlgPlanGetPacs::OnTimer(UINT_PTR nIDEvent)
{

	Gdiplus::Graphics g(GetDlgItem(IDC_STC_GIF)->GetSafeHwnd());
	GUID dim_select_id = Gdiplus::FrameDimensionTime;
	m_gif->SelectActiveFrame(&dim_select_id, m_frameIndex);
	g.DrawImage(m_gif, 0, 0, m_gif->GetWidth(), m_gif->GetHeight());

	++m_frameIndex;
	if (m_frameIndex >= m_numFrames)
	{
		m_frameIndex = 0;
	}

	m_gifTimer = SetTimer(0, m_frameDelays[m_frameIndex]*8, NULL);

	return;
}

void CDlgPlanGetPacs::OnOK(void)
{
	if (m_threadType == THREAD_TYPE_NONE)
	{
		RetrievePlans();
	}

	return;
}


void CDlgPlanGetPacs::OnLvnItemchangedList1(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	// TODO: Add your control notification handler code here
	*pResult = 0;
}
