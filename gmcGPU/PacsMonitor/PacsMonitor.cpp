#include "stdafx.h"
#include "TimeParser.h"
#include "PacsMonitor.h"

CPacsMonitor::CPacsMonitor(void)
{
  return;
}

CPacsMonitor::~CPacsMonitor(void)
{
  return;
}

bool CPacsMonitor::OpenLogFile(LPCTSTR logFile)
{
  FILE* fd = fopen(logFile, "r");

  if (fd == NULL)
  {
    return false;
  }

  fseek(fd, 0, SEEK_END);
  long int fileSize = ftell(fd);
  fseek(fd, 0, SEEK_SET);

  m_logBuffer.Alloc(fileSize + 1);

  fread(m_logBuffer(0), 1, fileSize, fd);
  m_logBuffer[fileSize] = 0;
  m_bufferPos = m_logBuffer(0);

  fclose(fd);
  fd = NULL;

  return true;
}

char* CPacsMonitor::ParseLogFile(COleDateTime* timeBoundary)
{
  COleDateTime* timePlanStore;

  char* filePosPlan;
  char* filePosTime;
  char* filePosNewLine;
  int   uidLength;
  
  while (m_bufferPos && ((filePosPlan = strstr(m_bufferPos, "RT Ion Plan Storage")) != NULL))
  {
    *(filePosPlan - 1) = 0;
    filePosNewLine = strrchr(m_bufferPos, 10);
    *(filePosNewLine  - 1) = 0;
    filePosNewLine = strrchr(m_bufferPos, 10);
    filePosTime = filePosNewLine + 1;

    timePlanStore = m_timeParser.Parse(filePosTime);

    m_bufferPos = filePosPlan;

    if (timePlanStore > timeBoundary)
    {
      m_bufferPos = strstr(m_bufferPos, "iuid=");
      m_bufferPos += 5;
      filePosNewLine = strchr(m_bufferPos, 10);

      uidLength = filePosNewLine - m_bufferPos;
      strncpy(m_uidIonPlan, m_bufferPos, uidLength);
      m_uidIonPlan[uidLength] = 0;

      return m_uidIonPlan;
    }
  }

  return NULL;
}
