#pragma once

#include "DirectXMath.h"

using namespace DirectX;

#include "UniArray.h"
#include "DirectX.h"


#define TRANSPORT_FLAG_SPOT_SIZE_ZERO        (0x01)
#define TRANSPORT_FLAG_SPOT_PROTONS_CONSTANT (0x02)
#define NUM_RBE_DOSE_MAX                     (3)

struct SVoxGridGeometry
{
  int   dims[3];
  int   numVoxels = 0;
  float voxelSize[3];
  float voxelOrigin[3];
  float voxelVolume;
  float unitVecs[3][3] = { { 1, 0, 0 }, { 0, 1, 0 }, { 0, 0, 1 } };
};

struct SDataGrid : SVoxGridGeometry
{
  float bounds[2] = {};
  float* data     = NULL;
};

struct STransportParams
{
  unsigned int numCycles           = 0;
  unsigned int protonScaleFactor   = 0;
  unsigned int numProtonsTransport = 0;
  bool spotProtonsConstant         = false;
  float energyPlan                 = 0.0;

  float rbeVals[3] = { 2.0, 3.0, 10.0 };

  // Quantities to calc
  bool dosePrimary = false;
  bool doseAux     = false;
  bool doseQA      = false;
  bool dijCalc     = false;
  bool statsCalc   = false;

  // Run params
  bool autoMode    = false;
  bool pacsSave    = false;

  CString pathNamePlan;
  CString fileNamePlan;
  CString fileNameDose;
};

struct SDijParams
{
  unsigned int numBins;
  unsigned int numVoxels;
};

// CTextureGroup is an array of related textures that share a common grid geometry
class CTextureGroup : public CUniArray<CDxTexture3D>
{
public:

  CTextureGroup(void);

  void CalcFieldTransform(SVoxGridGeometry* baseGrid);

  SVoxGridGeometry m_volumeGrid;
  XMFLOAT4X4       m_fieldTransform;

  inline ID3D11Texture3D* GetTexture(int txtrIndex) { return ItemPtr(txtrIndex)->m_texture; }
  inline ID3D11ShaderResourceView* GetTextureView(int txtrIndex) { return ItemPtr(txtrIndex)->m_view; }
  inline ID3D11UnorderedAccessView* GetTextureViewRW(int txtrIndex) { return ItemPtr(txtrIndex)->m_viewRW; }
  inline ID3D11Texture3D* GetTextureRead(int txtrIndex) { return ItemPtr(txtrIndex)->m_textureRead; }

  inline SVoxGridGeometry* GetGrid(void) { return &m_volumeGrid; }

  inline void ReleaseTextures(void)
  {
    forPrray(this, txtr)
    {
      txtr->Release();
    }
  }

  void ClearAll(void)
  {
    ReleaseTextures();
    Clear();
  }

};

enum
{
  IMG_TYPE_HU = 0,
  IMG_TYPE_RSP,

  NUM_IMG_TYPES
};

enum
{
  DOSE_TYPE_DOSE = 0,
  DOSE_TYPE_LET,
  DOSE_TYPE_BIODOSE,
  DOSE_TYPE_DIVLET,
  DOSE_TYPE_RBE0,
  DOSE_TYPE_RBE1,
  DOSE_TYPE_RBE2,

  NUM_DOSE_TYPES
};

#define DOSE_GAMMA NUM_DOSE_TYPES

enum
{
  MTRL_COPPER = 0,
  MTRL_LUCITE,
  MTRL_WATER,
  MTRL_AIR,
  MTRL_KRYPTO,

  NUM_MATERIALS
};
