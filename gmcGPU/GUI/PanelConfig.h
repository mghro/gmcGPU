#pragma once


class CPanelBase;
class CPanelPACSMonitor;

template <typename T> class  CUniArray;

class CPanelConfig : public CPanelBase
{
public:

  CPanelConfig(CPanelTabCtrl* parent);
  ~CPanelConfig(void);

private:

  BOOL OnInitDialog(void);
  void LoadServers(int wndIndex);
  void LoadAEs(int wndIndex, CUniArray<CString>* aeNames);

  void SetServerQuery(void);
  void SetAEQuery(void);
  void SetServerSave(void);
  void SetAESave(void);
  void SetAENameGmc(int indexCombo);
  void SetAEGMCQuery(void);
  void SetAEGMCSave(void);

  void SetMIMMonitorStatus(void);

  CUniArray<SPACSServer>* m_serversQuery;
  CUniArray<SPACSServer>* m_serversStore;

  SPACSServer* m_selectedServerQuery;
  SPACSServer* m_selectedServerStore;

  SDicomMsgParams m_dicomMsgParamsQuery = {};
  SDicomMsgParams m_dicomMsgParamsStore = {};

  CPanelPACSMonitor* m_panelPACSMonitor;

  DECLARE_MESSAGE_MAP()
};
