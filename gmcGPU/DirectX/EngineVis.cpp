#include "stdafx.h"
#include "DirectX.h"
#include "DoseDisplayVS.hxx"
#include "DoseDisplayPS.hxx"
#include "DoseLevelsPs.hxx"
#include "LineDrawUserVS.hxx"
#include "LineDrawUserPS.hxx"
#include "DoseDiffPS.hxx"
#include "Resampler3DCS.hxx"
#include "PointDoseCS.hxx"
#include "TileConfig.h"
#include "PointDose.h"
#include "ShaderResample.h"
#include "PhantomQA.h"
#include "Transport.h"
#include "EngineVis.h"

#define NUM_VOLUME_VIEWS    (3)  // Axial, Sagital, Coronal
#define NUM_RECT_VERTICES   (4)
#define NUM_VIEW_VERTICES   (NUM_RECT_VERTICES * NUM_VOLUME_VIEWS)

CEngineVis::CEngineVis(void) :
  m_brushWhite           (NULL),
  m_layoutDose           (NULL),
  m_infoMeasure          (NULL),
  m_infoPtDose           (NULL),
  m_visInfoPrim          (NULL),
  m_visInfoSec           (NULL),
  m_displayBoundsType    (DISPLAY_BOUNDS_PRIM),
  m_imageType            (0),
  m_doseDiffStatus       (0)
{
  return;
}

CEngineVis::~CEngineVis(void)
{
  SafeRelease(m_brushWhite);
  SafeRelease(m_layoutDose);

  SafeRelease(m_vShaderUserLine);
  SafeRelease(m_layoutContour);
  SafeRelease(m_pShaderUserLine);

  // MLW todo: clear dx 

  return;
}

void CEngineVis::ClearResources(void)
{

  m_arrayVisInfo.Clear();

  m_visInfoPrim = NULL;
  m_visInfoSec  = NULL;

  return;
}

bool CEngineVis::Initialize(void)
{
  CEngineBase::Initialize();

  ReturnOnFalse(InitializeVis2D());

  m_numViewerPixHorz = m_dxDisplayRect->Width();
  m_numViewerPixVert = m_dxDisplayRect->Height();

  return true;
}

bool CEngineVis::CreatePersistentResources(void)
{
  ReturnOnFalse
  (
    CreateTileVertexBuffer() &&
    CreateShaderBuffers()
  );

  return true;
}

bool CEngineVis::InitializeVis2D(void)
{
  // MLW check return, check putting in CGPU

  IDWriteFactory* pDWriteFactory;
  ReturnFailHR(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(pDWriteFactory), reinterpret_cast<IUnknown**>(&pDWriteFactory)));

  ReturnFailHR(pDWriteFactory->CreateTextFormat(L"Font Family", NULL,
    DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
    16.0, L"", &m_textFormat));

  pDWriteFactory->Release();

  m_dxRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White, 1.0f), &m_brushWhite);

  return true;
}

bool CEngineVis::CreateShaders(void)
{

  ReturnOnFalse
  (
    CreateShadersDoseDisplay() &&
    CreateShadersMeasure()
  );

  m_psDoseDiff.BuildShader(g_doseDiffPS, sizeof(g_doseDiffPS));
  m_psDoseLevels.BuildShader(g_doseLevelsPS, sizeof(g_doseLevelsPS));

  m_ptDoseCS.BuildShader(g_pointDoseCS, sizeof(g_pointDoseCS));

  return true;
}

bool CEngineVis::CreateShadersDoseDisplay(void)
{
  // Define the vertex shader input layout
  D3D11_INPUT_ELEMENT_DESC layout[] =
  {
    { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    { "POSITION", 1, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 }
  };

  ReturnFailHR(m_dxDevice->CreateInputLayout(layout, 3, g_doseDisplayVS, sizeof(g_doseDisplayVS), &m_layoutDose));

  m_vertexShaderDose.BuildShader(g_doseDisplayVS, sizeof(g_doseDisplayVS));

  m_pixelShaderDose.BuildShader(g_doseDisplayPS, sizeof(g_doseDisplayPS));

  return true;
}

bool CEngineVis::CreateShadersMeasure(void)
{

  // Define the input layout
  D3D11_INPUT_ELEMENT_DESC layout[] =
  {
    { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 0,                            D3D11_INPUT_PER_VERTEX_DATA, 0 },
    { "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }
  };

  ReturnFailHR(m_dxDevice->CreateInputLayout(layout, 2, g_lineDrawUserVS, sizeof(g_lineDrawUserVS), &m_layoutContour));

  // Create the line draw pixel shader
  m_dxDevice->CreatePixelShader(g_lineDrawUserPS, sizeof(g_lineDrawUserPS), NULL, &m_pShaderUserLine);

    // Create the line draw vertex shader
  ReturnFailHR(m_dxDevice->CreateVertexShader(g_lineDrawUserVS, sizeof(g_lineDrawUserVS), NULL, &m_vShaderUserLine));

  return true;
}

bool CEngineVis::CreateTileVertexBuffer(void)
{
  /*________________________________________________________________

    The tile vertices are in the following coordinate systems:
      Shader Viewport 3D coordinates, range -1:1
      Image Texture 3D coordinates,   range  0:1
      Screen 2D coordinates,          range  0:1
  __________________________________________________________________*/

  STileVertex tileVertices[] =
  {
    // SAGITTAL
    {
      XMFLOAT3(-1.0, -1.0,  0.5),  // Lower left vertex
      XMFLOAT3(0.0,  1.0,  0.0),
      XMFLOAT2(0.0,  1.0)
    },
     {
       XMFLOAT3(-1.0, 1.0,  0.5),  // Upper left vertex
       XMFLOAT3(0.0,  1.0,  1.0),
       XMFLOAT2(0.0,  0.0)
     },
     {
       XMFLOAT3(1.0, -1.0,  0.5),  // Lower right vertex
       XMFLOAT3(0.0,  0.0,  0.0),
       XMFLOAT2(1.0,  1.0)
     },
     {
       XMFLOAT3(1.0,  1.0,  0.5),  // Upper right vertex
       XMFLOAT3(0.0,  0.0,  1.0),
       XMFLOAT2(0.0,  1.0)
     },

    // CORONAL
     {
       XMFLOAT3(-1.0, -1.0,  0.5),  // Lower left vertex
       XMFLOAT3(0.0,  0.0,  0.0),
       XMFLOAT2(0.0,  1.0)
     },
     {
       XMFLOAT3(-1.0,  1.0,  0.5),  // Upper left vertex
       XMFLOAT3(0.0,  0.0,  1.0),
       XMFLOAT2(0.0,  0.0)
     },
     {
       XMFLOAT3(1.0, -1.0,  0.5),  // Lower right vertex
       XMFLOAT3(1.0,  0.0,  0.0),
       XMFLOAT2(1.0,  1.0)
     },
     {
       XMFLOAT3(1.0,  1.0,  0.5),  // Upper right vertex
       XMFLOAT3(1.0,  0.0,  1.0),
       XMFLOAT2(1.0,  0.0)
     },

    // AXIAL
     {
       XMFLOAT3(-1.0, -1.0,  0.5), // Lower left vertex
       XMFLOAT3(0.0,  1.0,  0.0),
       XMFLOAT2(0.0,  1.0)
     },
     {
       XMFLOAT3(-1.0,  1.0,  0.5), // Upper left vertex
       XMFLOAT3(0.0,  0.0,  0.0),
       XMFLOAT2(0.0,  0.0)
     },
     {
       XMFLOAT3(1.0, -1.0,  0.5),  // Lower right vertex
       XMFLOAT3(1.0,  1.0,  0.0),
       XMFLOAT2(1.0,  1.0)
     },
     {
       XMFLOAT3(1.0,  1.0,  0.5),  // Upper right vertex
       XMFLOAT3(1.0,  0.0,  0.0),
       XMFLOAT2(1.0,  0.0)
     },
  };

  m_tileVertexBuffer.CreateVertexBuffer(sizeof(STileVertex) * NUM_VIEW_VERTICES, tileVertices);

  return true;
}

bool CEngineVis::CreateShaderBuffers(void)
{

  m_vertextInfoCBuffer.CreateConstantBufferDyn(sizeof(STileInfoVS));

  m_vertexBuffMeasure.CreateConstantBufferDyn(sizeof(SContourVertex) * 8);

  m_dispatchCBuffer.CreateConstantBufferDyn(4*sizeof(unsigned int)); // MLW todo: make struct
  
  // Dose Pixel Shader params buffer // MLW todod: do we need for external
  m_psParamsBufferPrim.CreateConstantBufferDyn(sizeof(SDisplayParamsDosePS));
  m_psParamsBufferSec.CreateConstantBufferDyn(sizeof(SDisplayParamsDosePS));
  
  // Point Dose buffers
  m_ptDoseParamsCBuff.CreateConstantBufferDyn(sizeof(SPointDoseParams));

  m_ptDoseResultsBuff.CreateBuffRWTyped(8, DXGI_FORMAT_R32_FLOAT);
  m_ptDoseResultsBuff.CreateBufferRead();

  m_fieldLevelCBuffer.CreateConstantBufferDyn(sizeof(SFieldLevels), &m_fieldLevels);

  return true;
}

bool CEngineVis::CreateColorMapBuffer(SColorMap* colorMap)
{

  m_colorScaleCBuffer.CreateConstantBufferFixed(sizeof(SColorMap), colorMap);

  return true;
}

/*_____________________________________________________________________________________________

  Responding to results
_____________________________________________________________________________________________*/

void CEngineVis::LoadTransportInfo(void)
{

  ClearResources();

  m_txtrGroupDosesPatient->CalcFieldTransform(&m_txtrGroupImagesPatient->m_volumeGrid);

  if (m_txtrGroupDosesQA->m_numItems)
  {
    m_txtrGroupDosesQA->CalcFieldTransform(&m_txtrGroupImagesPhantom->m_volumeGrid);
  }

  BuildPairList();

  m_visInfoPrim = m_arrayVisInfo(0);

  if (m_arrayVisInfo.m_numItems > 2) // More than a single dose + external
  {
    m_visInfoSec = m_arrayVisInfo(1);
  }

  if (m_txtrGroupImagesPatient->m_volumeGrid.unitVecs[0][0] == 1.0)
  {
    m_imageLabelsCurrent = m_imageLabelsSupineHF[0];
  }
  else
  {
    m_imageLabelsCurrent = m_imageLabelsProneHF[0];
  }

  SetDisplayBoundsType(m_displayBoundsType);
  SetImageType(IMG_TYPE_HU);

  PrepDoseRender();

  return;
}

void CEngineVis::BuildPairList(void)
{

  int numFields = m_txtrGroupDosesPatient->m_numItems + m_txtrGroupDosesQA->m_numItems + 3; // + 2 For external, doseSum, gamma

  m_arrayVisInfo.Alloc(numFields);
  SVisInfoPS* visInfo = m_arrayVisInfo(0);

  forPrray(m_txtrGroupDosesPatient, txtr)
  {

    visInfo->arrayTxtureImages = m_txtrGroupImagesPatient;
    visInfo->shaderViews[0]    = m_txtrGroupImagesPatient->ItemPtr(m_imageType)->m_view;
    visInfo->shaderViews[1]    = txtr->m_view;

    visInfo->displayParams.fieldTransform = m_txtrGroupDosesPatient->m_fieldTransform;
    visInfo->doseMaxData = txtr->m_dataMax;
    visInfo->doseMinData = txtr->m_dataMin;
    visInfo->displayParams.doseMinDisplay = txtr->m_dataMin;
    visInfo->displayParams.doseMaxDisplay = txtr->m_dataMax;

    ++visInfo;
  }

  forPrray(m_txtrGroupDosesQA, txtr)
  {

    visInfo->arrayTxtureImages = m_txtrGroupImagesPhantom;
    visInfo->shaderViews[0]    = m_txtrGroupImagesPhantom->ItemPtr(m_imageType)->m_view;
    visInfo->shaderViews[1]    = txtr->m_view;

    visInfo->displayParams.fieldTransform = m_txtrGroupDosesQA->m_fieldTransform;
    visInfo->doseMaxData = txtr->m_dataMax;
    visInfo->doseMinData = txtr->m_dataMin;
    visInfo->displayParams.doseMinDisplay = txtr->m_dataMin;
    visInfo->displayParams.doseMaxDisplay = txtr->m_dataMax;

    ++visInfo;
  }

  return;
}

void CEngineVis::LoadExternalDoseDicom(void)
{
  // Add externl to display pair array
  m_txtrGroupDosesExternal->CalcFieldTransform(&m_txtrGroupImagesPatient->m_volumeGrid);

  SVisInfoPS* visInfo = m_arrayVisInfo.m_pItemLast - 2;

  CDxTexture3D* txtr = m_txtrGroupDosesExternal->ItemPtr(0);

  visInfo->arrayTxtureImages = m_txtrGroupImagesPatient;
  visInfo->shaderViews[0] = m_txtrGroupImagesPatient->ItemPtr(m_imageType)->m_view;
  visInfo->shaderViews[1] = txtr->m_view;

  visInfo->displayParams.fieldTransform = m_txtrGroupDosesExternal->m_fieldTransform;
  visInfo->doseMaxData = txtr->m_dataMax;
  visInfo->doseMinData = txtr->m_dataMin;
  visInfo->displayParams.doseMinDisplay = txtr->m_dataMin;
  visInfo->displayParams.doseMaxDisplay = txtr->m_dataMax;

  if (m_visInfoPrim)
  {
    m_visInfoSec = visInfo;
  }
  else
  {
    m_visInfoPrim = visInfo;
  }

  SetDisplayBoundsType(m_displayBoundsType);

  PrepDoseRender();

  return;
}

void CEngineVis::LoadDoseSum(void)
{
  SVisInfoPS* visInfo = m_arrayVisInfo.m_pItemLast - 1;

  CDxTexture3D* txtr = m_txtrGroupDosesExternal->ItemPtr(1);

  visInfo->arrayTxtureImages = m_txtrGroupImagesPatient;
  visInfo->shaderViews[0] = m_txtrGroupImagesPatient->ItemPtr(m_imageType)->m_view;
  visInfo->shaderViews[1] = txtr->m_view;

  visInfo->displayParams.fieldTransform = m_txtrGroupDosesExternal->m_fieldTransform;
  visInfo->doseMaxData = txtr->m_dataMax;
  visInfo->doseMinData = txtr->m_dataMin;
  visInfo->displayParams.doseMinDisplay = txtr->m_dataMin;
  visInfo->displayParams.doseMaxDisplay = txtr->m_dataMax;

  if (m_visInfoPrim)
  {
    m_visInfoSec = visInfo;
  }
  else
  {
    m_visInfoPrim = visInfo;
  }

  SetDisplayBoundsType(m_displayBoundsType);

  PrepDoseRender();

  return;
}

void CEngineVis::LoadGamma(void)
{
  // Add externl to display pair array
  m_txtrGroupGammas->CalcFieldTransform(&m_txtrGroupImagesPatient->m_volumeGrid);

  SVisInfoPS* visInfo = m_arrayVisInfo.m_pItemLast;

  CDxTexture3D* txtr = m_txtrGroupGammas->ItemPtr(0);

  visInfo->arrayTxtureImages = m_txtrGroupImagesPatient;
  visInfo->shaderViews[0] = m_txtrGroupImagesPatient->ItemPtr(m_imageType)->m_view;
  visInfo->shaderViews[1] = txtr->m_view;

  visInfo->displayParams.fieldTransform = m_txtrGroupGammas->m_fieldTransform;
  visInfo->doseMaxData = txtr->m_dataMax;
  visInfo->doseMinData = txtr->m_dataMin;
  visInfo->displayParams.doseMinDisplay = txtr->m_dataMin;
  visInfo->displayParams.doseMaxDisplay = txtr->m_dataMax;

  if (m_visInfoPrim)
  {
    m_visInfoSec = visInfo;
  }
  else
  {
    m_visInfoPrim = visInfo;
  }

  SetDisplayBoundsType(m_displayBoundsType);

  PrepDoseRender();

  return;
}

void CEngineVis::LoadExternalDoseMSH(void)
{

  LoadExternalDoseDicom();

  return;
}

void CEngineVis::SetResultsType(int slot, int indexResult)
{

  if (slot == 0)
  {
    m_visInfoPrim = m_arrayVisInfo(indexResult);
  }
  else
  {
    m_visInfoSec = m_arrayVisInfo(indexResult);
  }

  SetDisplayBoundsType(m_displayBoundsType);

  // Set buffers for next render
  if (slot == 0)
  {
    m_dxContext->PSSetConstantBuffers(2, 1, &m_psParamsBufferPrim.m_buffer);
    m_dxContext->PSSetShaderResources(0, 2, m_visInfoPrim->shaderViews);
  }

  return;
}

void  CEngineVis::SetTileConfig(CTileConfig* tileConfig)
{
  m_tileConfig = tileConfig;
  m_arrayTiles = &m_tileConfig->m_arrayTiles;

  return;
}

void  CEngineVis::CalcSlicePositions(float posFraction)
{
  //SVoxGridGeometry* grid = &m_arrayDisplayPair.ItemPtr(0)->txtrGroupImgs->m_volumeGrid;
  SVoxGridGeometry* grid = &m_visInfoPrim->arrayTxtureImages->m_volumeGrid;


  for (int axis = 0; axis < 3; ++axis)
  {
    m_posSlices[axis] = (grid->voxelOrigin[axis] - grid->voxelSize[axis] / 2.0) + grid->voxelSize[axis] * grid->dims[axis] * posFraction;
  }
  
  return;
}

void CEngineVis::SetDoseBoundsUser(float min, float max)
{
  SDisplayParamsDosePS* visParams;

  visParams = &m_visInfoPrim->displayParams;
  visParams->doseMinDisplay = min;
  visParams->doseMaxDisplay = max;

  m_psParamsBufferPrim.Upload(visParams);

  if (m_visInfoSec)
  {
    visParams = &m_visInfoSec->displayParams;
    visParams->doseMinDisplay = min;
    visParams->doseMaxDisplay = max;

    m_psParamsBufferSec.Upload(visParams);
  }

  return;
}

void CEngineVis::SetWindowLevels(float min, float max)
{
  D3D11_MAPPED_SUBRESOURCE mappedResource;

  m_fieldLevels.imageMin   = min;
  m_fieldLevels.imageMax   = max;
  m_fieldLevels.imageWidth = max - min;

  m_dxContext->Map(m_fieldLevelCBuffer.m_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
  memcpy(mappedResource.pData, &m_fieldLevels, sizeof(SFieldLevels));
  m_dxContext->Unmap(m_fieldLevelCBuffer.m_buffer, 0);

  return;
}

bool CEngineVis::DisplayDoseDiff(void)
{
  if (m_visInfoSec)
  {
    m_doseDiffStatus = !m_doseDiffStatus;
  }

  if (m_doseDiffStatus)
  {
    PrepDoseDiff();
  }

  return m_doseDiffStatus;
}

void  CEngineVis::SetImageType(int type)
{
  m_imageType = type;

  if (m_visInfoPrim->arrayTxtureImages->m_numItems > 1)
  {
    m_visInfoPrim->shaderViews[0] = m_visInfoPrim->arrayTxtureImages->ItemPtr(m_imageType)->m_view;
    m_dxContext->PSSetShaderResources(0, 1, &m_txtrGroupImagesPatient->ItemPtr(m_imageType)->m_view);
  }

  return;
}

void CEngineVis::SetDisplayBoundsType(int boundsType)
{
  SDisplayParamsDosePS* visParams;

  m_displayBoundsType = boundsType;

  switch (boundsType)
  {

  case DISPLAY_BOUNDS_PRIM:

    visParams = &m_visInfoPrim->displayParams;
    visParams->doseMinDisplay = m_visInfoPrim->doseMinData;
    visParams->doseMaxDisplay = m_visInfoPrim->doseMaxData;
    m_psParamsBufferPrim.Upload(visParams);

    if (m_visInfoSec)
    {
      visParams = &m_visInfoSec->displayParams;
      visParams->doseMinDisplay = m_visInfoPrim->doseMinData;
      visParams->doseMaxDisplay = m_visInfoPrim->doseMaxData;
      m_psParamsBufferSec.Upload(visParams);
    }
    break;

  case DISPLAY_BOUNDS_SEC:

    if(m_visInfoSec)
    {
      visParams = &m_visInfoPrim->displayParams;
      visParams->doseMinDisplay = m_visInfoSec->doseMinData;
      visParams->doseMaxDisplay = m_visInfoSec->doseMaxData;
      m_psParamsBufferPrim.Upload(visParams);

      visParams = &m_visInfoSec->displayParams;
      visParams->doseMinDisplay = m_visInfoSec->doseMinData;
      visParams->doseMaxDisplay = m_visInfoSec->doseMaxData;
      m_psParamsBufferSec.Upload(visParams);
    }
    break;

  case DISPLAY_BOUNDS_INDPNDNT:

    visParams = &m_visInfoPrim->displayParams;
    visParams->doseMinDisplay = m_visInfoPrim->doseMinData;
    visParams->doseMaxDisplay = m_visInfoPrim->doseMaxData;
    m_psParamsBufferPrim.Upload(visParams);

    if (m_visInfoSec)
    {
      visParams = &m_visInfoSec->displayParams;
      visParams->doseMinDisplay = m_visInfoSec->doseMinData;
      visParams->doseMaxDisplay = m_visInfoSec->doseMaxData;
      m_psParamsBufferSec.Upload(visParams);
    }
    break;
  }

  return;
}

void CEngineVis::PrepDoseDiff(void)
{

  // Load pixel shader
  ID3D11Buffer* psCBuffers[] = 
  {
    m_colorScaleCBuffer.m_buffer,
    m_fieldLevelCBuffer.m_buffer,
    m_psParamsBufferPrim.m_buffer,
    m_psParamsBufferSec.m_buffer
  };

  ID3D11ShaderResourceView* psViews[] = 
  { 
    m_txtrGroupImagesPatient->ItemPtr(1)->m_view, 
    m_visInfoPrim->shaderViews[1], 
    m_visInfoSec->shaderViews[1]
  };

  ID3D11SamplerState* psSamplers[] = 
  { m_textureSampler->m_sampler };

  m_psDoseDiff.SetConstantBuffers(psCBuffers, sizeof(psCBuffers));
  m_psDoseDiff.SetResourceViews(psViews, sizeof(psViews));
  m_psDoseDiff.SetSamplers(psSamplers, sizeof(psSamplers));
  m_psDoseDiff.LoadResources();

  return;
}

void CEngineVis::PrepDoseRender(void)
{
  m_dxContext->OMSetRenderTargets(1, &m_dxRenderTargetView, NULL);

  // Load vertex buffer, topology and layout
  UINT stride = sizeof(STileVertex);
  UINT offset = 0;
  m_dxContext->IASetVertexBuffers(0, 1, &m_tileVertexBuffer.m_buffer, &stride, &offset);
  m_dxContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
  m_dxContext->IASetInputLayout(m_layoutDose);

  // Load vertex shader 
  ID3D11Buffer* vsCBuffers[] = { m_vertextInfoCBuffer.m_buffer };

  m_vertexShaderDose.SetConstantBuffers(vsCBuffers, sizeof(vsCBuffers));
  m_vertexShaderDose.LoadResources();

  // Load pixel shader
  ID3D11Buffer* psCBuffers[]          = { m_colorScaleCBuffer.m_buffer, m_fieldLevelCBuffer.m_buffer, m_psParamsBufferPrim.m_buffer };
  ID3D11SamplerState* psSamplers[]    = { m_textureSampler->m_sampler };

  m_pixelShaderDose.SetConstantBuffers(psCBuffers, sizeof(psCBuffers));
  m_pixelShaderDose.SetResourceViews(m_visInfoPrim->shaderViews, 2*sizeof(ID3D11ShaderResourceView*));
  m_pixelShaderDose.SetSamplers(psSamplers, sizeof(psSamplers));
  m_pixelShaderDose.LoadResources();

  return;
}

void CEngineVis::ClearDisplay(void)
{
  m_dxContext->ClearRenderTargetView(m_dxRenderTargetView, m_colorBlack);
  m_dxSwapChain->Present(0, 0);

  return;
}

void CEngineVis::Render(void)
{
  if (m_doseDiffStatus)
  {
    Render2();
    return;
  }

  m_dxContext->ClearRenderTargetView(m_dxRenderTargetView, m_colorBlack);

  if (m_visInfoPrim->arrayTxtureImages)
  {
    forPrrayN(m_arrayTiles, tile, m_tileConfig->m_numTilesPrimary)
    {
      m_dxContext->RSSetViewports(1, &tile->viewPort);

      m_vertextInfoCBuffer.Upload(&tile->vsTileInfo);

      m_dxContext->Draw(4, tile->vsTileInfo.viewAxis * 4);
    }
  }

  if (m_visInfoSec && m_arrayTiles->m_numItems > 1)
  {
    // Set shader secondary dose resources
    m_dxContext->PSSetConstantBuffers(2, 1, &m_psParamsBufferSec.m_buffer);
    m_dxContext->PSSetShaderResources(0, 2, m_visInfoSec->shaderViews);

    SDisplayTile* tile = m_arrayTiles->ItemPtr(m_tileConfig->m_numTilesPrimary);

    m_dxContext->RSSetViewports(1, &tile->viewPort);

    m_vertextInfoCBuffer.Upload(&tile->vsTileInfo);

    m_dxContext->Draw(4, tile->vsTileInfo.viewAxis * 4);

    // Set shader primary dose resources for next render pass
    if (m_visInfoPrim->arrayTxtureImages)
    {
      m_dxContext->PSSetConstantBuffers(2, 1, &m_psParamsBufferPrim.m_buffer);
      m_dxContext->PSSetShaderResources(0, 2, m_visInfoPrim->shaderViews);
    }
    
  }

  m_dxRenderTarget->BeginDraw();

  DrawTileBoundaries();

  if (m_infoMeasure)
  {
    DrawMeasurement();
  }
  else if (m_infoPtDose)
  {
    DrawPts();
  }

  DrawSurfaceOverlays();

  m_dxRenderTarget->EndDraw();

  m_dxSwapChain->Present(0, 0);

  return;
}

void CEngineVis::Render2(void)
{
  D3D11_MAPPED_SUBRESOURCE mapResource;

  m_dxContext->ClearRenderTargetView(m_dxRenderTargetView, m_colorBlack);

  UINT stride = sizeof(STileVertex);
  UINT offset = 0;

  m_dxContext->IASetVertexBuffers(0, 1, &m_tileVertexBuffer.m_buffer, &stride, &offset);
  m_dxContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
  m_dxContext->IASetInputLayout(m_layoutDose);

  forPrrayN(m_arrayTiles, tile, m_tileConfig->m_numTilesPrimary)
  {
    m_dxContext->RSSetViewports(1, &tile->viewPort);

    m_dxContext->Map(m_vertextInfoCBuffer.m_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapResource);
    memcpy(mapResource.pData, &tile->vsTileInfo, sizeof(STileInfoVS));
    m_dxContext->Unmap(m_vertextInfoCBuffer.m_buffer, 0);

    m_dxContext->Draw(4, tile->vsTileInfo.viewAxis * 4);
  }

  m_dxSwapChain->Present(0, 0);

  return;
}

void CEngineVis::DrawTileBoundaries(void)
{
  D2D1_POINT_2F point1;
  D2D1_POINT_2F point2;

  for (int i = 1; i < m_tileConfig->m_numTilesHorz; ++i)
  {
    point1.x = float(i * (m_arrayTiles->m_pItemFirst->tileRect.Width() + 1) - 1);
    point1.y = 0.0;
    point2.x = point1.x;
    point2.y = float(m_numViewerPixVert);
    m_dxRenderTarget->DrawLine(point1, point2, m_brushWhite, 1.0, NULL);
  }

  for (int i = 1; i < m_tileConfig->m_numTilesVert; ++i)
  {
    point1.x = 0.0;
    point1.y = float(i * (m_arrayTiles->m_pItemFirst->tileRect.Height() + 1) - 1);
    point2.x = float(m_numViewerPixHorz);
    point2.y = point1.y;
    m_dxRenderTarget->DrawLine(point1, point2, m_brushWhite, 1.0, NULL);
  }

  return;
}

#include <wchar.h>
void CEngineVis::DrawSurfaceOverlays(void)
{
  WCHAR** labels;
  int rotation;
  int endTile;
  WCHAR slicePos[16];

  endTile = (m_visInfoSec && m_arrayTiles->m_numItems > 1) ? 
    m_tileConfig->m_numTilesPrimary + 1 : m_tileConfig->m_numTilesPrimary;

  forPrrayN(m_arrayTiles, tile, endTile)
  {
    labels = m_imageLabelsCurrent + tile->vsTileInfo.viewAxis * NUM_IMAGE_DIR_LABELS;
    //rotation = tile->imageGeometry->rotationIndex;
    rotation = 0; // MLW todo: generalize

    for (int vertex = 0; vertex < 4; ++vertex)
    {
      m_dxRenderTarget->DrawText(labels[(rotation + vertex) % NUM_IMAGE_DIR_LABELS], wcslen(labels[rotation]), m_textFormat,
        tile->orientTextRect[vertex], m_brushWhite);
    }

    swprintf(slicePos, 16, L"%8.2f mm", m_posSlices[tile->vsTileInfo.viewAxis]);

    m_dxRenderTarget->DrawText(slicePos, wcslen(slicePos), m_textFormat, tile->orientTextRect[4], m_brushWhite);
  }

  return;
}

void CEngineVis::DrawMeasurement(void)
{

  UINT offset = 0;
  UINT stride = sizeof(SContourVertex);

  m_dxContext->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINESTRIP);
  m_dxContext->IASetInputLayout(m_layoutContour);
  m_dxContext->IASetVertexBuffers(0, 1, &m_vertexBuffMeasure.m_buffer, &stride, &offset);
  m_dxContext->VSSetShader(m_vShaderUserLine, NULL, 0);
  m_dxContext->PSSetShader(m_pShaderUserLine, NULL, 0);


  m_dxContext->RSSetViewports(1, &m_infoMeasure->drawTile->viewPort);

  //if (m_infoMeasure->type) 
  SContourVertex vertices[5];
  if (0) // Area Measurement
  {
    vertices[0].position = XMFLOAT3(m_infoMeasure->origin.x, m_infoMeasure->origin.y, 0.5);
    vertices[0].color    = XMFLOAT4(0, 1, 0, 1);
    vertices[1].position = XMFLOAT3(m_infoMeasure->corner.x, m_infoMeasure->origin.y, 0.5);
    vertices[1].color    = XMFLOAT4(0, 1, 0, 1);
    vertices[2].position = XMFLOAT3(m_infoMeasure->corner.x, m_infoMeasure->corner.y, 0.5);
    vertices[2].color    = XMFLOAT4(0, 1, 0, 1);
    vertices[3].position = XMFLOAT3(m_infoMeasure->origin.x, m_infoMeasure->corner.y, 0.5);
    vertices[3].color    = XMFLOAT4(0, 1, 0, 1);
    vertices[4].position = XMFLOAT3(m_infoMeasure->origin.x, m_infoMeasure->origin.y, 0.5);
    vertices[4].color    = XMFLOAT4(0, 1, 0, 1);

    m_vertexBuffMeasure.Upload(vertices);

    m_dxContext->Draw(5, 0);
  }
  else // Distance Measurement
  {

    TRACE("DrawMeasure %f %f %f %f\n", m_infoMeasure->origin.x, m_infoMeasure->origin.y, m_infoMeasure->corner.x, m_infoMeasure->corner.y); // MLW dbg

    vertices[0].position = XMFLOAT3(m_infoMeasure->origin.x, m_infoMeasure->origin.y, 0.5);
    vertices[0].color    = XMFLOAT4(0, 1, 0, 1);
    vertices[1].position = XMFLOAT3(m_infoMeasure->corner.x, m_infoMeasure->corner.y, 0.5);
    vertices[1].color    = XMFLOAT4(0, 1, 0, 1);

    m_vertexBuffMeasure.Upload(vertices);

    m_dxContext->IASetVertexBuffers(0, 1, &m_vertexBuffMeasure.m_buffer, &stride, &offset);

    m_dxContext->Draw(2, 0);
  }

  stride = sizeof(STileVertex);
  m_dxContext->IASetVertexBuffers(0, 1, &m_tileVertexBuffer.m_buffer, &stride, &offset); // MLW check add to gpu.cpp
  m_dxContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
  m_dxContext->IASetInputLayout(m_layoutDose);
  m_dxContext->VSSetShader(m_vertexShaderDose.m_vertexShader, NULL, 0);
  m_dxContext->PSSetShader(m_pixelShaderDose.m_pixelShader, NULL, 0);

  return;
}

void CEngineVis::DrawPts(void)
{

  D2D1_POINT_2F point1;
  D2D1_POINT_2F point2;

  int endIndex;

  endIndex = (m_tileConfig->GetNumTiles() > 1 && m_visInfoSec) ? 2 : 1;

  for (int ptIndex = 0; ptIndex < endIndex; ++ptIndex)
  {
    point1.x = m_infoPtDose[ptIndex].x - 8;
    point1.y = m_infoPtDose[ptIndex].y;

    point2.x = point1.x + 17;
    point2.y = point1.y;

    m_dxRenderTarget->DrawLine(point1, point2, m_brushWhite, 1.0, NULL);

    point1.x = m_infoPtDose[ptIndex].x;
    point1.y = m_infoPtDose[ptIndex].y - 8;

    point2.x = point1.x;
    point2.y = point1.y + 17;

    m_dxRenderTarget->DrawLine(point1, point2, m_brushWhite, 1.0, NULL);
  }

  return;
}

void CEngineVis::PrepPointDose(void)
{

  if (m_visInfoSec && m_arrayTiles->m_numItems > 1)
  {
    ID3D11Buffer* buffers[] = { m_ptDoseParamsCBuff.m_buffer, m_psParamsBufferPrim.m_buffer, m_psParamsBufferSec.m_buffer };
    m_ptDoseCS.SetConstantBuffers(buffers, sizeof(buffers));

    ID3D11ShaderResourceView* views[] = { m_visInfoPrim->shaderViews[0], m_visInfoPrim->shaderViews[1], m_visInfoSec->shaderViews[1] };

    m_ptDoseCS.SetResourceViews(views, 3 * sizeof(ID3D11ShaderResourceView*));
  }
  else
  {
    ID3D11Buffer* buffers[] = { m_ptDoseParamsCBuff.m_buffer, m_psParamsBufferPrim.m_buffer };
    m_ptDoseCS.SetConstantBuffers(buffers, sizeof(buffers));

    ID3D11ShaderResourceView* views[4] = {
      m_visInfoPrim->shaderViews[0],
      m_txtrGroupDosesPatient->ItemPtr(DOSE_TYPE_DOSE)->m_view,
      NULL,
      NULL };

    if (m_transportParams->doseAux)
    {
      views[2] = m_txtrGroupDosesPatient->ItemPtr(DOSE_TYPE_LET)->m_view;
      views[3] = m_txtrGroupDosesPatient->ItemPtr(DOSE_TYPE_BIODOSE)->m_view;
    }

    m_ptDoseCS.SetResourceViews(views, 4 * sizeof(ID3D11ShaderResourceView*));
  }

  ID3D11UnorderedAccessView* uavViews[] = { m_ptDoseResultsBuff.m_viewRW };
  m_ptDoseCS.SetUAVViews(uavViews, sizeof(uavViews));

  ID3D11SamplerState* samplers[] = { m_textureSampler->m_sampler };
  m_ptDoseCS.SetSamplers(samplers, sizeof(samplers));

  m_ptDoseCS.LoadResources();

  return;
}

void CEngineVis::CalcPointDose(SPt3* dosePt, SPt2* imagePt, SDisplayTile* tile, float* results)
{

  float imagePos = tile->vsTileInfo.slicePosFract;

  D3D11_MAPPED_SUBRESOURCE mappedResource;
  m_dxContext->Map(m_ptDoseParamsCBuff.m_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);

  SPointDoseParams* mapParams = (SPointDoseParams*) mappedResource.pData;
  mapParams->dosePoint = *dosePt;
  mapParams->secondaryFlag = m_visInfoSec ? 1 : 0;

  m_dxContext->Unmap(m_ptDoseParamsCBuff.m_buffer, 0);

  m_dxContext->Dispatch(1, 1, 1);

  m_dxContext->CopyResource(m_ptDoseResultsBuff.m_bufferRead, m_ptDoseResultsBuff.m_buffer);
  m_dxContext->Map(m_ptDoseResultsBuff.m_bufferRead, 0, D3D11_MAP_READ, 0, &mappedResource);

  memcpy(results, mappedResource.pData, 8 * sizeof(float));

  float* see = (float*)mappedResource.pData;

  TRACE("DosePt %f %f %f\n%f %f %f %f %f\n", see[0], see[1], see[2], see[3], see[4], see[5], see[6], see[7]);

  m_dxContext->Unmap(m_ptDoseResultsBuff.m_bufferRead, 0);

  m_infoPtDose = imagePt;

  Render();

  m_infoPtDose = NULL;

  return;
}






