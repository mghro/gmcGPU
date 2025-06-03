#pragma once

#include "TimeParser.h"
#include "UniArray.h"


class CPacsMonitor
{
public:

  CPacsMonitor(void);
  ~CPacsMonitor(void);

  bool OpenLogFile(LPCTSTR logFile);

  char* ParseLogFile(COleDateTime* timeBoundary);

private:

  CTimeParser m_timeParser;

  CUniArray<char>  m_logBuffer;
  char*            m_bufferPos;

  char             m_uidIonPlan[65];

};