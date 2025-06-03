#pragma once

#include "DirectX.h"

class CIonPlan;
struct SAppConfig;


class CAppAutoRun
{
public:

  CAppAutoRun(void);
  ~CAppAutoRun(void);

  void Run(FILE* fd);

private:

  bool ParseConfigFile(FILE* fd, SAppConfig* config);
  bool ParseIonPlan(FILE* fd, CIonPlan* ionPlan, CString* pathName);

  void Compute(SAppConfig* config, CIonPlan* ionPlan);
  void SaveDoseDicom(LPCTSTR fileName, LPCTSTR seriesDescr);


  CDirectX m_gpu;

  CIonPlan* m_ionPlan;

  STransportParams m_transportParams;

};