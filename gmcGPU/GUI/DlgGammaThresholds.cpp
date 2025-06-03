#include "stdafx.h"
#include "resource.h"
#include "GammaIO.h"
#include "DlgGammaThresholds.h"


CDlgGammaThresholds::CDlgGammaThresholds(SGammaThresholds* gammaThrehses) : CDialogEx(IDD_DLG_GAMMA_THESHES, NULL),
  m_threshes (gammaThrehses)
{

  return;
}

CDlgGammaThresholds::~CDlgGammaThresholds(void)
{
  return;
}

void CDlgGammaThresholds::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);

  DDX_Text(pDX, IDC_EDIT1, m_threshes->threshDistanceMM);
  DDX_Text(pDX, IDC_EDIT2, m_threshes->threshDosePrcnt);
  DDX_Text(pDX, IDC_EDIT3, m_threshes->threshDoseMinPrcnt);

  
  if (pDX->m_bSaveAndValidate)
  {
    DDX_Text(pDX, IDC_EDIT1, m_threshes->threshDistanceMM);
    DDV_MinMaxFloat(pDX, m_threshes->threshDistanceMM, 0, 10.0);

    DDX_Text(pDX, IDC_EDIT2, m_threshes->threshDosePrcnt);
    DDV_MinMaxFloat(pDX, m_threshes->threshDosePrcnt, 0.1, 100.0);

    DDX_Text(pDX, IDC_EDIT3, m_threshes->threshDoseMinPrcnt);
    DDV_MinMaxFloat(pDX, m_threshes->threshDoseMinPrcnt, 0.1, 100.0);
  }

	return;
}
