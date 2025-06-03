#pragma once

#include "Vector.h"
#include "UniArray.h"

using namespace DirectX;

typedef struct STileVertex
{
  DirectX::XMFLOAT3 imagePos;
  DirectX::XMFLOAT3 volPos;
  DirectX::XMFLOAT2 texturePos;

} STileVertex;

typedef struct STileInfoVS
{
  DirectX::XMMATRIX worldMatrix;

  float zoomFactor    = 1.0;
  float slicePosFract = 0.0;
  int   viewAxis      = 0;

} STileInfoVS;

typedef struct SDisplayTile
{

  CRect tileRect;

  float tileScaleHorz;
  float tileScaleVert;

  DirectX::XMMATRIX scaleMatrix;
  DirectX::XMMATRIX transMatrix;

  STileInfoVS vsTileInfo;
  float slicePosAbs;

  int vertexStartIndex;

  // Rectangles where image info overlayed on image
  D2D_RECT_F imageNumTextRect;
  D2D_RECT_F orientTextRect[5];

  D3D11_VIEWPORT viewPort;

} SDisplayTile;

class CTileConfig
{
public:

  CTileConfig(void);
  ~CTileConfig(void);

  void CalcTileGeometry(int numTilesHorz, int numTilesVert, int numViewerPixHorz, int numViewerPixVert, int* axisInit);
  void SetImageGeometry(int* dims, float* voxelSizes);

  void SetPrimaryView(int viewAxis, bool synchStatus);
  void SetSecondaryView(int viewAxis, bool synchStatus);
  void SetImagePos(float pos, bool synchStatus);
  void SetZoomFactor(float zoom);
  void SetImageTranslation(CPoint* translation);
  inline int GetPrimaryView(void) { return m_arrayTiles[m_scrollTileIndex].vsTileInfo.viewAxis; }

  inline int GetNumTiles(void) { return m_arrayTiles.m_numItems; }
  inline float GetScrollTileSlicePos(void);
  SDisplayTile* FindTile(CPoint* point);

  SDisplayTile* ConvertPoint(CPoint* point, CPoint* panOffset, SPt2* ptInfo);
  void CTileConfig::ConvertPtScreenPixToShader(CPoint* pointPix, SPt2* pointShader);

  void CalcSlicePositions(float posFraction);


  int m_numTilesHorz;
  int m_numTilesVert;

  int m_numTilesPrimary;

  float m_numPixHorzTile;
  float m_numPixVertTile;

  SDisplayTile* m_tilePicked;

  CUniArray<SDisplayTile>  m_arrayTiles;

  float m_baseScaleHorz[3];
  float m_baseScaleVert[3];
  float m_numScreenPixImgHorz[3];
  float m_numScreenPixImgVert[3];
  float m_scaleAbsHorz[3];
  float m_scaleAbsVert[3];

  int m_scrollTileIndex;

private:

  float m_zoomFactor;
  SPt2  m_panOffsetScreenPix;

  void UpdateViewAxis(SDisplayTile* tile, int viewAxis);

};
