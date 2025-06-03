#pragma once

#include "json.h"
#include "Transport.h"

class CPhantomQA
{
public:

  CPhantomQA(void);
  ~CPhantomQA(void);

  bool ParseJSON(FILE* fd);
  bool ParseElements(Json::Value* root);

  bool ParseBlobs(LPCTSTR blobPath);

  SDataGrid m_voxGridRSP;
  SDataGrid m_voxGridHU;

  SVoxGridGeometry m_voxGridCalc;

};

