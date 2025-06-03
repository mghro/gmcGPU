#include "stdafx.h"
#include "resource.h"
#include "DlgSaveQABeams.h"

BEGIN_MESSAGE_MAP(CDlgSaveQABeams, CDialogEx)

END_MESSAGE_MAP()

CDlgSaveQABeams::CDlgSaveQABeams(void) : CDialogEx(IDD_DLG_SAVE_QA, NULL),
	m_statusGamma (1),
	m_statusDicom (0)
{
  return;
}

CDlgSaveQABeams::~CDlgSaveQABeams(void)
{
  return;
}

BOOL CDlgSaveQABeams::OnInitDialog(void)
{
	UpdateData();

	return TRUE;
}

void CDlgSaveQABeams::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);

	DDX_Check(pDX, IDC_CHECK1, m_statusGamma);
	DDX_Check(pDX, IDC_CHECK2, m_statusDicom);

	return;
}

void CDlgSaveQABeams::OnOK(void)
{
	UpdateData(TRUE);

	CDialog::OnOK();

	return;
}
