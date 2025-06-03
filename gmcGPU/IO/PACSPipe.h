#pragma once

#include <afxmt.h>

#include "UniArray.h"
#include "DicomMsgParams.h"

enum
{
  TM_QUERY_START = 0x100,
  TM_QUERY_COMPLETED
};

struct SInstanceInfo
{
  CString studyUID;
  CString seriesUID;
  CString instanceUID;
};

struct SPlanInfo
{
  CString fileNameBase;
  CString fileNameFull;
  CString instanceUID;
  CString studyUID;
  CString seriesUID;
  CString seriesDescr;
  CString planName;
  CString planDescr;
  CString planDate;
  CString structSetUID;
  CString doseUID;
};

struct SLogInfo
{
  CUniArray<char> fileResultsBuffer;
  char* filePosCurrent;
  char* filePosPlan;
  COleDateTime* timeBoundary;
};

class CPACSPipe
{
public:

  CPACSPipe(void);
  ~CPACSPipe(void);

  void SetMsgParamsQuery(SDicomMsgParams* paramsQuery);
  void SetMsgParamsStore(SDicomMsgParams* paramsStore);

  bool OpenPacsLog(COleDateTime* timeBoundary);
  int  ProcessPlanFileSet(LPCTSTR planUid);
  int  GetInfoPatientPlans(LPCTSTR patientID);
  bool GetInfoRetrievedPlans(void);
  bool GetInfoPlan(SPlanInfo* planInfo);
  bool GetPlanFileSet(int planIndex);
  bool ConvertStudyDicom2Json(int planIndex);
  bool ConvertStudyDicom2Json(LPCTSTR pathName, LPCTSTR fileNamePlan);

  bool StoreFile(LPCTSTR fileName);
  
  inline CUniArray<SPlanInfo>* GetPlans(void)
  {
    return &m_arrayPlans;
  }

  inline SPlanInfo* GetPlan(int indexPlan)
  {
    return m_arrayPlans(indexPlan);
  }

  inline LPCTSTR GetPlanDirectory(void)
  {
    return m_directoryPlanFile;
  }

  bool ExecCmdGetPlan(LPCTSTR instanceUID);

private:

  bool ExecCmdFindPatientSeries(LPCTSTR patientID);
  bool ExecCmdGetPlan(LPCTSTR studyUID, LPCTSTR seriesUID);
  bool ExecCmdGetInstance(CString uidInstance);
  bool ExecCmdGetSeries(SInstanceInfo* instance);
  bool ExecuteDicomCommand(LPCTSTR dicomCommand);


  int GetRTIonPlans(void);
  bool ReadCTuidsFromStructSetFile(LPCTSTR fileName, SInstanceInfo* instance);

  void ClearDirectory(CString path);

  CUniArray<SPlanInfo> m_arrayPlans;
  int m_numIonPlans;

  SHELLEXECUTEINFO m_execInfo;

  SDicomMsgParams* m_msgParamsQuery;
  SDicomMsgParams* m_msgParamsStore;

  CString m_gmcBaseDirectory;
  CString m_commandDirectory;
  CString m_playerDirectory;
  CString m_getFilesDirectory;
  CString m_directoryPlanFile;
  CString m_dicomCommandScript;
  CString m_dicomFindResultsFileName;
  CString m_dicomFindResultsAuxFileName;
  CString m_dicomGetResultsFileName;
  CString m_astroidMatchFileName;
  CString m_execParamsFind;
  CString m_execParamsFindAux;
  CString m_execParamsGet;
  CString m_execParamsStore;
  CString m_astroidID;

  CString m_patientID;

  SLogInfo m_logInfo;

};
