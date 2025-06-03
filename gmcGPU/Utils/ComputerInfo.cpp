#include "stdafx.h"
#include "BlixLogger.h"
#include "ComputerInfo.h"

CComputerInfo::CComputerInfo(void)
{
  m_dbFileName.Format("%s\\Blix\\ComputerInfo.db", getenv("ProgramData"));

  return;
}

CComputerInfo::~CComputerInfo(void)
{
  return;
}

bool CComputerInfo::Open(bool writeStatus)
{
  bool status;
  CFileStatus fileStatus;

  if (!CFile::GetStatus(m_dbFileName, fileStatus))
  {
    status = CreateDatabase();
  }

  status = m_sqliteInterface.Open(m_dbFileName, writeStatus);

  return status;
}

void CComputerInfo::Close(void)
{
  m_sqliteInterface.Close();

  return;
}

bool CComputerInfo::CreateDatabase(void)
{
  CString sqlTable;
  bool status;

  status = m_sqliteInterface.Open(m_dbFileName, true);

  if (status)
  {

    RUNLOG("Succuessfully created ComputerInfo database file. %s\n", m_dbFileName)

    /* Create SQL statement */
    sqlTable = "CREATE TABLE COMPUTERINFO ("  \
      "ID						INTEGER		PRIMARY KEY,"   \
      "HOSTNAME			TEXT			NOT_NULL,"		  \
      "OWNER				TEXT			NOT_NULL,"		  \
      "LOCATION			TEXT      NOT NULL,"		  \
      "DESCRIPTION	TEXT      NOT NULL );";

    status = m_sqliteInterface.ExecuteCommand(sqlTable, NULL, NULL);

    if (status == false)
    {
      RUNLOG("Could not create COMPUTERINFO database table.\n", NULL)
    }

    m_sqliteInterface.Close();
  }
  else
  {
    RUNLOG("Could not create ComputerInfo database file. %s\n", m_dbFileName)
  }

  return status;
}

int ComputerInfoQueryCallback(void *data, int argc, char **argv, char **azColName)
{
  CComputerInfo* computerInfo = (CComputerInfo*)data;

  computerInfo->ProcessDatabaseQuery(argc, argv, azColName);

  return 0;
}

bool CComputerInfo::Read(void)
{
  CString fileName;

  fileName.Format("%s\\Blix\\ComputerInfo.db", getenv("ProgramData"));

  CString command("SELECT * FROM COMPUTERINFO");

  bool status = m_sqliteInterface.ExecuteCommand(command, ComputerInfoQueryCallback, this);

  if (status == false)
  {
    RUNLOG("Could not query ComputerInfo database table.\n", NULL)
  }

  return status;
}

void CComputerInfo::ProcessDatabaseQuery(int argc, char **argv, char **azColName)
{
  char** column = azColName;
  char** value = argv;

  for (int i = 0; i < argc; ++i)
  {
    if (strcmp(*azColName, "HOSTNAME") == 0)
    {
      m_hostname = *value;
    }
    else if (strcmp(*azColName, "OWNER") == 0)
    {
      m_owner = *value;
    }
    else if (strcmp(*azColName, "LOCATION") == 0)
    {
      m_location = *value;
    }
    else if (strcmp(*azColName, "DESCRIPTION") == 0)
    {
      m_description = *value;
    }
  
    ++value;
    ++azColName;
  }

  return;
}

bool CComputerInfo::Save(void)
{
  CString dbName;
  bool status = true;

  CString table = _T("COMPUTERINFO");
  CString computerInfo;

  int numRows = m_sqliteInterface.GetNumRows(table);

  if (numRows > 0)
  {
    computerInfo.Format("DELETE from COMPUTERINFO");

    status = m_sqliteInterface.ExecuteCommand(computerInfo, NULL, NULL);

    if (status)
    {
      computerInfo.Format(                                                   \
        "INSERT INTO COMPUTERINFO (HOSTNAME, OWNER, LOCATION, DESCRIPTION) " \
        "VALUES('%s', '%s', '%s', '%s');",

      m_hostname, m_owner, m_location, m_description);

      status = m_sqliteInterface.ExecuteCommand(computerInfo, NULL, NULL);

      if (status)
      {
        RUNLOG("Successfully save COMPUTERINFO table.\n", NULL)
      }
      else
      {
        RUNLOG("Could not save COMPUTERINFO table.\n", NULL)
      }
    }
    else
    {
      RUNLOG("Could not clear COMPUTERINFO table.\n", NULL)
    }
  }

  return status;
}

int CComputerInfo::CalcBufferSize(void)
{
  m_numBufferBytes = (m_hostname.GetLength() + m_owner.GetLength() + m_location.GetLength() +
    m_description.GetLength()) * sizeof(TCHAR) + 5 * sizeof(int);

  return m_numBufferBytes;
}

byte* CComputerInfo::FillBuffer(byte* buffer)
{
  int   numStringBytes;
  byte* bufferPtr;

  bufferPtr = buffer;

  memcpy(bufferPtr, &m_numBufferBytes, sizeof(int));
  bufferPtr += sizeof(int);

  numStringBytes = m_hostname.GetLength() * sizeof(TCHAR);
  memcpy(bufferPtr, &numStringBytes, sizeof(int));
  bufferPtr += sizeof(int);
  memcpy(bufferPtr, m_hostname.GetBuffer(), numStringBytes);
  m_hostname.ReleaseBuffer();
  bufferPtr += numStringBytes;

  numStringBytes = m_owner.GetLength() * sizeof(TCHAR);
  memcpy(bufferPtr, &numStringBytes, sizeof(int));
  bufferPtr += sizeof(int);
  memcpy(bufferPtr, m_owner.GetBuffer(), numStringBytes);
  m_owner.ReleaseBuffer();
  bufferPtr += numStringBytes;

  numStringBytes = m_location.GetLength() * sizeof(TCHAR);
  memcpy(bufferPtr, &numStringBytes, sizeof(int));
  bufferPtr += sizeof(int);
  memcpy(bufferPtr, m_location.GetBuffer(), numStringBytes);
  m_location.ReleaseBuffer();
  bufferPtr += numStringBytes;

  numStringBytes = m_description.GetLength() * sizeof(TCHAR);
  memcpy(bufferPtr, &numStringBytes, sizeof(int));
  bufferPtr += sizeof(int);
  memcpy(bufferPtr, m_description.GetBuffer(), numStringBytes);
  m_description.ReleaseBuffer();
  bufferPtr += numStringBytes;

  return bufferPtr;
}

byte* CComputerInfo::ReadFromBuffer(byte* buffer)
{
  byte* bufferPtr = buffer;

  int numBufferBytes;
  int numStringBytes;
  int numChars;

  numBufferBytes = *((int*)bufferPtr);
  bufferPtr += sizeof(int);

  numStringBytes = *((int*)bufferPtr);
  bufferPtr += sizeof(int);
  numChars = numStringBytes / sizeof(TCHAR);
  memcpy(m_hostname.GetBuffer(numChars), bufferPtr, numStringBytes);
  m_hostname.ReleaseBuffer();
  bufferPtr += numStringBytes;

  numStringBytes = *((int*)bufferPtr);
  bufferPtr += sizeof(int);
  numChars = numStringBytes / sizeof(TCHAR);
  memcpy(m_owner.GetBuffer(numChars), bufferPtr, numStringBytes);
  m_owner.ReleaseBuffer();
  bufferPtr += numStringBytes;

  numStringBytes = *((int*)bufferPtr);
  bufferPtr += sizeof(int);
  numChars = numStringBytes / sizeof(TCHAR);
  memcpy(m_location.GetBuffer(numChars), bufferPtr, numStringBytes);
  m_location.ReleaseBuffer();
  bufferPtr += numStringBytes;

  numStringBytes = *((int*)bufferPtr);
  bufferPtr += sizeof(int);
  numChars = numStringBytes / sizeof(TCHAR);
  memcpy(m_description.GetBuffer(numChars), bufferPtr, numStringBytes);
  m_description.ReleaseBuffer();
  bufferPtr += numStringBytes;

  return bufferPtr;
}
