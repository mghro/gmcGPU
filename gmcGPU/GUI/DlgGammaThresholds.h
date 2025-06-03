#pragma once

struct SGammaThresholds;

class CDlgGammaThresholds : public CDialogEx
{
public:

  CDlgGammaThresholds(SGammaThresholds* gammaThreshes);
  ~CDlgGammaThresholds(void);

  virtual void DoDataExchange(CDataExchange* pDX);

private:

  SGammaThresholds* m_threshes;

};

