#include "stdafx.h"
#include "DirectX.h"
#include "Vector.h"
#include "UniArray.h"
#include "TileConfig.h"

CTileConfig::CTileConfig(void) :
  m_panOffsetScreenPix   ({}),
  m_numTilesHorz         (0),
  m_numTilesVert         (0),
  m_scrollTileIndex      (0),
  m_numPixHorzTile       (0),
  m_zoomFactor           (1.0)
{

  return;
}

CTileConfig::~CTileConfig(void)
{
  m_arrayTiles.Clear();

  return;
}

void CTileConfig::CalcTileGeometry(int numTilesHorz, int numTilesVert, int numViewerPixHorz, int numViewerPixVert, int* axisInit)
{
  float vertPosPixels;
  float horzPosPixels;
  int   vertexIndex;

  m_numTilesHorz = numTilesHorz;
  m_numTilesVert = numTilesVert;

  m_arrayTiles.Alloc(m_numTilesHorz * m_numTilesVert);

  // Tile size for this tile configuration
  m_numPixHorzTile = (numViewerPixHorz - (m_numTilesHorz - 1)) / (float)m_numTilesHorz;
  m_numPixVertTile = (numViewerPixVert - (m_numTilesVert - 1)) / (float)m_numTilesVert;


  vertexIndex = 0;
  vertPosPixels = 0.0;

  SDisplayTile* tile = m_arrayTiles(0);
  int* axis = axisInit;
  for (int i = 0; i < m_numTilesVert; ++i)
  {
    horzPosPixels = 0.0;

    for (int j = 0; j < m_numTilesHorz; ++j)
    {
      tile->vsTileInfo.viewAxis = *axis;
      ++axis;

      D3D11_VIEWPORT* vp = &tile->viewPort;

      vp->Width    = (FLOAT)m_numPixHorzTile;
      vp->Height   = (FLOAT)m_numPixVertTile;
      vp->MinDepth = 0.0f;
      vp->MaxDepth = 1.0f;
      vp->TopLeftX = horzPosPixels;
      vp->TopLeftY = vertPosPixels;

      // Store the tile pixel geometry with respect to the
      tile->tileRect.SetRect(horzPosPixels, vertPosPixels, horzPosPixels + m_numPixHorzTile, vertPosPixels + m_numPixVertTile);

      // Store the first index into the total vertex array for this tile
      tile->vertexStartIndex = vertexIndex;

      // Set tile image label positions
      tile->imageNumTextRect.left   = horzPosPixels + 2;
      tile->imageNumTextRect.right  = tile->imageNumTextRect.left + 50;
      tile->imageNumTextRect.top    = vertPosPixels + 2;
      tile->imageNumTextRect.bottom = tile->imageNumTextRect.top + 20;

      tile->orientTextRect[1].left   = horzPosPixels + m_numPixHorzTile / 2 - 2;
      tile->orientTextRect[1].right  = tile->orientTextRect[1].left + 10;
      tile->orientTextRect[1].top    = vertPosPixels + 4;
      tile->orientTextRect[1].bottom = tile->orientTextRect[1].top + 10;

      tile->orientTextRect[3].left   = horzPosPixels + m_numPixHorzTile / 2 - 2;
      tile->orientTextRect[3].right  = tile->orientTextRect[3].left + 10;
      tile->orientTextRect[3].top    = vertPosPixels + m_numPixVertTile - 24;
      tile->orientTextRect[3].bottom = tile->orientTextRect[3].top + 10;

      tile->orientTextRect[2].left   = horzPosPixels + 4;
      tile->orientTextRect[2].right  = tile->orientTextRect[2].left + 10;
      tile->orientTextRect[2].top    = vertPosPixels + m_numPixVertTile / 2 - 8;
      tile->orientTextRect[2].bottom = tile->orientTextRect[2].top + 10;

      tile->orientTextRect[0].left   = horzPosPixels + m_numPixHorzTile - 16;
      tile->orientTextRect[0].right  = tile->orientTextRect[0].left + 10;
      tile->orientTextRect[0].top    = vertPosPixels + m_numPixVertTile / 2 - 8;
      tile->orientTextRect[0].bottom = tile->orientTextRect[0].top + 10;

      tile->orientTextRect[4].left   = horzPosPixels + 4;
      tile->orientTextRect[4].right  = horzPosPixels + 96;
      tile->orientTextRect[4].top    = vertPosPixels + 4;
      tile->orientTextRect[4].bottom = vertPosPixels + 16;

      horzPosPixels += (m_numPixHorzTile + 1);
      vertexIndex += 4; // 4 vertices per rectancular tile

      ++tile;
    }

    vertPosPixels += (m_numPixVertTile + 1); // +1 accounts for line drawn between tiles
  }

  return;
}

void CTileConfig::SetImageGeometry(int* dims, float* voxelSizes)
{
  float numScreenPixImg[3];

  // Use x voxelSize as unit of measure
  numScreenPixImg[0] = dims[0];
  numScreenPixImg[1] = dims[1] * voxelSizes[1] / voxelSizes[0];
  numScreenPixImg[2] = dims[2] * voxelSizes[2] / voxelSizes[0];

  float tileWidth  = (float)m_arrayTiles(0)->tileRect.Width();
  float tileHeight = (float)m_arrayTiles(0)->tileRect.Height();

  // Sagital (x) horz (y) vert (z)
  m_numScreenPixImgHorz[0] = numScreenPixImg[1];               // ptdose
  m_numScreenPixImgVert[0] = numScreenPixImg[2];
  m_baseScaleHorz[0]       = numScreenPixImg[1] / tileWidth;   // scalematrix
  m_baseScaleVert[0]       = numScreenPixImg[2] / tileHeight;
  m_scaleAbsHorz[0]        = dims[1] * voxelSizes[1] / 2;      // distance calc
  m_scaleAbsVert[0]        = dims[2] * voxelSizes[2] / 2;

  // Coronal (y) horz (x) vert (z)
  m_numScreenPixImgHorz[1] = numScreenPixImg[0];
  m_numScreenPixImgVert[1] = numScreenPixImg[2];
  m_baseScaleHorz[1]       = numScreenPixImg[0] / tileWidth;
  m_baseScaleVert[1]       = numScreenPixImg[2] / tileHeight;
  m_scaleAbsHorz[1]        = dims[0] * voxelSizes[0] / 2;
  m_scaleAbsVert[1]        = dims[2] * voxelSizes[2] / 2;

  // Axial (z) horz (x) vert (y)
  m_numScreenPixImgHorz[2] = numScreenPixImg[0];
  m_numScreenPixImgVert[2] = numScreenPixImg[1];
  m_baseScaleHorz[2]       = numScreenPixImg[0] / tileWidth;
  m_baseScaleVert[2]       = numScreenPixImg[1] / tileHeight;
  m_scaleAbsHorz[2]        = dims[0] * voxelSizes[0] / 2;
  m_scaleAbsVert[2]        = dims[1] * voxelSizes[1] / 2;

  forArray(m_arrayTiles, tile)
  {
    tile->tileScaleHorz = m_baseScaleHorz[tile->vsTileInfo.viewAxis];
    tile->tileScaleVert = m_baseScaleVert[tile->vsTileInfo.viewAxis];

    tile->scaleMatrix = XMMatrixScaling(tile->tileScaleHorz, tile->tileScaleVert, 1.0);
    tile->transMatrix = XMMatrixIdentity();

    tile->vsTileInfo.worldMatrix   = tile->scaleMatrix;
    tile->vsTileInfo.slicePosFract = 0.0;
  }

  return;
}

void CTileConfig::SetPrimaryView(int viewAxis, bool synchStatus)
{
  if (m_arrayTiles.m_numItems <= 2)
  {
    UpdateViewAxis(m_arrayTiles(0), viewAxis);

    if (m_arrayTiles.m_numItems == 2)
    {
      UpdateViewAxis(m_arrayTiles(1), viewAxis);
    }
  }
  else
  {
    m_scrollTileIndex = viewAxis;

    if (synchStatus)
    {
      SDisplayTile* scrollTile = m_arrayTiles(m_scrollTileIndex);

      UpdateViewAxis(m_arrayTiles(m_arrayTiles.m_numItems - 1), scrollTile->vsTileInfo.viewAxis);

      m_arrayTiles(m_arrayTiles.m_numItems - 1)->vsTileInfo.slicePosFract = scrollTile->vsTileInfo.slicePosFract;
    }
  }

  return;
}

void CTileConfig::SetSecondaryView(int indexAxis, bool synchStatus)
{

  UpdateViewAxis(m_arrayTiles.m_pItemLast, indexAxis);

  if (synchStatus)
  {
    m_scrollTileIndex = indexAxis;
    m_arrayTiles(m_scrollTileIndex)->vsTileInfo.slicePosFract = m_arrayTiles.m_pItemLast->vsTileInfo.slicePosFract;
  }

  return;
}

void CTileConfig::UpdateViewAxis(SDisplayTile* tile, int viewAxis)
{

  tile->vsTileInfo.viewAxis = viewAxis;
  tile->tileScaleHorz = m_zoomFactor * m_baseScaleHorz[viewAxis];
  tile->tileScaleVert = m_zoomFactor * m_baseScaleVert[viewAxis];

  tile->scaleMatrix = XMMatrixScaling(tile->tileScaleHorz, tile->tileScaleVert, 1.0);
  tile->vsTileInfo.worldMatrix = tile->scaleMatrix * tile->transMatrix;

  return;
}

void CTileConfig::SetImagePos(float pos, bool synchStatus)
{
  SDisplayTile* scrollTile = m_arrayTiles(m_scrollTileIndex);

  scrollTile->vsTileInfo.slicePosFract = pos;

  if (synchStatus)
  {
    if (m_scrollTileIndex == (m_arrayTiles.m_numItems - 1))
    {
      m_arrayTiles(scrollTile->vsTileInfo.viewAxis)->vsTileInfo.slicePosFract = pos;
    }
    else
    {
      m_arrayTiles.m_pItemLast->vsTileInfo.slicePosFract = pos;
    }
  }

  return;
}

void CTileConfig::CalcSlicePositions(float posFraction)
{
  //SVoxGridGeometry* grid = &m_arrayDisplayPair->ItemPtr(0)->txtrGroupImgs->m_volumeGrid;

  //for (int axis = 0; axis < 3; ++axis)
  //{
  //  m_posSlices[axis] = (grid->voxelOrigin[axis] - grid->voxelSize[axis] / 2.0) + grid->voxelSize[axis] * grid->dims[axis] * posFraction;
  //}

  return;
}

void CTileConfig::SetZoomFactor(float zoomFactor)
{
  m_zoomFactor = zoomFactor;

  forArray(m_arrayTiles, tile)
  {
    tile->tileScaleHorz = zoomFactor * m_baseScaleHorz[tile->vsTileInfo.viewAxis];
    tile->tileScaleVert = zoomFactor * m_baseScaleVert[tile->vsTileInfo.viewAxis];

    tile->scaleMatrix = XMMatrixScaling(tile->tileScaleHorz, tile->tileScaleVert, 1.0);

    tile->vsTileInfo.worldMatrix = tile->scaleMatrix * tile->transMatrix;
    //tile->vsTileInfo.zoomFactor  = zoomFactor;
  }

  return;
}

void CTileConfig::SetImageTranslation(CPoint* translation)
{
  m_panOffsetScreenPix.x = translation->x;
  m_panOffsetScreenPix.y = translation->y;

  forArray(m_arrayTiles, tile)
  {
    tile->transMatrix = XMMatrixTranslation((translation->x * 2.0 / tile->tileRect.Width()),
      (-translation->y * 2.0 / tile->tileRect.Height()), 0.0);

    tile->vsTileInfo.worldMatrix = tile->scaleMatrix * tile->transMatrix;
  }

  return;
}

inline float CTileConfig::GetScrollTileSlicePos(void)
{
  return m_arrayTiles[m_scrollTileIndex].vsTileInfo.slicePosFract;
}
SDisplayTile* CTileConfig::FindTile(CPoint* point)
{

  forArray(m_arrayTiles, tile)
  {
    if (tile->tileRect.PtInRect(*point))
    {
      m_tilePicked = tile;

      return m_tilePicked;
    }
  }

  m_tilePicked = m_arrayTiles(0);

  return m_tilePicked;
}

SDisplayTile* CTileConfig::ConvertPoint(CPoint* point, CPoint* panOffset, SPt2* ptInfo)
{
  float numPix;

  SDisplayTile* tilePicked = FindTile(point);

  CRect* rect = &tilePicked->tileRect;

  ptInfo[1].x = point->x;
  ptInfo[1].y = point->y;

  numPix = m_zoomFactor * m_numScreenPixImgHorz[tilePicked->vsTileInfo.viewAxis];
  ptInfo[0].x = (point->x - panOffset->x - rect->left) - (rect->Width() - numPix) / 2;
  ptInfo[0].x /= numPix;

  numPix = m_zoomFactor * m_numScreenPixImgVert[tilePicked->vsTileInfo.viewAxis];
  ptInfo[0].y = (point->y - panOffset->y - rect->top) - (rect->Height() - numPix) / 2;
  ptInfo[0].y /= numPix;

  if (m_arrayTiles.m_numItems > 1)
  {
    SDisplayTile* mirrorTile;

    if (tilePicked == m_arrayTiles.ItemPtrLast())
    {
      if (m_arrayTiles.m_numItems > 2)
      {
        mirrorTile = m_arrayTiles(0) + tilePicked->vsTileInfo.viewAxis;
      }
      else
      {
        mirrorTile = m_arrayTiles.ItemPtrFirst();
      }
    }
    else
    {
      mirrorTile = m_arrayTiles.ItemPtrLast();
    }

    ptInfo[2].x = mirrorTile->tileRect.left + (point->x - tilePicked->tileRect.left);
    ptInfo[2].y = mirrorTile->tileRect.top  + (point->y - tilePicked->tileRect.top);
  }

  return tilePicked;
}

void CTileConfig::ConvertPtScreenPixToShader(CPoint* pointPix, SPt2* pointShader)
{
  CRect* rect = &m_tilePicked->tileRect;

  pointShader->x = (-1.0F + (pointPix->x - m_panOffsetScreenPix.x - rect->left) * 2.0F / rect->Width()) / m_tilePicked->tileScaleHorz;
  pointShader->y = (+1.0F - (pointPix->y - m_panOffsetScreenPix.y - rect->top) * 2.0F / rect->Height()) / m_tilePicked->tileScaleVert;

  return;
}