#pragma once

#include <time.h>
#include "AppConfigInstance.h"

#define APPLOGFILE    \
FILE*  g_appLogFile;  \
DWORD  g_timeStart;   \
DWORD  g_timePrev;    \
DWORD  g_timeNow;

extern FILE*  g_appLogFile;
extern DWORD  g_timeStart;
extern DWORD  g_timePrev;
extern DWORD  g_timeNow;

// Log text only
#define APPLOG(format, ...)                \
fprintf(g_appLogFile, format, __VA_ARGS__); \
fflush(g_appLogFile);

// Log with elapsed time from last APPLOGT
#define APPLOGT(format, ...)    \
{                  \
g_timeNow = GetTickCount(); \
fprintf(g_appLogFile, format, __VA_ARGS__); \
fprintf(g_appLogFile, "\nElapsed Time %d ms\n\n", (g_timeNow - g_timePrev)); \
g_timePrev = g_timeNow; \
fflush(g_appLogFile);   \
} 

// 4D info is time and code position
#define APPLOG4D(format, ...)    \
{                     \
CString fileLine;     \
fileLine.Format("%s:%-4d", strrchr("\\" __FILE__, '\\') + 1, __LINE__); \
fprintf(g_appLogFile, "\n%-32s | %-40s | %s", fileLine, __FUNCTION__, (GetCurrentTime() - g_timeStart)); \
APPLOG(format, ...) \
}  

#define APPLOGINIT()  \
CString logFileName;  \
CTime rawLogTime = CTime::GetCurrentTime(); \
logFileName.Format("%s\\AppLogs\\AppLogGMC_%s.txt", (LPCTSTR) g_configInstance->m_directoryBase, rawLogTime.Format("%Y_%d_%b_%H-%M-%S")); \
g_appLogFile = fopen(logFileName, "w");     \
if(g_appLogFile == NULL) { \
CString msg; \
msg.Format("Could not open log file\r\n%s\r\nExiting.", (LPCTSTR) logFileName); \
AfxMessageBox(msg); exit(0);} \
else { \
g_timePrev = g_timeStart = GetTickCount(); \
APPLOG("App Start %s\n", rawLogTime.Format("%Y_%d_%b_%H-%M-%S"))}

#define DBGLOGFILE(Bag)   \
FILE* g_dbg##Bag;

#define DBGLOGFILE_USE(Bag) \
extern FILE* g_dbg##Bag;

#define DBGLOG4D(Bag) \
{                     \
CString fileLine;     \
fileLine.Format("%s:%-4d", strrchr("\\" __FILE__, '\\') + 1, __LINE__); \
fprintf(g_dbg##Bag, "%-32s | %-32s | %s", fileLine, __FUNCTION__, (GetCurrentTime() - g_timeStart)); \
} 

// Log without 4D info
#define DBGLOG0(Bag, format, ...)         \
fprintf(g_dbg##Bag, format, __VA_ARGS__); \
fflush(g_dbg##Bag);

#define DBGLOG(Bag, format, ...)          \
DBGLOG4D(Bag)                             \
fprintf(g_dbg##Bag, format, __VA_ARGS__); \
fflush(g_dbg##Bag);

#define DBGLOGINIT(Bag)  \
{                        \
time(&g_logTime);        \
CString dbgFileName;     \
CTime rawBagTime = CTime::GetCurrentTime(); \
dbgFileName.Format("%s\\AppLogs\\DbgLog%s_%s.txt", g_configInstance->m_directoryBase, #Bag, rawBagTime.Format("%Y_%d_%b_%H-%M-%S")); \
g_dbg##Bag = fopen(dbgFileName, "w");       \
}                        \
APPLOG(" Start\n", NULL)

#define DBGBINARYDUMP(Bag, data, length) \
FILE* g_dbg##Bag;        \
CString dbgFileName;     \
CTime rawBagTime = CTime::GetCurrentTime(); \
dbgFileName.Format("%s\\AppLogs\\DbgDump%s_%s.bin", g_configInstance->m_directoryBase, #Bag, rawBagTime.Format("%Y_%d_%b_%H-%M-%S")); \
g_dbg##Bag = fopen(dbgFileName, "wb"); \
fwrite(data, length, 1, g_dbg##Bag);   \
fclose(g_dbg##Bag);

#define BLIXDBG_BSTREAM_FILE(Bag)   \
FILE* g_dbgStream##Bag = NULL;

#define BLIXDBG_BSTREAM_INIT(Bag)  \
{                      \
CString dbgFileName;   \
CTime rawBagTime = CTime::GetCurrentTime();  \
dbgFileName.Format("%s\\AppLogs\\DbgStream%s_%s.bin", g_configInstance->m_directoryBase, #Bag, rawBagTime.Format("%Y_%d_%b_%H-%M-%S")); \
g_dbgStream##Bag = fopen(dbgFileName, "wb"); \
}

#define BLXDBG_BSTREAM_ADD(Bag, data, length) \
fwrite(data, length, 1, g_dbgStream##Bag);    \
fflush(g_dbgStream##Bag);

#define BLXDBG_BSTREAM_CLOSE(Bag, data, length) \
fclose(g_dbgStream##Bag);











