#pragma once

class CSliceHedra
{
public:

  CSliceHedra(void);
  CSliceHedra(LPCTSTR structName, float* color);
  ~CSliceHedra(void);

  inline LPCTSTR GetName(void) { return m_name; }

private:

  CString m_name;
};

