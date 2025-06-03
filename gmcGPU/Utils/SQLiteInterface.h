#pragma once
 
#include <afxstr.h>

struct sqlite3;

typedef int(*QueryCallback)(void *data, int argc, char **argv, char **azColName);

class CSQLiteInterface
{
public:

  CSQLiteInterface(void);
  ~CSQLiteInterface(void);

  bool Open(LPCTSTR dbName, bool writeStatus = false);
  bool ExecuteCommand(CString& command, QueryCallback callback, void* data);
  int	 GetNumRows(CString& tableName);
  bool GetCommandInt(LPCTSTR command, int* intValue);
  bool GetCommandString(LPCTSTR command, CString& stringValue);
  void Close(void);

private:

  sqlite3 *m_db;

};