#pragma once

class CIonPlan;

class CUtils
{
public:

  CUtils(void);
  ~CUtils(void);

  void ExportDepthInfo(CIonPlan* plan, int beamIndex, int controlPtIndex);
  void SpreadProtons(int numProtons, SPt2* spotSize);
  void CalcScatterAngle(float* depthDose, int numDose);
  void ArrayTest(void);
  //void CalcMaxMin(ID3D11Texture3D* inputTexture, ID3D11Texture3D* readTexture);
  //template <typename T> FindBounds(ID3D11Texture3D* inputTexture, ID3D11Texture3D* readTexture);

private:

  float Interpolate(float* data, float *sample);

  float m_param[16];
};