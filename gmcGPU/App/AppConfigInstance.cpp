#include "stdafx.h"
#include <tchar.h>
#include "Heapster.h"
#include "UniArray.h"
#include "JsonParser.h"
#include "PACSServer.h"
#include "AppConfigInstance.h"

CAppConfigInstance::CAppConfigInstance(void)
{
  return;
}

CAppConfigInstance::~CAppConfigInstance(void)
{
  return;
}

bool CAppConfigInstance::ParseJson(void)
{
  FILE* fd = fopen(".\\GMCConfigInstance.json", "r");
  ReturnOnNull(fd);

  CJsonParser jParser;

  ReturnOnFalse(jParser.ParseFile(fd));

  fclose(fd);

  Json::Value* root = jParser.GetRoot();
  ReturnOnNull(root);

  ReturnOnFalse((*root).isMember("BaseDirectory"));
  m_directoryBase = (*root)["BaseDirectory"].asCString();

  // Convert possible relative path to absolute path
  // Needed because command scripts built from this and Windows does not allow
  //   using relative paths to batch files
  TCHAR NPath[MAX_PATH];
  _fullpath(NPath, m_directoryBase, MAX_PATH);
  m_directoryBase = NPath;

  if ((*root).isMember("GPUName"))
  {
    m_gpuName = (*root)["GPUName"].asCString();
  }
  else
  {
    m_gpuName = "NVIDIA";
  }

  if ((*root).isMember("QASaveDirectory"))
  {
    m_pathNameQAsave = (*root)["QASaveDirectory"].asCString();
  }
  else
  {
    m_pathNameQAsave.Empty();
  }

  // Parse GMC calling AE title names
  Json::Value aeNamesGMC;
  aeNamesGMC = (*root)["AENamesGmcSequence"];
  ParseAENamesGmc(&aeNamesGMC);

  Json::Value jServers;

  // Get the DICOM servers for querying
  ReturnOnFalse((*root).isMember("PACSServerQuerySequence"));
  jServers = (*root)["PACSServerQuerySequence"];

  ReturnOnFalse((jServers.size() > 0));
  ReturnOnFalse(m_serversQuery.Alloc(jServers.size()));
  ReturnOnFalse(ParseServers(&m_serversQuery, &jServers));

  // Get the DICOM servers for saving
  ReturnOnFalse((*root).isMember("PACSServerSaveSequence"));
  jServers = (*root)["PACSServerSaveSequence"];

  ReturnOnFalse((jServers.size() > 0));
  ReturnOnFalse(m_serversStore.Alloc(jServers.size()));
  ReturnOnFalse(ParseServers(&m_serversStore, &jServers));

  if ((*root).isMember("MIMMonitorLogFile"))
  {
    m_mimMonitorLogFile = (*root)["MIMMonitorLogFile"].asCString();
  }
  else
  {
    m_mimMonitorLogFile.Empty();
  }

  return true;
}

bool CAppConfigInstance::ParseAENamesGmc(Json::Value* jAENamesGmc)
{
  ReturnOnFalse(m_aeNamesGmc.Alloc(jAENamesGmc->size()));

  forArrayI(m_aeNamesGmc, aeName, aeIndex)
  {
    *aeName = (*jAENamesGmc)[aeIndex]["AEGmcName"].asCString();
  }

  return true;
}

bool CAppConfigInstance::ParseServers(CUniArray<SPACSServer>* serverArray, Json::Value* jServers)
{

  forPrrayI(serverArray, server, serverIndex)
  {
    server->serverName      = (*jServers)[serverIndex]["ServerName"].asCString();
    server->serverIPAddress = (*jServers)[serverIndex]["ServerAddress"].asCString();
    server->serverPort      = (*jServers)[serverIndex]["ServerPort"].asCString();

    ReturnOnFalse((*jServers)[serverIndex].isMember("AESequence"));

    Json::Value jAE = (*jServers)[serverIndex]["AESequence"];

    ReturnOnFalse(server->aeNames.Alloc(jAE.size()));

    forArrayI(server->aeNames, aeName, aeIndex)
    {
      *aeName = jAE[aeIndex]["AEName"].asCString();
    }
  }

  return true;
}
