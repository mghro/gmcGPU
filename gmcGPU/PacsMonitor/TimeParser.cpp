#include "stdafx.h"
#include "TimeParser.h"

CTimeParser::CTimeParser(void)
{
  //CalcMonthHash();

  return;
}

CTimeParser::~CTimeParser(void)
{
  return;
}

COleDateTime* CTimeParser::Parse(char* datePos)
{
  strncpy(m_strMonth, datePos, 3);
  datePos += 4;
  strncpy(m_strDay, datePos, 2);
  datePos += 3;
  strncpy(m_strYear, datePos, 4);
  datePos += 5;
  strncpy(m_strHour, datePos, 2);
  datePos += 3;
  strncpy(m_strMinute, datePos, 2);
  datePos += 3;
  strncpy(m_strSecond, datePos, 2);

  int monthHash = *m_strMonth * 3 + *(m_strMonth + 1) * 2 + *(m_strMonth + 2);
  
  switch (monthHash)
  {
  case 526:
    m_month = 0;
    break;
  case 510:
    m_month = 1;
    break;
  case 539:
    m_month = 2;
    break;
  case 533:
    m_month = 3;
    break;
  case 546:
    m_month = 4;
    break;
  case 566:
    m_month = 5;
  break;
  case 564:
    m_month = 6;
    break;
  case 532:
    m_month = 7;
    break;
  case 563:
    m_month = 8;
    break;
  case 551:
    m_month = 9;
    break;
  case 574:
    m_month = 10;
    break;
  case 505:
    m_month = 11;
    break;
  }

  m_day    = atoi(m_strDay);
  m_year   = atoi(m_strYear);
  m_hour   = atoi(m_strHour);
  m_minute = atoi(m_strMinute);
  m_second = atoi(m_strSecond);

  m_timeTotal.SetDateTime(m_year, m_month, m_day, m_hour, m_minute, m_second);

  return &m_timeTotal;
}

void CTimeParser::CalcMonthHash(void)
{
  char* month;

  char* months[12] =
  {
    "Jan",
    "Feb",
    "Mar",
    "Apr",
    "May",
    "Jun",
    "Jul",
    "Aug",
    "Sep",
    "Oct",
    "Nov",
    "Dec"
  };

  for (int index = 0; index < 12; ++index)
  {
    month = months[index];
    m_hashTableMonths[index] = *month * 3 + *(month + 1) * 2 + *(month + 2);
  }

  return;
}

