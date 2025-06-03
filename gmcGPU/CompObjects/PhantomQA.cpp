#include "stdafx.h"
#include "JsonParser.h"
#include "PhantomQA.h"

CPhantomQA::CPhantomQA(void)
{

  return;
}

CPhantomQA::~CPhantomQA(void)
{
  return;
}

bool CPhantomQA::ParseJSON(FILE* fd)
{

  //CJsonParser jparser;
  //bool status = jparser.ParseFile(fd);

  //Json::Value* root = jparser.GetRoot();

  m_voxGridRSP.voxelOrigin[0] = -248.0;
  m_voxGridRSP.voxelOrigin[1] =  1.0;
  m_voxGridRSP.voxelOrigin[2] = -174.0;
  m_voxGridRSP.dims[0]        = 248;
  m_voxGridRSP.dims[1]        = 183;
  m_voxGridRSP.dims[2]        = 177;
  m_voxGridRSP.voxelSize[0]   = 2.0;
  m_voxGridRSP.voxelSize[1]   = 2.0;
  m_voxGridRSP.voxelSize[2]   = 2.0;
  m_voxGridRSP.voxelVolume    = m_voxGridRSP.voxelSize[0] * m_voxGridRSP.voxelSize[1] * m_voxGridRSP.voxelSize[2];
  m_voxGridRSP.numVoxels      = m_voxGridRSP.dims[0] * m_voxGridRSP.dims[1] * m_voxGridRSP.dims[2];

  m_voxGridRSP.data = new float[m_voxGridRSP.numVoxels];
  float* end = m_voxGridRSP.data + m_voxGridRSP.numVoxels;
  for (float* fPtr = m_voxGridRSP.data; fPtr < end; ++fPtr)
  {
    *fPtr = 1.0;
  }

  m_voxGridHU = m_voxGridRSP;

  m_voxGridHU.data = new float[m_voxGridHU.numVoxels];
  memset(m_voxGridHU.data, 0, m_voxGridHU.numVoxels * sizeof(float));

  m_voxGridCalc.voxelOrigin[0] = -174.0;
  m_voxGridCalc.voxelOrigin[1] =  1.0;
  m_voxGridCalc.voxelOrigin[2] = -174.0;
  m_voxGridCalc.dims[0]        = 175;
  m_voxGridCalc.dims[1]        = 175;
  m_voxGridCalc.dims[2]        = 175;
  m_voxGridCalc.voxelSize[0]   = 2.0;
  m_voxGridCalc.voxelSize[1]   = 2.0;
  m_voxGridCalc.voxelSize[2]   = 2.0;
  m_voxGridCalc.voxelVolume    = m_voxGridCalc.voxelSize[0] * m_voxGridCalc.voxelSize[1] * m_voxGridCalc.voxelSize[2];
  m_voxGridCalc.numVoxels      = m_voxGridCalc.dims[0] * m_voxGridCalc.dims[1] * m_voxGridCalc.dims[2];

  return true;
}

bool CPhantomQA::ParseElements(Json::Value* root)
{

  return true;
}

bool CPhantomQAParseBlobs(LPCTSTR blobPath)
{

  return true;
}
