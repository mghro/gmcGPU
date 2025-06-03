#pragma once

class CMsgBlinker : public CStatic
{
public:
  
  CMsgBlinker(void);
  ~CMsgBlinker(void);

  void StartTimer(void);
  void StopTimer(void);

private:

  afx_msg void OnTimer(UINT_PTR nIDEvent);

  ULONG_PTR m_gifTimer;

  int m_toggle;

  DECLARE_MESSAGE_MAP()
};

