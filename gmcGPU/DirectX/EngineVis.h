#pragma once

#include "UniArray.h"
#include "DirectX.h"
#include "Vector.h"
#include "EngineBase.h"

class CTileConfig;

enum
{
  DISPLAY_BOUNDS_PRIM = 0,
  DISPLAY_BOUNDS_SEC,
  DISPLAY_BOUNDS_INDPNDNT,

  NUM_DISPLAY_BOUNDS
};

#define NUM_CUBE_FACES          (6)
#define NUM_IMAGE_DIR_LABELS    (4)
#define MAX_NUM_COLORS          (1024)

using namespace DirectX;

class  CDxTexture3D;
class  CPhantomQA;
class  CEngineBase;
struct STransportResults;
struct SVoxGridGeometry;
struct SDisplayTile;

struct SColor4
{
  float red;
  float green;
  float blue;
  float alpha; // Not used but needed for shader struct spacing
};

struct SColorMap
{
  SColor4 colors[MAX_NUM_COLORS];
  int numColors;
  int pad[3];
};

struct SFieldLevels
{

  float imageMin;
  float imageMax;
  float imageWidth;

  float percentage;

  float pad[4];
};

struct SDisplayParamsDosePS
{
  XMFLOAT4X4 fieldTransform;

  float doseMinDisplay;
  float doseMaxDisplay;

  int   pad[6];
};

struct SVisInfoPS
{
  CTextureGroup* arrayTxtureImages = NULL;

  float doseMinData;
  float doseMaxData;

  SDisplayParamsDosePS      displayParams = {};
  ID3D11ShaderResourceView* shaderViews[2] = { NULL, NULL };
};

struct SContourVertex
{
  XMFLOAT3 position;
  XMFLOAT4 color;
};

struct SMeasureInfo
{
  SPt2 origin;
  SPt2 corner;
  int   type;
  SDisplayTile* drawTile;
};

class CEngineVis : public CEngineBase
{
public:

  CEngineVis(void);
  ~CEngineVis(void);

  bool Initialize(void);

  void ClearResources(void);

  bool CreateColorMapBuffer(SColorMap* colorMap);

  void LoadTransportInfo(void);
  void LoadExternalDoseDicom(void);
  void LoadExternalDoseMSH(void);
  void LoadGamma(void);
  void LoadDoseSum(void);

  // Respond to User Actions
  void  SetResultsType(int slot, int indexResult);
  void  SetWindowLevels(float min, float max);
  void  SetDoseBoundsUser(float min, float max);
  void  SetImageType(int type);
  void  SetDisplayBoundsType(int type);
  void  SetTileConfig(CTileConfig* tileConfig);

  void  CalcSlicePositions(float posFraction);
  SDisplayTile* FindPointTile(CPoint* point);
  inline void SetMeasureInfo(SMeasureInfo* measureInfo) { m_infoMeasure = measureInfo; }

  void  Render(void);
  void  Render2(void);
  void  ClearDisplay(void);
  
  void  PrepDoseDiff(void);
  bool  DisplayDoseDiff(void);

  void  PrepPointDose(void);
  void  CalcPointDose(SPt3* dosePt, SPt2* imagePt, SDisplayTile* tile, float* results);

  inline float GetMaxDosePrimary(void) { return (m_visInfoPrim->arrayTxtureImages ? m_visInfoPrim->doseMaxData : 0.0F); }
  inline float GetMaxDoseSecondary(void) { return (m_visInfoSec->arrayTxtureImages ? m_visInfoSec->doseMaxData : 0.0F);  }
  
private:

  // One time initialization routines
  bool CreateShaders(void);
  bool CreatePersistentResources(void);

  bool InitializeVis2D(void);
  bool CreateShaderBuffers(void);
  bool CreateShadersDoseDisplay(void);
  bool CreateShadersMeasure(void);
  bool CreateTileVertexBuffer(void);

  void BuildPairList(void);

  void PrepDoseRender(void);
  void DrawTileBoundaries(void);
  void DrawSurfaceOverlays(void);
  void DrawMeasurement(void);
  void DrawPts(void);

  // Vertex shader resources
  CDxShaderVertex    m_vertexShaderDose;
  ID3D11InputLayout* m_layoutDose;
  CDxBuffConstant    m_tileVertexBuffer;
  CDxBuffConstant    m_vertextInfoCBuffer;

  // Pixel shader resources
  CDxShaderPixel      m_pixelShaderDose;
  CDxShaderPixel      m_psDoseDiff;
  CDxShaderPixel      m_psDoseLevels;
  CDxBuffConstant     m_colorScaleCBuffer;
  CDxBuffConstant     m_fieldLevelCBuffer;

  // Contour resources
  ID3D11VertexShader* m_vShaderUserLine;
  ID3D11PixelShader*  m_pShaderUserLine;
  ID3D11InputLayout*  m_layoutContour;

  // 2D drawing resources
  ID2D1SolidColorBrush* m_brushWhite;
  IDWriteTextFormat*    m_textFormat; 

  // Screeen and tile state info
  CTileConfig* m_tileConfig;
  CUniArray<SDisplayTile>* m_arrayTiles;
  int m_numViewerPixHorz;
  int m_numViewerPixVert;

  int m_imageType;

  //CUniArray<SDisplayPair>  m_arrayDisplayPair;

  CUniArray<SVisInfoPS>  m_arrayVisInfo;

  SVisInfoPS* m_visInfoPrim;
  SVisInfoPS* m_visInfoSec;
  CDxBuffConstant m_psParamsBufferPrim;
  CDxBuffConstant m_psParamsBufferSec;

  SDataGrid m_spotDoseGrid;

  SFieldLevels m_fieldLevels;

  float m_colorBlack[4] = { 0.0, 0.0, 0.0, 1.0 };

  WCHAR** m_imageLabelsCurrent;
  WCHAR* m_imageLabelsSupineHF[NUM_CUBE_FACES][NUM_IMAGE_DIR_LABELS] =
  {
    { L"A", L"S", L"P", L"I" }, // IMG_ORNTN_POS_X_L
    { L"L", L"S", L"R", L"I" }, // IMG_ORNTN_POS_Y_P
    { L"L", L"A", L"R", L"P" }, // IMG_ORNTN_POS_Z_S
  };

  WCHAR* m_imageLabelsProneHF[NUM_CUBE_FACES][NUM_IMAGE_DIR_LABELS] =
  {
    { L"P", L"S", L"A", L"I" }, // IMG_ORNTN_NEG_X_R
    { L"R", L"S", L"L", L"I" }, // IMG_ORNTN_NEG_Y_A
    { L"L", L"A", L"R", L"P" }, // IMG_ORNTN_POS_Z_S
  };


  bool m_doseDiffStatus;
  int  m_displayBoundsType;

  // Point Dose Resources
  CDxShaderCompute m_ptDoseCS;
  CDxBuffConstant  m_ptDoseParamsCBuff;
  CDxBuffRW        m_ptDoseResultsBuff;

  CDxBuffConstant m_dispatchCBuffer;

  SMeasureInfo* m_infoMeasure;
  SPt2*         m_infoPtDose;
  float         m_posSlices[3];

  // Measurement resources
  CDxBuffConstant m_vertexBuffMeasure;

};