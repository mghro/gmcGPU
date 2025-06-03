#include "stdafx.h"
#include "resource.h"
#include "UniArray.h"
#include "Transport.h"
#include "DlgResultsSaveDicom.h"

BEGIN_MESSAGE_MAP(CDlgResultsSaveDicom, CDialogEx)
	ON_BN_CLICKED(IDC_CHK_ALL, ProcessAll)
	ON_COMMAND_RANGE(IDC_CHECK1, IDC_CHECK6, ProcessSingle)
END_MESSAGE_MAP()

CDlgResultsSaveDicom::CDlgResultsSaveDicom(bool doseAux, bool doseGamma) : CDialogEx(IDD_DLG_SAVE_DICOM, NULL),
  m_doseAux       (doseAux),
	m_doseGamma     (doseGamma),
	m_dosesSelected (0)
{
	return;
}

CDlgResultsSaveDicom::~CDlgResultsSaveDicom(void)
{
	return;
}

void CDlgResultsSaveDicom::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);

	DDX_Text(pDX, IDC_EDT_DESCRIPTION, m_description);

	return;
}

BOOL CDlgResultsSaveDicom::OnInitDialog(void)
{

	if (m_doseAux == false)
	{
		HideDoseAuxSelections();

		CWnd* wnd = GetDlgItem(IDC_STATIC);

		wnd->SetWindowTextA("Save Primary Dose");
	}

	if (m_doseGamma == false)
	{
		GetDlgItem(IDC_CHECK8)->ShowWindow(SW_HIDE);
	}

	((CButton*)GetDlgItem(IDC_CHECK1))->SetCheck(BST_CHECKED);

	return TRUE;
}

void CDlgResultsSaveDicom::HideDoseAuxSelections(void)
{

	for (int index = IDC_CHECK2; index <= IDC_CHECK7; ++index)
	{
		GetDlgItem(index)->ShowWindow(SW_HIDE);
	}

	GetDlgItem(IDC_CHK_ALL)->ShowWindow(SW_HIDE);

	return;
}

void CDlgResultsSaveDicom::ProcessAll(void)
{

  if (((CButton*)GetDlgItem(IDC_CHK_ALL))->GetCheck() == BST_CHECKED)
  {
    for (int index = IDC_CHECK1; index <= IDC_CHECK8; ++index)
    {
      ((CButton*)GetDlgItem(index))->SetCheck(BST_CHECKED);
    }
  }

  return;
}

void CDlgResultsSaveDicom::ProcessSingle(UINT nID)
{

	CButton* buttonAll    = (CButton*)GetDlgItem(IDC_CHK_ALL);
	CButton* buttonSelect = (CButton*)GetDlgItem(nID);

	// If All checked and unchecking a field, uncheck All
	if ( (buttonAll->GetCheck() == BST_CHECKED) && (buttonSelect->GetCheck() == BST_UNCHECKED))
	{
		buttonAll->SetCheck(BST_UNCHECKED);
	}

	return;
}

int CDlgResultsSaveDicom::GetSaveFields(void)
{
	return m_dosesSelected;
}

void CDlgResultsSaveDicom::CalcSelectedDoses(void)
{

	m_dosesSelected = 0;

	for (int index = IDC_CHECK1; index <= IDC_CHECK8; ++index)
	{
		if (((CButton*)GetDlgItem(index))->GetCheck() == BST_CHECKED)
		{
			m_dosesSelected |= 1 << (index - IDC_CHECK1);
		}
	}

	return;
}

void CDlgResultsSaveDicom::OnOK(void)
{

	UpdateData(TRUE);

	CalcSelectedDoses();

	if (m_dosesSelected == 0)
	{
		AfxMessageBox("No Dose Field Selected.");
	}
	else
	{
		CDialog::OnOK();
	}

	return;
}
