#include "stdafx.h"
#include "Heapster.h"
#include "JsonParser.h"
#include "IonPlan.h"
#include "DicomDose.h"
#include "Transport.h"
#include "EngineTransport.h"
#include "AppAutoRun.h"

struct SAppConfig
{
  CString m_jsonDir;
  CString m_jsonFileName;
  CString m_doseDscrpt;

} SAppconfig;

CAppAutoRun::CAppAutoRun(void) :
  m_ionPlan   (NULL)
{

  m_gpu.InitializeGPU(NULL); // MLW todo: add adapter

  return;
}

CAppAutoRun::~CAppAutoRun(void)
{
  return;
}

void CAppAutoRun::Run(FILE* fd)
{
  SAppConfig appConfig;

  ParseConfigFile(fd, &appConfig);

  CIonPlan ionPlan;

  CString planName;
  planName.Format("%s\\%s", appConfig.m_jsonDir, appConfig.m_jsonFileName);
  FILE* planFD = fopen(planName, "r");

  ParseIonPlan(planFD, &ionPlan, &appConfig.m_jsonDir);
  fclose(planFD);

  Compute(&appConfig, &ionPlan);

  return;
}

bool CAppAutoRun::ParseConfigFile(FILE* fd, SAppConfig* config)
{
  CJsonParser jparser;

  ReturnOnFalse(jparser.ParseFile(fd))

  Json::Value* root = jparser.GetRoot();

  config->m_jsonDir = (*root)["JSONDir"].asCString();

  m_transportParams.numCycles          = (*root)["NumCycles"].asInt();
  m_transportParams.protonScaleFactor  = (*root)["ProtonScale"].asInt();
  m_transportParams.statsCalc          = (*root)["RunFlags"].asInt();

  config->m_doseDscrpt   = (*root)["DoseDescription"].asCString();
  config->m_jsonFileName = (*root)["JSONFileName"].asCString();

  return true;
}

bool CAppAutoRun::ParseIonPlan(FILE* fd, CIonPlan* ionPlan, CString* pathName)
{

  ReturnOnFalse(ionPlan->ParseJSON(fd))

  ReturnOnFalse(ionPlan->ParseBlobs((LPCTSTR)pathName));

  return true;
}

void CAppAutoRun::Compute(SAppConfig* config, CIonPlan* ionPlan)
{
  CEngineTransport engineTransport;

  engineTransport.Initialize();

  engineTransport.LoadRTIonPlan(ionPlan);

  m_transportParams.spotProtonsConstant = false;  // MLW todo: add spotProtonsConstant to config

  engineTransport.PrepCompute(&m_transportParams);

  engineTransport.ComputePlanDose(&m_transportParams); // MLW todo: add statistics flag to config

  CString doseFileName;
  doseFileName.Format("%s\\%s_Dose.dcm", config->m_jsonDir, config->m_doseDscrpt);

  //engineTransport.SaveDoseDicom(doseFileName, config->m_doseDscrpt); // MLW todo: change to enginePostCalc.++

  return;
}


