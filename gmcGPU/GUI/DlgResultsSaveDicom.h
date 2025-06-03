#pragma once

struct SPACSServer;

template <typename T> class  CUniArray;

class CDlgResultsSaveDicom : public CDialogEx
{
public:

	CDlgResultsSaveDicom(bool doseAux, bool doseGamma);
	~CDlgResultsSaveDicom(void);

	int GetSaveFields(void);

	CString& GetDescription(void)
	{
		return m_description;
	}

protected:

	BOOL OnInitDialog(void);
	virtual void DoDataExchange(CDataExchange* pDX);

	afx_msg void ProcessAll(void);
	afx_msg void ProcessSingle(UINT nID);

	void HideDoseAuxSelections(void);

	void CalcSelectedDoses(void);

	void OnOK(void);

	CUniArray<SPACSServer>* m_servers;

	CString m_description;
	int     m_dosesSelected;
	bool    m_doseAux;
	bool    m_doseGamma;

	DECLARE_MESSAGE_MAP()
};