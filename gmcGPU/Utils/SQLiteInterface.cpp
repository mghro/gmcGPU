#include "StdAfx.h"
#include "sqlite3.h"
#include "SQLiteInterface.h"

CSQLiteInterface::CSQLiteInterface(void) :
  m_db(NULL)
{
  return;
}

CSQLiteInterface::~CSQLiteInterface(void)
{

}

//_____________________________________________________________________________
//
//  bool CSQLiteInterface::Open(LPCTSTR dbName, bool writeStatus)
//
//  If trying to write to a file it is opened with sqlite3_open which will
//  create the file if it does not exist.
//  If just trying to read a file it is opened with sqlite3_open_v2 which does
//  not create the file if it does not exist.
//_____________________________________________________________________________

bool CSQLiteInterface::Open(LPCTSTR dbName, bool writeStatus)
{
  int status;

  if (writeStatus)
  {
    status = sqlite3_open(dbName, &m_db);
  }
  else
  {
    status = sqlite3_open_v2(dbName, &m_db, SQLITE_OPEN_READONLY, NULL);
  }

  return(status ? false : true);
}

bool CSQLiteInterface::ExecuteCommand(CString& command, QueryCallback callback, void* data)
{
  char* errorMsg = 0;

  int sqlStatus = sqlite3_exec(m_db, command, callback, data, &errorMsg);

  if (sqlStatus != SQLITE_OK)
  {
    sqlite3_free(errorMsg);
  }

  return(sqlStatus ? false : true);
}

int CSQLiteInterface::GetNumRows(CString& tableName)
{
  CString query;
  int queryResult;
  int numRows;

  query.Format("SELECT COUNT (*) from ");
  query += tableName;

  sqlite3_stmt *stmt;
  if (sqlite3_prepare_v2(m_db, query, -1, &stmt, NULL) != SQLITE_OK)
  {
    return -1;
  }
  else
  {
    queryResult = sqlite3_step(stmt);

    if (queryResult == SQLITE_ROW)
    {
      numRows = sqlite3_column_int(stmt, 0);
    }
    else
    {
      numRows = -1;
    }

    sqlite3_finalize(stmt);
  }

  return numRows;
}

bool CSQLiteInterface::GetCommandString(LPCTSTR command, CString& stringValue)
{
  int queryResult;

  sqlite3_stmt *stmt;
  if (sqlite3_prepare_v2(m_db, command, -1, &stmt, NULL) != SQLITE_OK)
  {
    return false;
  }
  else
  {
    queryResult = sqlite3_step(stmt);

    if (queryResult == SQLITE_ROW)
    {
      stringValue = sqlite3_column_text(stmt, 0);
    }
    else
    {
      return false;
    }

    sqlite3_finalize(stmt);
  }

  return true;
}

bool CSQLiteInterface::GetCommandInt(LPCTSTR command, int* intValue)
{
  int queryResult;

  sqlite3_stmt *stmt;
  if (sqlite3_prepare_v2(m_db, command, -1, &stmt, NULL) != SQLITE_OK)
  {
    return false;
  }
  else
  {
    queryResult = sqlite3_step(stmt);

    if (queryResult == SQLITE_ROW)
    {
      *intValue = sqlite3_column_int(stmt, 0);
    }
    else
    {
      return false;
    }

    sqlite3_finalize(stmt);
  }

  return true;
}

void CSQLiteInterface::Close(void)
{

  sqlite3_close(m_db);

  return;
}

