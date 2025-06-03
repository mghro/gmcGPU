#pragma once

#include "UniArray.h"
#include "json.h"

using namespace Json;

struct SPACSServer;

class CAppConfigInstance
{
public:

  CAppConfigInstance(void);
  ~CAppConfigInstance(void);

  bool ParseJson(void);
  bool ParseAENamesGmc(Json::Value* jAENamesGmc);
  bool ParseServers(CUniArray<SPACSServer>* serverArray, Json::Value* jServers);

  CString m_directoryBase;
  CString m_gpuName;
  CString m_pathNameQAsave;
  CString m_mimMonitorLogFile;

  CUniArray<CString>     m_aeNamesGmc;

  CUniArray<SPACSServer> m_serversQuery;
  CUniArray<SPACSServer> m_serversStore;

private:

  

};

extern CAppConfigInstance* g_configInstance;
