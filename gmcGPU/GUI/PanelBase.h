#pragma once

#include "PanelTabCtrl.h"
#include "DicomMsgParams.h"

class  CAppState;
struct SPACSServer;

class CPanelBase : public CDialog
{
public:

  CPanelBase(CWnd* parent);
  ~CPanelBase(void);

  virtual void OnTabEnter(int prevPanel, void* data) {};

  static void SetAppState(CAppState* appState);

  LPCTSTR GetName(void)
  {
    return m_panelName;
  }

protected:

  //CPanelTabCtrl*  m_parent;
  CWnd* m_parent;

  CString m_panelName;

  static CAppState* m_appState;

};

class CStaticCustom : public CStatic
{
  CStaticCustom(void);
  ~CStaticCustom(void);

  HBRUSH CtlColor(CDC* pDC, UINT nCtlColor);

protected:

  CBrush m_brush;
};