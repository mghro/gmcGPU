#pragma once

#include "json.h"
#include "Vector.h"
#include "DcmtkGMC.h"
#include "UniArray.h"
#include "Transport.h"
#include "ShaderPlan.h"

struct SFractionGroup
{
  int     fractionNum;
  CString description;
  int     numFractionsPlanned;
  int     numBeams;
  int     indexFractionBeam;
};

struct SBeamInfo
{
  int beamNumber;
  CString beamName;
  int fractionIndex;
};

struct SControlParseParams;

class CIonPlan
{
public:

  CIonPlan(void);
  ~CIonPlan(void);

  void Clear(void);
  bool ParseJSON(Json::Value* root);
  bool ParseJSON(FILE* file);
  bool ParsePlanDICOM(LPCTSTR planFile);
  bool ParseBlobs(LPCTSTR blobPath);
  void SaveApertures(void);
  void SaveDevices(void);

  CString  m_studyDate;
  CString  m_studyUID;
  CString  m_seriesUID;
  CString  m_planUID;
  CString  m_seriesDescription;
  CString  m_patientName;
  CString  m_patientID;
  CString  m_planName;
  CString  m_planLabel;
  CString  m_astroidID;
  CString  m_frameReferenceUID;

  float        m_numProtonsPlan;
  unsigned int m_numFractions;

  CUniArray<SShaderBeam>         m_arrayBeams;
  CUniArray<SBeamInfo>           m_arrayMetaBeams;
  CUniArray<SShaderControlPt>    m_arrayControlPts;
  CUniArray<SShaderSpot>         m_arraySpots;
  CUniArray<SShaderDevice>       m_arrayDevices;
  CUniArray<int>                 m_arrayDeviceIndexes;
  CUniArray<SShaderAperture>     m_arrayApertures;
  CUniArray<SPt2>                m_arrayAperturePts;
  CUniArray<SShaderCompensator>  m_arrayCompensators;
  CUniArray<float>               m_arrayCompensatorDepths;
  CUniArray<SSOBPGrid>           m_arraySOBPGrids;
  CUniArray<float>               m_arrayDepthDoses;
  CUniArray<float>               m_arrayAlphas;
  CUniArray<float>               m_arrayLETs;
  CUniArray<SFractionGroup>      m_arrayFractionGroups;
  CUniArray<int>                 m_arrayFractionBeams;
  CUniArray<float>               m_arrayFractionBeamDoses;

  CString m_blobName;

  SDataGrid m_voxGridRSP;
  SDataGrid m_voxGridHU;

  float  m_rspScale;
  int    m_rspType;
  bool   m_flagProne;
  bool   m_flagSOBP;
  bool   m_flagHCL;

private:

  void ReadMetaData(Json::Value* root);
  void CalcControlPtMatrix(SControlParseParams* params);
  void ReadShort(FILE* fd, SDataGrid* grid, float scale);
  void ReadUShort(FILE* fd, SDataGrid* grid, float scale);
  bool ParseGrids(Json::Value* root);
  void CountPlanObjectsJson(Json::Value* root);
  bool CountPlanObjectsDicom(DRTIonPlanIOD& plan);
  void FillPlanJson(Json::Value* root);

  void FillPlanDicom(DRTIonPlanIOD& plan);
  unsigned int FillControlPtDicom(DRTIonControlPointSequence::Item* controlPt, SShaderControlPt* shaderPt,
    SControlParseParams* controlParams, SShaderSpot* spot, unsigned int cntrlPtIndex);

  void SaveEnergies(Json::Value* root);
  void CalcImageBounds(SDataGrid* grid);
};

