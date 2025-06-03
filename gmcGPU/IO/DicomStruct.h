#pragma once

class CSliceHedra;

class CDicomStruct
{
public:

  CDicomStruct(void);
  ~CDicomStruct(void);

  bool ParseStructFile(LPCTSTR fileNameStruct, CSliceHedra* sliceHedra);
};

