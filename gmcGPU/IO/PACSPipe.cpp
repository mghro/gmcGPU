#include "stdafx.h"
#include <iostream>
#include "process.h"
#include "Heapster.h"
#include "DcmtkGMC.h"
#include "AppConfigInstance.h"
#include "Logger.h"
#include "PACSPipe.h"

#define DICOM_SOP_RTIONPLAN "1.2.840.10008.5.1.4.1.1.481.8"

CPACSPipe::CPACSPipe(void)
{

  m_gmcBaseDirectory = g_configInstance->m_directoryBase;

  m_playerDirectory.Format("%s\\PACS\\PlaYer", m_gmcBaseDirectory);
  m_commandDirectory.Format("%s\\PACS\\Dicom", m_gmcBaseDirectory);
  m_getFilesDirectory.Format("%s\\GetFiles", m_commandDirectory);
  m_dicomCommandScript.Format("%s\\DicomCommandScript.bat", m_commandDirectory);
  m_dicomFindResultsFileName.Format("%s\\DicomFindResults.txt", m_commandDirectory);
  m_dicomFindResultsAuxFileName.Format("%s\\DicomFindResultsAux.txt", m_commandDirectory);
  m_dicomGetResultsFileName.Format("%s\\DicomGetResults.txt", m_commandDirectory);

  m_execParamsFind    = "> DicomFindResults.txt";
  m_execParamsFindAux = "> DicomFindResultsAux.txt";
  m_execParamsGet     = "> DicomGetResults.txt";
  m_execParamsGet     = "> DicomGetResults.txt";
  m_execParamsStore   = "> DicomStoreResults.txt";

  m_execInfo = { 0 };

  m_execInfo.cbSize       = sizeof(SHELLEXECUTEINFO);
  m_execInfo.fMask        = SEE_MASK_NOCLOSEPROCESS;
  m_execInfo.hwnd         = NULL;
  m_execInfo.lpVerb       = _T("open");
  m_execInfo.lpFile       = NULL;
  m_execInfo.lpParameters = NULL;
  m_execInfo.nShow        = SW_HIDE;
  m_execInfo.hInstApp     = NULL;
  m_execInfo.lpDirectory  = m_commandDirectory;
  m_execInfo.lpFile       = m_dicomCommandScript;
  m_execInfo.lpParameters = m_execParamsFind;

  return;
}

CPACSPipe::~CPACSPipe(void)
{

  return;
}

void CPACSPipe::SetMsgParamsQuery(SDicomMsgParams* paramsQuery)
{
  m_msgParamsQuery = paramsQuery;

  return;
}

void CPACSPipe::SetMsgParamsStore(SDicomMsgParams* paramsStore)
{
  m_msgParamsStore = paramsStore;

  return;
}

bool CPACSPipe::OpenPacsLog(COleDateTime* timeBoundary)
{

  FILE* fd = fopen("..\\libs.log", "r");
  ReturnOnNull(fd);

  fseek(fd, 0, SEEK_END);
  long int fileSize = ftell(fd);
  fseek(fd, 0, SEEK_SET);

  m_logInfo.fileResultsBuffer.Alloc(fileSize + 1);

  fread(m_logInfo.fileResultsBuffer(0), 1, fileSize, fd);
  fclose(fd);

  m_logInfo.fileResultsBuffer[fileSize] = 0;

  m_logInfo.filePosCurrent = m_logInfo.fileResultsBuffer(0);

  return true;
}

int CPACSPipe::ProcessPlanFileSet(LPCTSTR uidPlan)
{

  m_patientID.Empty();

  ClearDirectory(m_getFilesDirectory);

  ExecCmdGetPlan(uidPlan);

  m_arrayPlans.Alloc(1);

  m_numIonPlans = 0;

  GetInfoRetrievedPlans();

  if (m_numIonPlans)
  {
    GetPlanFileSet(0);
  }

  return m_numIonPlans;
}

/*_____________________________________________________________________________________________

  Routines to enumerate all patient RTIon plans.

    ExecCmdFindPatientSeries - Find all patient series stored in PACS.
    GetRTIonPlans - Parse find results for RTIon plans and retrieve them from PACS.
    GetInfoRetrievedPlans - Open all retrieved plans and extract plan info/descriptions
_______________________________________________________________________________________________*/

int CPACSPipe::GetInfoPatientPlans(LPCTSTR patientID)
{
  int numPlans;

  m_patientID = patientID;

  ClearDirectory(m_getFilesDirectory);

  ExecCmdFindPatientSeries(patientID);

  if ((numPlans = GetRTIonPlans()) < 1)
  {
    return numPlans;
  }

  m_arrayPlans.Alloc(numPlans);

  m_numIonPlans = 0;

  GetInfoRetrievedPlans();

  return m_numIonPlans;
}

int CPACSPipe::GetRTIonPlans(void)
{
  int numPlans = 0;

  FILE* fd = fopen(m_dicomFindResultsFileName, "r");
  ReturnOnNull(fd);

  fseek(fd, 0, SEEK_END);
  long int fileSize = ftell(fd);
  fseek(fd, 0, SEEK_SET);

  CUniArray<char> fileResultsBuffer(fileSize + 1);

  fread(fileResultsBuffer(0), 1, fileSize, fd);
  fclose(fd);

  fileResultsBuffer[fileSize] = '\0';

  char* bufferPos;
  char* modalityPos;
  char* endBracket;
  char* studyUID  = NULL;
  char* seriesUID = NULL;
  bool  planFlag  = false;

  bufferPos = fileResultsBuffer(0);

  // Loop over all series for RTPlan instances
  while ( bufferPos && ((bufferPos = strstr(bufferPos, "(0008,0060) CS")) != NULL)) // Need the CS to skip over repeat of command string
  {

    // Mark where we want to search for RTPLAN after finding study/series and terminating strings
    modalityPos = bufferPos;

    // Look for Study UID in series info
    bufferPos = strstr(bufferPos, "(0020,000d) UI");

    // The following bufferPos operations make use of the fact that MIM places null char after UID (most of the time, sometimes not)
    if (bufferPos)
    {
      // Save the start of the study UID
      bufferPos = strchr(bufferPos, '[') + 1;
      studyUID = bufferPos;

      // If the end of the study UID does not have null char then make the bracket one
      endBracket = strchr(bufferPos, ']');
      if (endBracket)
      {
        *endBracket = 0;
      }

      // Jump over the null char for further processing
      bufferPos = bufferPos + strlen(bufferPos) + 1;

      // Look for Series UID
      bufferPos = strstr(bufferPos, "(0020,000e) UI");
      if (bufferPos)
      {
        // Save the start of the series UID
        bufferPos = strchr(bufferPos, '[') + 1;
        seriesUID = bufferPos;

        // If the end of the series UID does not have null char then make the bracket one
        endBracket = strchr(bufferPos, ']');
        if (endBracket)
        {
          *endBracket = 0;
        }

        // Need to check both plan types because some PACS (MIM) don't use RTIONPLAN
        planFlag = (strstr(modalityPos, "RTPLAN") != NULL) || (strstr(modalityPos, "RTIONPLAN") != NULL);
        TRACE("Find Modality %d %s\n", numPlans, modalityPos);

        // Jump over the null char for further processing
        bufferPos = bufferPos + strlen(bufferPos) + 1;
      }

      if (planFlag)
      {
        ExecCmdGetPlan(studyUID, seriesUID);
        ++numPlans;
      }

    }
  }

  return numPlans;
}

bool CPACSPipe::GetInfoRetrievedPlans(void)
{
  WIN32_FIND_DATA fileData;
  HANDLE fileHandle;

  CString parseObject = m_getFilesDirectory;
  parseObject += "\\*";

  fileHandle = FindFirstFile(parseObject, &fileData);  // Find file "."
  FindNextFile(fileHandle, &fileData);                 // Find file ".."

  SPlanInfo* planInfo = m_arrayPlans(0);

  while (FindNextFile(fileHandle, &fileData))
  {
    planInfo->fileNameBase = fileData.cFileName;
    planInfo->fileNameFull.Format("%s\\%s", m_getFilesDirectory, fileData.cFileName);

    if (GetInfoPlan(planInfo))
    {
      ++planInfo;
    }
  }

  return true;
}

bool CPACSPipe::GetInfoPlan(SPlanInfo* planInfo)
{

  OFFilename dcmFileName(planInfo->fileNameFull);

  DcmFileFormat fileformat;
  OFCondition status = fileformat.loadFile(dcmFileName);

  if (status.bad())
  {
    return false;
  }

  OFString element;

  DRTIonPlanIOD rtPlan;
  status = rtPlan.read(*fileformat.getDataset());

  // Some site used RTPlan rather than RTIonPlan (also HCL data)
  if (status.bad())
  {
    DRTPlanIOD rtPlan;
    status = rtPlan.read(*fileformat.getDataset());

    if (status.bad())
    {
      return false;
    }
    
    rtPlan.getSOPInstanceUID(element);
    planInfo->instanceUID = element.data();

    if (m_patientID.IsEmpty())
    {
      rtPlan.getPatientID(element);
      m_patientID = element.data();
    }

    rtPlan.getSeriesDescription(element);
    planInfo->seriesDescr = element.data();

    // Used in file name so must replace bad characters
    rtPlan.getRTPlanName(element);
    planInfo->planName = element.data();
    planInfo->planName.Replace('\\', '-');
    planInfo->planName.Replace('/', '-');
    planInfo->planName.Replace(':', '-');
    planInfo->planName.Replace('&', '-');
    planInfo->planName.Replace('*', '-');
    planInfo->planName.Replace('?', '-');
    planInfo->planName.Replace(';', '-');

    rtPlan.getRTPlanDescription(element);
    planInfo->planDescr = element.data();

    OFString stringModality;
    rtPlan.getModality(stringModality);

    rtPlan.getRTPlanDate(element);
    planInfo->planDate = element.data();

    DRTReferencedStructureSetSequence& structSequence = rtPlan.getReferencedStructureSetSequence();
    structSequence.gotoFirstItem();
    DRTReferencedStructureSetSequence::Item structItem = structSequence.getCurrentItem();

    structItem.getReferencedSOPInstanceUID(element);
    planInfo->structSetUID = element.data();

    DRTReferencedDoseSequence& doseSequence = rtPlan.getReferencedDoseSequence();
    doseSequence.gotoFirstItem();
    DRTReferencedDoseSequence::Item doseItem = doseSequence.getCurrentItem();

    element.clear();
    doseItem.getReferencedSOPInstanceUID(element);
    planInfo->doseUID = element.data();

    ++m_numIonPlans;

    return true;
  }

  rtPlan.getSOPInstanceUID(element);
  planInfo->instanceUID = element.data();

  if (m_patientID.IsEmpty())
  {
    rtPlan.getPatientID(element);
    m_patientID = element.data();
  }

  rtPlan.getSeriesDescription(element);
  planInfo->seriesDescr = element.data();

  // Used in file name so must replace bad characters
  rtPlan.getRTPlanName(element);
  planInfo->planName = element.data();
  planInfo->planName.Replace('\\', '-');
  planInfo->planName.Replace('/', '-');
  planInfo->planName.Replace(':', '-');
  planInfo->planName.Replace('&', '-');
  planInfo->planName.Replace('*', '-');
  planInfo->planName.Replace('?', '-');
  planInfo->planName.Replace(';', '-');

  rtPlan.getRTPlanDescription(element);
  planInfo->planDescr = element.data();

  OFString stringModality;
  rtPlan.getModality(stringModality);

  rtPlan.getRTPlanDate(element);
  planInfo->planDate = element.data();

  DRTReferencedStructureSetSequence& structSequence = rtPlan.getReferencedStructureSetSequence();
  structSequence.gotoFirstItem();
  DRTReferencedStructureSetSequence::Item structItem = structSequence.getCurrentItem();

  structItem.getReferencedSOPInstanceUID(element);
  planInfo->structSetUID = element.data();

  DRTReferencedDoseSequence& doseSequence = rtPlan.getReferencedDoseSequence();
  doseSequence.gotoFirstItem();
  DRTReferencedDoseSequence::Item doseItem = doseSequence.getCurrentItem();

  element.clear();
  doseItem.getReferencedSOPInstanceUID(element);
  planInfo->doseUID = element.data();

  ++m_numIonPlans;

  return true;
}

bool CPACSPipe::GetPlanFileSet(int planIndex)
{
  SInstanceInfo instanceInfoStruct;
  SInstanceInfo instanceInfoDose;
  SInstanceInfo ctInfo;
  CString modality;
  SPlanInfo* planInfo;

  planInfo = m_arrayPlans(planIndex);

  // Create patient directory
  CString patientDir = m_gmcBaseDirectory + "\\Patients\\" + m_patientID;
  if (CreateDirectory(patientDir, NULL) == false && GetLastError() != ERROR_ALREADY_EXISTS)
  {
    return false;
  }

  m_directoryPlanFile = patientDir + "\\" + planInfo->instanceUID;
  if (CreateDirectory(m_directoryPlanFile, NULL) == false && GetLastError() != ERROR_ALREADY_EXISTS)
  {
    return false;
  }

  // Move selected plan file to patient directory
  CString newFile = m_directoryPlanFile + "\\" + planInfo->fileNameBase;
  MoveFile(planInfo->fileNameFull, newFile);
  planInfo->fileNameFull = newFile;

  // Remove unselected plan files
  ClearDirectory(m_getFilesDirectory);

  ExecCmdGetInstance(planInfo->structSetUID);
  ExecCmdGetInstance(planInfo->doseUID);

  CString fileName = m_getFilesDirectory + "\\RS." + planInfo->structSetUID;
  ReadCTuidsFromStructSetFile(fileName, &ctInfo);

  ExecCmdGetSeries(&ctInfo);

  CString move;
  move.Format("move \"%s\\*\" \"%s\"", m_getFilesDirectory, m_directoryPlanFile);
  system(move);

  return true;
}

bool CPACSPipe::ReadCTuidsFromStructSetFile(LPCTSTR fileName, SInstanceInfo* instance)
{
  OFFilename dcmFileName(fileName);

  DcmFileFormat fileformat;
  OFCondition status = fileformat.loadFile(dcmFileName);

  DRTStructureSetIOD rtStructSet;
  status = rtStructSet.read(*fileformat.getDataset());

  if (status.good())
  {
    OFString element;

    DRTReferencedFrameOfReferenceSequence& frameRefSequence = rtStructSet.getReferencedFrameOfReferenceSequence();
    frameRefSequence.gotoFirstItem();
    DRTReferencedFrameOfReferenceSequence::Item frameRefItem = frameRefSequence.getCurrentItem();

    DRTRTReferencedStudySequence& studySequence = frameRefItem.getRTReferencedStudySequence();
    studySequence.gotoFirstItem();
    DRTRTReferencedStudySequence::Item studyItem = studySequence.getCurrentItem();

    studyItem.getReferencedSOPInstanceUID(element);
    instance->studyUID = element.data();

    DRTRTReferencedSeriesSequence& seriesSequence = studyItem.getRTReferencedSeriesSequence();
    seriesSequence.gotoFirstItem();
    DRTRTReferencedSeriesSequence::Item seriesItem = seriesSequence.getCurrentItem();

    seriesItem.getSeriesInstanceUID(element);
    instance->seriesUID = element.data();

  }

  return true;
}


bool CPACSPipe::ConvertStudyDicom2Json(int planIndex)
{

  ConvertStudyDicom2Json(m_directoryPlanFile, m_arrayPlans(planIndex)->fileNameBase);

  return true;
}

bool CPACSPipe::ConvertStudyDicom2Json(LPCTSTR pathName, LPCTSTR fileNamePlan)
{

  CString playerCmd;

  // Build PlaYer command
  playerCmd.Format("py \"%s\\player.py\" -i GMC \"%s\" %s", m_playerDirectory, pathName, fileNamePlan);

  CString params;
  params.Format("> \"%s\\PlaYerScriptResults.txt\"", m_playerDirectory);

  CString playerScript = m_playerDirectory + "\\RunPlayerGMC.bat";
  FILE* fd = fopen(playerScript, "w");
  ReturnOnNull(fd);


  fprintf(fd, "%s", (LPCTSTR)playerCmd);
  fclose(fd);

  SHELLEXECUTEINFO playerExecInfo = { 0 };

  playerExecInfo.cbSize       = sizeof(SHELLEXECUTEINFO);
  playerExecInfo.fMask        = SEE_MASK_NOCLOSEPROCESS;
  playerExecInfo.hwnd         = NULL;
  playerExecInfo.lpVerb       = _T("open");
  playerExecInfo.lpFile       = NULL;
  playerExecInfo.lpParameters = NULL;
  playerExecInfo.nShow        = SW_HIDE;
  playerExecInfo.hInstApp     = NULL;
  playerExecInfo.lpDirectory  = m_playerDirectory;
  playerExecInfo.lpFile       = playerScript;
  playerExecInfo.lpParameters = params;

  // Execute PlaYer
  ShellExecuteEx(&playerExecInfo);
  WaitForSingleObject(playerExecInfo.hProcess, INFINITE);

  return true;
}

void CPACSPipe::ClearDirectory(CString path)
{
  CFileFind ff;

  path += "\\*";

	BOOL result = ff.FindFile(path);

  result = ff.FindNextFile(); // \\.
  result = ff.FindNextFile(); // \\..

	while(result)
	{
		result = ff.FindNextFile();
    if (!ff.IsDots() && !ff.IsDirectory())
    {
      DeleteFile(ff.GetFilePath());
    }
		else if (ff.IsDirectory())
		{
			path = ff.GetFilePath();
			ClearDirectory(path);
			RemoveDirectory(path);
		}
	}

  return;
}

bool CPACSPipe::ExecCmdFindPatientSeries(LPCTSTR patientID)
{
  CString dicomCmnd;

  // Build Find patient series query and store in command file
  dicomCmnd.Format("findscu -S -aet %s -aec %s -k QueryRetrieveLevel=SERIES -k (0008,0060) -k (0020,000D) -k (0020,000E) -k PatientID=%s +sr %s %s",
    m_msgParamsQuery->aeNameGMC, m_msgParamsQuery->aeNameServer, patientID, m_msgParamsQuery->serverIPAddress, m_msgParamsQuery->serverPortNum);
  m_execInfo.lpParameters = m_execParamsFind;

  ReturnOnFalse(ExecuteDicomCommand(dicomCmnd));

  return true;
}

bool CPACSPipe::ExecCmdGetInstance(CString uidInstance)
{
  CString dicomCmnd;

  // Build  Get instance command
  dicomCmnd.Format("getscu -B +xi -S -aet %s -aec %s -k QueryRetrieveLevel=IMAGE -k (0008,0018)=%s -od %s %s %s",
    m_msgParamsQuery->aeNameGMC, m_msgParamsQuery->aeNameServer, uidInstance, m_getFilesDirectory, m_msgParamsQuery->serverIPAddress, m_msgParamsQuery->serverPortNum);
  m_execInfo.lpParameters = m_execParamsGet;

  ReturnOnFalse(ExecuteDicomCommand(dicomCmnd));

  return true;
}

bool CPACSPipe::ExecCmdGetPlan(LPCTSTR studyUID, LPCTSTR seriesUID)
{
  CString dicomCmnd;

  // Build Get series command
  dicomCmnd.Format("getscu -S +xi -B -aet %s -aec %s -k QueryRetrieveLevel=SERIES -k (0020,000d)=%s -k (0020,000e)=%s -od %s %s %s",
    m_msgParamsQuery->aeNameGMC, m_msgParamsQuery->aeNameServer,  studyUID, seriesUID, m_getFilesDirectory, 
    m_msgParamsQuery->serverIPAddress, m_msgParamsQuery->serverPortNum);
  m_execInfo.lpParameters = m_execParamsGet;

  ReturnOnFalse(ExecuteDicomCommand(dicomCmnd));

  return true;
}

bool CPACSPipe::ExecCmdGetPlan(LPCTSTR instanceUID)
{
  CString dicomCmnd;

  // Build Get instance command
  dicomCmnd.Format("getscu -S +xi -B -aet %s -aec %s -k QueryRetrieveLevel=IMAGE -k (0008,0018)=%s -od %s %s %s",
    m_msgParamsQuery->aeNameGMC, m_msgParamsQuery->aeNameServer, instanceUID, m_getFilesDirectory, m_msgParamsQuery->serverIPAddress, m_msgParamsQuery->serverPortNum);
  m_execInfo.lpParameters = m_execParamsGet;

  ReturnOnFalse(ExecuteDicomCommand(dicomCmnd));

  return true;
}

bool CPACSPipe::ExecCmdGetSeries(SInstanceInfo* instance)
{
  CString dicomCmnd;

  // Build Get series command
  dicomCmnd.Format("getscu -B +xi -S -aet %s -aec %s -k QueryRetrieveLevel=SERIES -k (0020,000d)=%s -k (0020,000e)=%s -od %s %s %s",
    m_msgParamsQuery->aeNameGMC, m_msgParamsQuery->aeNameServer, instance->studyUID, instance->seriesUID, m_getFilesDirectory, 
    m_msgParamsQuery->serverIPAddress, m_msgParamsQuery->serverPortNum);
  m_execInfo.lpParameters = m_execParamsGet;

  ReturnOnFalse(ExecuteDicomCommand(dicomCmnd));

  return true;
}

bool CPACSPipe::StoreFile(LPCTSTR fileName)
{
  CString dicomCmnd;

  dicomCmnd.Format("storescu -v -aet %s -aec %s %s %s \"%s\"",
    g_configInstance->m_directoryBase, m_msgParamsStore->aeNameGMC, m_msgParamsStore->aeNameServer, m_msgParamsStore->serverIPAddress, m_msgParamsStore->serverPortNum, fileName);
  m_execInfo.lpParameters = m_execParamsStore;

  ReturnOnFalse(ExecuteDicomCommand(dicomCmnd));

  return true;
}

bool CPACSPipe::ExecuteDicomCommand(LPCTSTR dicomCommand)
{
  FILE* fd = fopen(m_dicomCommandScript, "w");
  ReturnOnNull(fd);

  fprintf(fd, "%s", dicomCommand);
  fclose(fd);

  // Query MIM for all studies associated with patientID
  ShellExecuteEx(&m_execInfo);
  WaitForSingleObject(m_execInfo.hProcess, INFINITE);

  return true;
}


