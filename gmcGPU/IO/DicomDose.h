#pragma once

struct SVoxGridGeometry;
struct SDataGrid;

class DRTIonBeamSequence;
class CIonPlan;

struct SDicomInfo
{
  CString idPatient;
  CString uidStudy;
  CString uidSeries;
  CString uidInstance;
};

class CDicomDose
{
public:

  CDicomDose(void);
  ~CDicomDose(void);

  void ParseDoseFile(LPCTSTR doseFile, SDataGrid* grid);
  void BuildDoseFile(CIonPlan* ionPlan, unsigned short* doseData, SVoxGridGeometry* doseGrid, 
    LPCTSTR fileName, LPCTSTR seriesDescr, float storeScale, int beamIndex);

  bool ReadPlanBasic(LPCTSTR fileNamePlan, SDicomInfo* info);

  void SumBeamDoses(CIonPlan* ionPlan, CString beamFolder);
  void SumDose(SDataGrid* gridSum, SDataGrid* gridBeam, float scaleFactor);
  void ConvertDose(unsigned short* doseShort, float* doseFloat, int numVoxels, float scaleFactor);

};