#pragma once
class CTimeParser
{
public:

  CTimeParser(void);
  ~CTimeParser(void);

  COleDateTime* Parse(char* startPos);

private:

  void CalcMonthHash(void);

  char m_strMonth[4]  = {};
  char m_strDay[3]    = {};
  char m_strYear[5]   = {};
  char m_strHour[3]   = {};
  char m_strMinute[3] = {};
  char m_strSecond[3] = {};

  int m_hashTableMonths[12];

  int m_month;
  int m_day;
  int m_year;
  int m_hour;
  int m_minute;
  int m_second;

  COleDateTime m_timeTotal;
};

