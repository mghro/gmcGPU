#pragma once
#include "DirectX.h"
#include "EngineBase.h"
#include "GammaIO.h"

class  CFileIO;
struct SGammaThresholds;

enum
{
  SAVE_TYPE_PATIENT,
  SAVE_TYPE_QA,
  SAVE_TYPE_GAMMA
};

struct SEnergies
{
  double energyPlanProtons;
  double energyCompTotal;
  double energyCompPatient;
  double energyCompDevices;
  double energyExternalPatient;
};

class CEnginePostCalc : public CEngineBase
{
public:

  CEnginePostCalc(void);
  ~CEnginePostCalc(void);

  void ClearResources(void);

  void  LoadTransportInfo(void);
  float LoadExternalDoseDicom(LPCTSTR fileName);
  float LoadExternalDoseMSH(FILE* fd, SVoxGridGeometry* dataGrid);
  bool  LoadStructsDicom(LPCTSTR fileName);

  void CalcAuxFields(int numFractions);
  void CalcDoseProfiles(void);
  void CalcDoseProfile(int doseType, float numProtons);
  void CalcDoseHistograms(void);
  void SumBeamDoses(CIonPlan* ionPlan);
  float SumExternalDose(CIonPlan* ionPlan);
  void CalcDoseHistogram(int doseType, FILE* fd, int flag);
  void CalcEnergy(SEnergies* energies);
  SGammaResults* CalcGamma(SGammaThresholds* gammaThresholds);
  void CopyTextureData(CDxTexture3D* inputTexture, float* data);

  void SaveDicom(LPCTSTR fileName, LPCTSTR seriesDescr, CIonPlan* ionPlan, int saveType, int indexDose);
  void SaveQABeamDose(FILE* fd, SVoxGridGeometry* subGrid, int qaIndex);
  void SaveQABeamGeometry(FILE* fd, int* baseGridDims, float* isocenterQA, int* doseGridDims);
  void SaveDoseRaw2(FILE* fd, CDxTexture3D* doseTexture, SVoxGridGeometry* subGrid, int* dims);
  void SaveDoseRaw(FILE* fd, CDxTexture3D* doseTexture, SVoxGridGeometry* baseGrid, SVoxGridGeometry* subGrid);

  double CalcEnergyDevices(void);

private:

  // One time initialization routines
  bool CreateShaders(void);
  bool CreatePersistentResources(void);

  void CalcTextureBounds(CDxTexture3D* inputTexture);
  void CalcTextureBounds2(ID3D11ShaderResourceView* textureview);

  double CalcEnergyPatient(void);
  double CalcEnergyPatientMulti(SVoxGridGeometry* gridDose, CDxTexture3D* doseTexture);

  void SaveDoseDicom(LPCTSTR fileName, LPCTSTR seriesDescr, CIonPlan* ionPlan, SVoxGridGeometry* grid, CDxTexture3D* doseTexture, int indexBeam);

  CDxShaderCompute m_boundsCalcTxtrCS;
  CDxShaderCompute m_arrayMaxCS;
  CDxShaderCompute m_energyCS;
  CDxShaderCompute m_energyDevicesCS;
  CDxShaderCompute m_energyPatientFreeCS;
  CDxShaderCompute m_doseSumCS;

  CDxBuffConstant  m_bufferDispatch;
  CDxBuffConstant  m_dxBuffDoseSumParams;

  CDxBuffRW m_bufferThreads;
  CDxTexture3D m_dxBuffDoseSum;

  SDataGrid m_dataGridExtrnl;

  SGammaResults m_gammaResults;

};

