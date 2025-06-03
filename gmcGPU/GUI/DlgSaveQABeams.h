#pragma once

class CDlgSaveQABeams : public CDialogEx
{
public:

  CDlgSaveQABeams(void);
  ~CDlgSaveQABeams(void);

  // Quick & Dirty bitmask
  inline unsigned int GetStatusFormats(void) { return (unsigned int)(m_statusGamma + 2 * m_statusDicom); }

private:

  BOOL OnInitDialog(void);
  void DoDataExchange(CDataExchange* pDX);
  afx_msg void OnOK(void);

  int m_statusGamma;
  int m_statusDicom;

  DECLARE_MESSAGE_MAP()
};
