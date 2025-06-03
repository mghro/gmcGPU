#pragma once

#include "SQLiteInterface.h"
#include "BlixBufferObj.h"

class CComputerInfo  : public CBlixBufferObj
{
public:

  CComputerInfo(void);
  ~CComputerInfo(void);

  bool Open(bool writeStatus);
  bool Read(void);
  bool Save(void);
  void ProcessDatabaseQuery(int argc, char **argv, char **azColName);
  void Close(void);

  int   CalcBufferSize(void);
  byte* FillBuffer(byte* buffer);
  byte* ReadFromBuffer(byte* buffer);

  CString m_hostname;
  CString m_owner;
  CString m_location;
  CString m_description;
  CStringA m_ipAddress;

private:

  bool CreateDatabase(void);

  CString           m_dbFileName;
  CSQLiteInterface  m_sqliteInterface;
};