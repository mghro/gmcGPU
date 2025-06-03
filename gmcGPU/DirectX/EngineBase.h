#pragma once

#include "DirectX.h"
#include "Transport.h"
#include "AppState.h"

class CEngineBase : public CDxObj
{
public:

  CEngineBase(void);
  virtual ~CEngineBase(void);

  virtual bool Initialize(void);

  static void SetAppState(CAppState* appState);
  
  void ClearComputeUAViews(int startIndex, int endIndex);
  void ClearComputeResourceViews(int startIndex, int endIndex);

protected:

  virtual bool CreateShaders(void) = 0;
  virtual bool CreatePersistentResources(void) = 0;

  static CAppState* m_appState;
  static CFileIO*   m_fileIO;

  static CDxBuffRW*     m_buffRWProtonsAuxBase;

  // Dose texture groups
  static CTextureGroup* m_txtrGroupDosesPatient;
  static CTextureGroup* m_txtrGroupDosesQA;
  static CTextureGroup* m_txtrGroupDosesExternal;
  static CTextureGroup* m_txtrGroupGammas;
  static CTextureGroup* m_txtrGroupDosesStats;

  // Image texture groups
  static CTextureGroup* m_txtrGroupImagesPatient;
  static CTextureGroup* m_txtrGroupImagesPhantom;

  static CDxTexture3D*  m_txtrNumProtonsVox;

  static CDxTextureSampler* m_textureSampler;

  static STransportParams* m_transportParams;
  
};
