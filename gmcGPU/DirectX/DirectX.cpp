#include "stdafx.h"
#include "Logger.h"
#include "DirectX.h"

// Need to undef for non-development deployment
#define DBG_GPU

CDirectX::CDirectX(void) :
  m_device            (NULL),
  m_context           (NULL),
  m_swapChain         (NULL),
  m_renderTarget      (NULL),
  m_renderTargetView  (NULL),
  m_numBufferBytesMax (0)
{
  return;
}

CDirectX::~CDirectX(void)
{

  SafeRelease(m_renderTargetView);
  SafeRelease(m_renderTarget);
  SafeRelease(m_swapChain);
  SafeRelease(m_context);
  SafeRelease(m_device);

  return;
}

bool CDirectX::Initialize(CString* gpuName)
{
  IDXGIAdapter* adapter;
  CStringW searchName(*gpuName);

  if ((adapter = SelectDevice(searchName)) == NULL)
  {
    // Desired device not found. List all possible devices to log file.
    ListDevices();

    CString searchName1(searchName);
    APPLOG("No GPU: %s\n\n", (LPCTSTR) searchName1);

    return false;
  }

  ReturnOnFalse(InitializeGPU(adapter) && CalcMaxBufferSize());

  return true;
}

bool CDirectX::InitializeGPU(IDXGIAdapter* adapter)
{  
  bool status;

  D3D_FEATURE_LEVEL featureLevels[] =
  {   
    D3D_FEATURE_LEVEL_11_0,
  };

  UINT flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT | D3D11_CREATE_DEVICE_DISABLE_GPU_TIMEOUT;

#ifdef _DEBUG
  flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

  if (FAILED(D3D11CreateDevice(adapter, D3D_DRIVER_TYPE_UNKNOWN, 0, flags, featureLevels, ARRAYSIZE( featureLevels ),
    D3D11_SDK_VERSION, &m_device, 0, &m_context)))
  {
    APPLOG("FAIL: Selected GPU Not Responding\n")
    status =  false;
  }
  else
  {
    status = true;
  }
  
  if (adapter)
  {
    adapter->Release();
  }

  CDxObj::m_dxDevice  = m_device;
  CDxObj::m_dxContext = m_context;

  return status;
}

#include <dxgi1_4.h>
bool CDirectX::CalcMaxBufferSize(void)
{
  IDXGIFactory4* pFactory;
  CreateDXGIFactory1(__uuidof(IDXGIFactory4), (void**)&pFactory);

  IDXGIAdapter3* adapter3;
  pFactory->EnumAdapters(0, reinterpret_cast<IDXGIAdapter**>(&adapter3));

  DXGI_QUERY_VIDEO_MEMORY_INFO videoMemoryInfo;
  adapter3->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &videoMemoryInfo);

  // DirectX max of single buffer is max of one fourth total memory or 128*1024
  m_numBufferBytesMax =  max(128 * 1024, (videoMemoryInfo.Budget / 4));

  CDxObj::m_dxNumBufferBytesMax = m_numBufferBytesMax;

  return true;
}

IDXGIAdapter* CDirectX::SelectDevice(LPCWSTR deviceName)
{
  IDXGIFactory1* factory;

  if (FAILED(CreateDXGIFactory1(__uuidof(IDXGIFactory1), (void**)&factory)))
  {
    APPLOG("FAIL: DirectX Factory\n")
    return NULL;
  }

  IDXGIAdapter1* adapter = NULL;
  UINT adapter_id = 0;
  DXGI_ADAPTER_DESC desc;

  // Loop over existing adapters to hopefullf find desired device
  while (factory->EnumAdapters1(adapter_id, &adapter) != DXGI_ERROR_NOT_FOUND)
  {
    adapter->GetDesc(&desc);
    
    if (wcsstr(desc.Description, deviceName))
    {
      CString adapterName(desc.Description); // Convert from wide char to char for logging
      APPLOG("Selected Adapter %d %s\n\n", adapter_id, (LPCTSTR)adapterName);

      break;
    }

    adapter->Release();
    adapter = NULL;
    ++adapter_id;
  }

  factory->Release();

  return adapter;
}

bool CDirectX::ListDevices(void)
{
  IDXGIFactory1* factory;

  if (FAILED(CreateDXGIFactory1(__uuidof(IDXGIFactory1), (void**)&factory)))
  {
    APPLOG("FAIL: DirectX Factory\n");
    return false;
  }

  IDXGIAdapter1* adapter = NULL;
  UINT adapterIndex = 0;
  DXGI_ADAPTER_DESC desc;

  while (factory->EnumAdapters1(adapterIndex, &adapter) != DXGI_ERROR_NOT_FOUND)
  {
    adapter->GetDesc(&desc);

    CString adapterName(desc.Description); // Convert from wide char to char
    APPLOG("Display Adapter %d  %x %x %x %x %s\n", adapterIndex, desc.DeviceId, desc.SubSysId, 
      desc.AdapterLuid.HighPart, desc.AdapterLuid.LowPart, (LPCTSTR)adapterName);

    adapter->Release();
    adapter = NULL;
    ++adapterIndex;
  }

  factory->Release();

  return adapter;
}

bool CDirectX::InitializeGraphics(UINT flags, CWnd* graphicsWnd)
{
  if(
    ((flags & GPU_VIS3D) && InitializeVis3D(graphicsWnd))   &&
    ((flags & GPU_VIS2D) && InitializeVis2D())
    )
  {
    return true;
  }

  return false;
}

bool CDirectX::InitializeVis3D(CWnd* graphicsWnd)
{
  HRESULT hr;

  graphicsWnd->GetClientRect(&m_displayRect);
  CDxObj::m_dxDisplayRect = &m_displayRect;

  IDXGIDevice2* pDXGIDevice;
  if (FAILED(hr = m_device->QueryInterface(__uuidof(IDXGIDevice2), (void**)&pDXGIDevice)))
  {
    return false;
  }

  IDXGIAdapter* pDXGIAdapter;
  hr = pDXGIDevice->GetParent(__uuidof(IDXGIAdapter), (void**)&pDXGIAdapter);
  pDXGIDevice->Release();
  if (FAILED(hr))
  {
    return false;
  }

  IDXGIFactory2* pIDXGIFactory;
  hr = pDXGIAdapter->GetParent(__uuidof(IDXGIFactory2), (void**)&pIDXGIFactory);
  pDXGIAdapter->Release();
  if (FAILED(hr))
  {
    return false;
  }

  DXGI_SWAP_CHAIN_DESC1 swapDesc;
  ZeroMemory(&swapDesc, sizeof(swapDesc));

  swapDesc.BufferCount        = 1;
  swapDesc.Width              = m_displayRect.Width();
  swapDesc.Height             = m_displayRect.Height();
  swapDesc.Format             = DXGI_FORMAT_B8G8R8A8_UNORM;
  swapDesc.BufferUsage        = DXGI_USAGE_RENDER_TARGET_OUTPUT;
  swapDesc.SwapEffect         = DXGI_SWAP_EFFECT_DISCARD;
  swapDesc.SampleDesc.Count   = 1;
  swapDesc.SampleDesc.Quality = 0;
  swapDesc.Flags              = DXGI_SWAP_CHAIN_FLAG_GDI_COMPATIBLE;

  hr = pIDXGIFactory->CreateSwapChainForHwnd(m_device, graphicsWnd->GetSafeHwnd(), &swapDesc, NULL, NULL, &m_swapChain);
  pIDXGIFactory->Release();
  if (FAILED(hr))
  {
    return false;
  }

  CDxObj::m_dxSwapChain = m_swapChain;

  // Create a render target view
  ID3D11Texture2D* backBufferTexture = NULL;
  if (FAILED(m_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&backBufferTexture)))
  {
    return false;
  }

  hr = m_device->CreateRenderTargetView(backBufferTexture, NULL, &m_renderTargetView);
  backBufferTexture->Release();

  CDxObj::m_dxRenderTargetView = m_renderTargetView;

  return true;
}

bool CDirectX::InitializeVis2D(void)
{
  HRESULT hr;

  D2D1_PIXEL_FORMAT pixelFormat = D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE);

  D2D1_RENDER_TARGET_PROPERTIES renderProperties;
  renderProperties.dpiX        = 0;
  renderProperties.dpiY        = 0;
  renderProperties.minLevel    = D2D1_FEATURE_LEVEL_DEFAULT;
  renderProperties.pixelFormat = pixelFormat;
  renderProperties.usage       = D2D1_RENDER_TARGET_USAGE_GDI_COMPATIBLE;
  renderProperties.type        = D2D1_RENDER_TARGET_TYPE_DEFAULT;

  IDXGISurface* backBufferSurface = NULL;
  if (FAILED(m_swapChain->GetBuffer(0, __uuidof(IDXGISurface), (LPVOID*)&backBufferSurface)))
  {
    return false;
  }

  ID2D1Factory* factory;
  hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &factory);
  if(FAILED(hr))
  {
    SafeRelease(backBufferSurface);
    return false;
  }

  hr = factory->CreateDxgiSurfaceRenderTarget(backBufferSurface, &renderProperties, &m_renderTarget);
  SafeRelease(backBufferSurface);
  SafeRelease(factory);
  if (FAILED(hr))
  {
    return false;
  }

  CDxObj::m_dxRenderTarget = m_renderTarget;

  return true;
}

ID3D11Device*           CDxObj::m_dxDevice            = NULL;
ID3D11DeviceContext*    CDxObj::m_dxContext           = NULL;
IDXGISwapChain1*        CDxObj::m_dxSwapChain         = NULL;
ID2D1RenderTarget*      CDxObj::m_dxRenderTarget      = NULL;
ID3D11RenderTargetView* CDxObj::m_dxRenderTargetView  = NULL;
CRect*                  CDxObj::m_dxDisplayRect       = NULL;
UINT64                  CDxObj::m_dxNumBufferBytesMax = 0;

ID3D11UnorderedAccessView* const CDxObj::m_nullViewRW = NULL;
ID3D11ShaderResourceView*  const CDxObj::m_nullView   = NULL;

CDxShader::CDxShader(void) :
  m_constantBuffers    (NULL),
  m_bufferViews        (NULL),
  m_uavViews           (NULL),
  m_textureSamplers    (NULL),
  m_numConstantBuffers (0),
  m_numBufferViews     (0),
  m_numUAVViews        (0),
  m_numSamplers        (0)
{
  return;
}

CDxShader::~CDxShader(void)
{
  SafeDeleteArray(m_constantBuffers);
  SafeDeleteArray(m_bufferViews);
  SafeDeleteArray(m_uavViews);
  SafeDeleteArray(m_textureSamplers);

  return;
}

void CDxShader::SetConstantBuffers(ID3D11Buffer** buffers, size_t inputSize) // MLW todo: template these resources
{
  int numBuffers = (int) (inputSize / sizeof(ID3D11Buffer*));

  if (numBuffers > m_numConstantBuffers) // MLW use UniArray
  {
    SafeDelete(m_constantBuffers)

    m_constantBuffers = new ID3D11Buffer*[numBuffers];
  }

  m_numConstantBuffers = numBuffers;
  
  memcpy(m_constantBuffers, buffers, m_numConstantBuffers * sizeof(ID3D11Buffer*));

  return;
}

void CDxShader::SetResourceViews(ID3D11ShaderResourceView** views, size_t inputSize)
{
  int numViews = (int) (inputSize / sizeof(ID3D11ShaderResourceView*));

  if (numViews > m_numBufferViews)
  {
    SafeDelete(m_bufferViews)

    m_bufferViews = new ID3D11ShaderResourceView*[numViews];
  }

  m_numBufferViews = numViews;

  memcpy(m_bufferViews, views, m_numBufferViews * sizeof(ID3D11ShaderResourceView*));

  return;
}

void CDxShader::SetUAVViews(ID3D11UnorderedAccessView** views, size_t inputSize)
{
  int numViews = (int) (inputSize / sizeof(ID3D11UnorderedAccessView*));

  if (numViews > m_numUAVViews)
  {
    SafeDelete(m_uavViews)

    m_uavViews = new ID3D11UnorderedAccessView*[numViews];
  }

  m_numUAVViews = numViews;

  memcpy(m_uavViews, views, m_numUAVViews * sizeof(ID3D11UnorderedAccessView*));

  return;
}

void CDxShader::SetSamplers(ID3D11SamplerState** samplers, size_t inputSize)
{
  int numSamplers = (int)( inputSize / sizeof(ID3D11SamplerState*));

  if (numSamplers > m_numSamplers)
  {
    SafeDelete(m_textureSamplers)

    m_textureSamplers = new ID3D11SamplerState*[numSamplers];
  }

  m_numSamplers = numSamplers;

  memcpy(m_textureSamplers, samplers, m_numSamplers*sizeof(ID3D11SamplerState*));

  return;
}

CDxShaderCompute::CDxShaderCompute(void) :
  m_computeShader (NULL)
{
  return;
}

CDxShaderCompute::~CDxShaderCompute(void)
{

  SafeRelease(m_computeShader);

  return;
}

bool CDxShaderCompute::BuildShader(const BYTE* shaderBody, size_t shaderSize)
{

  if (FAILED(m_dxDevice->CreateComputeShader(shaderBody, shaderSize, nullptr, &m_computeShader)))
  {
    return false;
  }

  return true;
}

void CDxShaderCompute::LoadResources(void)
{

  m_dxContext->CSSetConstantBuffers(0, m_numConstantBuffers, m_constantBuffers);
  m_dxContext->CSSetShaderResources(0, m_numBufferViews, m_bufferViews);
  m_dxContext->CSSetUnorderedAccessViews(0, m_numUAVViews, m_uavViews, NULL);

  m_dxContext->CSSetShader(m_computeShader, NULL, 0);

  m_dxContext->CSSetSamplers(0, m_numSamplers, m_textureSamplers);

  return;
}

CDxShaderVertex::CDxShaderVertex(void) :
  m_vertexShader (NULL)
{
  return;
}

CDxShaderVertex::~CDxShaderVertex(void)
{

  SafeRelease(m_vertexShader);

  return;
}

bool CDxShaderVertex::BuildShader(const BYTE* shaderBody, size_t shaderSize)
{

  if (FAILED(m_dxDevice->CreateVertexShader(shaderBody, shaderSize, nullptr, &m_vertexShader)))
  {
    return false;
  }

  return true;
}

void CDxShaderVertex::LoadResources(void)
{

  m_dxContext->VSSetConstantBuffers(0, m_numConstantBuffers, m_constantBuffers);
  m_dxContext->VSSetShaderResources(0, m_numBufferViews, m_bufferViews);

  m_dxContext->VSSetShader(m_vertexShader, NULL, 0);

  return;
}

CDxShaderPixel::CDxShaderPixel(void) :
  m_pixelShader (NULL)
{
  return;
}

CDxShaderPixel::~CDxShaderPixel(void)
{

  SafeRelease(m_pixelShader);

  return;
}

bool CDxShaderPixel::BuildShader(const BYTE* shaderBody, size_t shaderSize)
{

  if (FAILED(m_dxDevice->CreatePixelShader(shaderBody, shaderSize, nullptr, &m_pixelShader)))
  {
    return false;
  }

  return true;
}

void CDxShaderPixel::LoadResources(void)
{

  m_dxContext->PSSetConstantBuffers(0, m_numConstantBuffers, m_constantBuffers);
  m_dxContext->PSSetShaderResources(0, m_numBufferViews, m_bufferViews);
  m_dxContext->PSSetSamplers(0, m_numSamplers, m_textureSamplers);

  m_dxContext->PSSetShader(m_pixelShader, NULL, 0);

  return;
}

CDxBuffConstant::CDxBuffConstant(void) : 
  m_buffer     (NULL), 
  m_bufferSize (0) 
{
  return;
};

CDxBuffConstant::~CDxBuffConstant(void) 
{ 
  Release();

  return;
};

void CDxBuffConstant::Release(void) 
{ 
  SafeRelease(m_buffer);

  return;
};

void CDxBuffConstant::CreateConstantBufferDyn(unsigned int byteWidth, void* data)
{

  D3D11_BUFFER_DESC bufferDesc;
  ZeroMemory(&bufferDesc, sizeof(D3D11_BUFFER_DESC));

  bufferDesc.Usage          = D3D11_USAGE_DYNAMIC;
  bufferDesc.ByteWidth      = byteWidth;
  bufferDesc.BindFlags      = D3D11_BIND_CONSTANT_BUFFER;
  bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

  D3D11_SUBRESOURCE_DATA* resDataPtr = NULL;
  D3D11_SUBRESOURCE_DATA resData = {};

  if (data)
  {
    resData.pSysMem = data;
    resDataPtr = &resData;
  }

  m_dxDevice->CreateBuffer(&bufferDesc, resDataPtr, &m_buffer);

  m_bufferSize = byteWidth;

  return;
}

void CDxBuffConstant::CreateConstantBufferFixed(unsigned int byteWidth, void* data)
{

  D3D11_BUFFER_DESC bufferDesc;
  ZeroMemory(&bufferDesc, sizeof(D3D11_BUFFER_DESC));

  bufferDesc.Usage     = D3D11_USAGE_IMMUTABLE;
  bufferDesc.ByteWidth = byteWidth;
  bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

  D3D11_SUBRESOURCE_DATA resData = {};
  resData.pSysMem = data;

  m_dxDevice->CreateBuffer(&bufferDesc, &resData, &m_buffer);

  m_bufferSize = byteWidth;

  return;
}

void CDxBuffConstant::CreateVertexBuffer(unsigned int byteWidth, void* data, D3D11_USAGE usage)
{

  D3D11_BUFFER_DESC bufferDesc;
  ZeroMemory(&bufferDesc, sizeof(D3D11_BUFFER_DESC));

  bufferDesc.Usage           = usage;
  bufferDesc.ByteWidth       = byteWidth;
  bufferDesc.BindFlags       = D3D11_BIND_VERTEX_BUFFER;
  //bufferDesc.CPUAccessFlags  = D3D11_CPU_ACCESS_WRITE;

  D3D11_SUBRESOURCE_DATA resData = {};
  resData.pSysMem = data;

  m_dxDevice->CreateBuffer(&bufferDesc, &resData, &m_buffer);

  m_bufferSize = byteWidth;

  return;
}

void CDxBuffConstant::Upload(void* inputData)
{
  D3D11_MAPPED_SUBRESOURCE mapResource;

  if (m_buffer)
  {
    m_dxContext->Map(m_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapResource);
    memcpy(mapResource.pData, inputData, m_bufferSize);
    m_dxContext->Unmap(m_buffer, 0);
  }

  return;
}

CDxBuffResource::CDxBuffResource(void) :
  m_buffer (NULL),
  m_view   (NULL)
{
  return;
}

CDxBuffResource::~CDxBuffResource(void)
{
  Release();

  return;
}

void CDxBuffResource::Release(void)
{
  SafeRelease(m_buffer);
  SafeRelease(m_view);

  return;
}

void CDxBuffResource::CreateStructuredBufferFixed(unsigned int numItems, unsigned int itemSize, void* data)
{
  if (numItems == 0)
  {
    return;
  }

  D3D11_BUFFER_DESC desc = {};
  D3D11_SUBRESOURCE_DATA resData = {};

  desc.BindFlags           = D3D11_BIND_SHADER_RESOURCE;
  desc.Usage               = D3D11_USAGE_IMMUTABLE;
  desc.MiscFlags           = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
  desc.CPUAccessFlags      = 0;
  desc.ByteWidth           = numItems * itemSize;
  desc.StructureByteStride = itemSize;

  resData.pSysMem = data;

  m_dxDevice->CreateBuffer(&desc, &resData, &m_buffer);

  D3D11_SHADER_RESOURCE_VIEW_DESC viewDesc;
  ZeroMemory(&viewDesc, sizeof(viewDesc));

  viewDesc.Format              = DXGI_FORMAT_UNKNOWN;
  viewDesc.ViewDimension       = D3D11_SRV_DIMENSION_BUFFER;
  viewDesc.Buffer.FirstElement = 0;
  viewDesc.Buffer.NumElements  = numItems;

  m_dxDevice->CreateShaderResourceView(m_buffer, &viewDesc, &m_view);

  return;
}

void CDxBuffResource::CreateResourceBufferFixed(unsigned int numItems, unsigned int itemSize, void* data,
  DXGI_FORMAT format)
{

  if (numItems == 0)
  {
    return;
  }

  D3D11_BUFFER_DESC desc = {};
  D3D11_SUBRESOURCE_DATA resData = {};

  desc.BindFlags           = D3D11_BIND_SHADER_RESOURCE;
  desc.Usage               = D3D11_USAGE_IMMUTABLE;
  desc.ByteWidth           = numItems * itemSize;
  desc.StructureByteStride = itemSize;

  resData.pSysMem = data;

  m_dxDevice->CreateBuffer(&desc, &resData, &m_buffer);

  D3D11_SHADER_RESOURCE_VIEW_DESC viewDesc;
  ZeroMemory(&viewDesc, sizeof(viewDesc));

  viewDesc.Format              = format;
  viewDesc.ViewDimension       = D3D11_SRV_DIMENSION_BUFFER;
  viewDesc.Buffer.NumElements  = numItems;

  m_dxDevice->CreateShaderResourceView(m_buffer, &viewDesc, &m_view);

  return;
}

void CDxBuffResource::CreateStructuredBufferDefault(unsigned int numItems, unsigned int itemSize, void* data)
{
  D3D11_BUFFER_DESC desc = {};

  desc.BindFlags           = D3D11_BIND_SHADER_RESOURCE;
  desc.Usage               = D3D11_USAGE_DEFAULT;
  desc.MiscFlags           = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
  desc.CPUAccessFlags      = 0;
  desc.ByteWidth           = numItems * itemSize;
  desc.StructureByteStride = itemSize;

  if (data)
  {
    D3D11_SUBRESOURCE_DATA resData = {};
    resData.pSysMem = data;

    m_dxDevice->CreateBuffer(&desc, &resData, &m_buffer);
  }
  else
  {
    m_dxDevice->CreateBuffer(&desc, NULL, &m_buffer);
  }

  D3D11_SHADER_RESOURCE_VIEW_DESC viewDesc;
  ZeroMemory(&viewDesc, sizeof(viewDesc));

  viewDesc.Format              = DXGI_FORMAT_UNKNOWN;
  viewDesc.ViewDimension       = D3D11_SRV_DIMENSION_BUFFER;
  viewDesc.Buffer.FirstElement = 0;
  viewDesc.Buffer.NumElements  = numItems;

  m_dxDevice->CreateShaderResourceView(m_buffer, &viewDesc, &m_view);

  return;
}

CDxBuffRW::CDxBuffRW(void) :
 m_buffer      (NULL),
 m_bufferRead  (NULL),
 m_viewRW      (NULL),
 m_view        (NULL)
{
  return;
}

CDxBuffRW::~CDxBuffRW(void)
{
  Release();

  return;
}

void CDxBuffRW::Release(void)
{
  SafeRelease(m_buffer);
  SafeRelease(m_bufferRead);
  SafeRelease(m_viewRW);
  SafeRelease(m_view);

  return;
}

void CDxBuffRW::CreateBuffRWTyped(unsigned int numItems,  DXGI_FORMAT format, void* data)
{

  m_numItems = numItems;
  
  switch (format)
  {
  case DXGI_FORMAT_R32_FLOAT:
    m_sizeItem = sizeof(float);
    break;
  case DXGI_FORMAT_R32G32_FLOAT:
    m_sizeItem = 2 * sizeof(float);
    break;
  default:
    m_sizeItem = 4;
    break;
  };

  D3D11_BUFFER_DESC desc = {};

  desc.ByteWidth           = numItems * m_sizeItem;
  desc.Usage               = D3D11_USAGE_DEFAULT;
  desc.BindFlags           = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
  desc.CPUAccessFlags      = D3D11_CPU_ACCESS_WRITE;
  desc.StructureByteStride = m_sizeItem;

  if (data)
  {
    D3D11_SUBRESOURCE_DATA bufferData;
    bufferData.pSysMem          = data;
    bufferData.SysMemPitch      = 0;
    bufferData.SysMemSlicePitch = 0;

    m_dxDevice->CreateBuffer(&desc, &bufferData, &m_buffer);
  }
  else
  {
    m_dxDevice->CreateBuffer(&desc, NULL, &m_buffer);
  }

  D3D11_UNORDERED_ACCESS_VIEW_DESC unorderedDesc = {};

  unorderedDesc.Format              = format;
  unorderedDesc.ViewDimension       = D3D11_UAV_DIMENSION_BUFFER;
  unorderedDesc.Buffer.FirstElement = 0;
  unorderedDesc.Buffer.NumElements  = numItems;

  m_dxDevice->CreateUnorderedAccessView(m_buffer, &unorderedDesc, &m_viewRW);

  D3D11_SHADER_RESOURCE_VIEW_DESC viewDesc;
  ZeroMemory(&viewDesc, sizeof(viewDesc));

  viewDesc.Format              = format;
  viewDesc.ViewDimension       = D3D11_SRV_DIMENSION_BUFFER;
  viewDesc.Buffer.FirstElement = 0;
  viewDesc.Buffer.NumElements  = numItems;

  m_dxDevice->CreateShaderResourceView(m_buffer, &viewDesc, &m_view);

  return;
}

void CDxBuffRW::CreateBuffRWStruct(unsigned int numItems, unsigned int itemSize, void* data, bool appendFlag)
{
  m_numItems = numItems;
  m_sizeItem = itemSize;

  D3D11_BUFFER_DESC desc = {};

  desc.BindFlags           = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
  desc.Usage               = D3D11_USAGE_DEFAULT;
  desc.MiscFlags           = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
  desc.CPUAccessFlags      = 0;
  desc.ByteWidth           = numItems * itemSize;
  desc.StructureByteStride = itemSize;

  D3D11_SUBRESOURCE_DATA resData     = {};
  D3D11_SUBRESOURCE_DATA* resDataPtr = NULL;
  if (data)
  {
    resData.pSysMem = data;
    resDataPtr = &resData;
  }

  m_dxDevice->CreateBuffer(&desc, resDataPtr, &m_buffer);

  D3D11_UNORDERED_ACCESS_VIEW_DESC uaViewDesc = {};

  uaViewDesc.Format              = DXGI_FORMAT_UNKNOWN;
  uaViewDesc.ViewDimension       = D3D11_UAV_DIMENSION_BUFFER;
  uaViewDesc.Buffer.FirstElement = 0;
  uaViewDesc.Buffer.NumElements  = numItems;
  if (appendFlag)
  {
    uaViewDesc.Buffer.Flags = D3D11_BUFFER_UAV_FLAG_APPEND;
  }

  m_dxDevice->CreateUnorderedAccessView(m_buffer, &uaViewDesc, &m_viewRW);

  D3D11_SHADER_RESOURCE_VIEW_DESC viewDesc;
  ZeroMemory(&viewDesc, sizeof(viewDesc));

  viewDesc.Format = DXGI_FORMAT_UNKNOWN;
  viewDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
  viewDesc.Buffer.FirstElement = 0;
  viewDesc.Buffer.NumElements = numItems;

  m_dxDevice->CreateShaderResourceView(m_buffer, &viewDesc, &m_view);

  return;
}

bool CDxBuffRW::CreateBufferRead(void)
{
  D3D11_BUFFER_DESC desc = {};
  desc = {};
  desc.Usage               = D3D11_USAGE_STAGING;
  desc.ByteWidth           = m_numItems * m_sizeItem;
  desc.BindFlags           = 0;
  desc.CPUAccessFlags      = D3D11_CPU_ACCESS_READ;
  desc.StructureByteStride = m_sizeItem;

  if (FAILED(m_dxDevice->CreateBuffer(&desc, NULL, &m_bufferRead)))
  {
    return false;
  }

  return true;
}

//CDxBuffRW::CDxBuffAppend(void) :
//  m_buffer      (NULL),
//  m_bufferRead  (NULL),
//  m_viewRW      (NULL),
//  m_view        (NULL)
//{
//  return;
//}
//
//CDxBuffRW::~CDxBuffRW(void)
//{
//  Release();
//
//  return;
//}
//
//void CDxBuffRW::Release(void)
//{
//  SafeRelease(m_buffer);
//  SafeRelease(m_bufferRead);
//  SafeRelease(m_viewRW);
//  SafeRelease(m_view);
//
//  return;
//}

CDxTexture3D::CDxTexture3D(void) :
  m_dataMin     (0.0),
  m_dataMax     (0.0),
  m_texture     (NULL),
  m_textureRead (NULL),
  m_viewRW      (NULL),
  m_view        (NULL)
{
  return;
}

CDxTexture3D::~CDxTexture3D(void)
{
  Release();

  return;
}

void CDxTexture3D::Release(void)
{
  SafeRelease(m_texture);
  SafeRelease(m_textureRead);
  SafeRelease(m_viewRW);
  SafeRelease(m_view);

  return;
}

void CDxTexture3D::CreateTexture3D(int* dims, float* data, float* bounds) // if data != null make immutable
{
  D3D11_TEXTURE3D_DESC txtrDesc;
  ZeroMemory(&txtrDesc, sizeof(D3D11_TEXTURE3D_DESC));

  txtrDesc.Width     = dims[0];
  txtrDesc.Height    = dims[1];
  txtrDesc.Depth     = dims[2];
  txtrDesc.MipLevels = 1;
  txtrDesc.Format    = DXGI_FORMAT_R32_FLOAT;
  txtrDesc.Usage     = D3D11_USAGE_DEFAULT;
  txtrDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

  D3D11_SUBRESOURCE_DATA resData;

  resData.pSysMem          = data;
  resData.SysMemPitch      = dims[0] * sizeof(float);
  resData.SysMemSlicePitch = dims[1] * resData.SysMemPitch;

  if (FAILED(m_dxDevice->CreateTexture3D(&txtrDesc, &resData, &m_texture)))
  {
    return;
  }

  D3D11_SHADER_RESOURCE_VIEW_DESC viewDesc = {};

  viewDesc.Format              = DXGI_FORMAT_R32_FLOAT;
  viewDesc.ViewDimension       = D3D11_SRV_DIMENSION_TEXTURE3D;
  viewDesc.Texture3D.MipLevels = 1;

  if (FAILED(m_dxDevice->CreateShaderResourceView(m_texture, &viewDesc, &m_view)))
  {
    return;
  }

  if (bounds)
  {
    m_dataMin = bounds[0];
    m_dataMax = bounds[1];
  }

  return;
}

void CDxTexture3D::CreateRWTextureAll3D(int* dims, DXGI_FORMAT format)
{
  CreateRWTexture3D(dims, format);

  CreateTextureRead(dims, format);

  CreateResView(format);

  return;
}

void CDxTexture3D::CreateRWTextureDual3D(int* dims, DXGI_FORMAT format, DXGI_FORMAT format2)
{
  CreateRWTexture3D(dims, format);

  CreateTextureRead(dims, format2);

  CreateResView(format2);

  return;
}

void CDxTexture3D::CreateRWTexture3D(int* dims, DXGI_FORMAT format)
{
  D3D11_TEXTURE3D_DESC txtrDesc = {};

  txtrDesc.Width     = dims[0];
  txtrDesc.Height    = dims[1];
  txtrDesc.Depth     = dims[2];
  txtrDesc.MipLevels = 1;
  txtrDesc.Format    = DXGI_FORMAT_R32_TYPELESS;
  txtrDesc.Usage     = D3D11_USAGE_DEFAULT;
  txtrDesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;

  m_dxDevice->CreateTexture3D(&txtrDesc, NULL, &m_texture);

  D3D11_UNORDERED_ACCESS_VIEW_DESC  uavDesc = {};
  uavDesc.Format                = format;
  uavDesc.ViewDimension         = D3D11_UAV_DIMENSION_TEXTURE3D;
  uavDesc.Texture3D.MipSlice    = 0;
  uavDesc.Texture3D.FirstWSlice = 0;
  uavDesc.Texture3D.WSize       = dims[2];

  m_dxDevice->CreateUnorderedAccessView(m_texture, &uavDesc, &m_viewRW);

  return;
}

bool CDxTexture3D::CreateTextureRead(int* dims, DXGI_FORMAT format)
{
  D3D11_TEXTURE3D_DESC txtrDesc;
  ZeroMemory(&txtrDesc, sizeof(D3D11_TEXTURE3D_DESC));

  txtrDesc.Width          = dims[0];
  txtrDesc.Height         = dims[1];
  txtrDesc.Depth          = dims[2];
  txtrDesc.MipLevels      = 1;
  txtrDesc.Format         = format;
  txtrDesc.Usage          = D3D11_USAGE_STAGING;
  txtrDesc.BindFlags      = 0;
  txtrDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
  txtrDesc.MiscFlags      = 0;

  if (FAILED(m_dxDevice->CreateTexture3D(&txtrDesc, NULL, &m_textureRead)))
  {
    return false;
  }

  return true;
}

void CDxTexture3D::CreateResView(DXGI_FORMAT format)
{
  D3D11_SHADER_RESOURCE_VIEW_DESC viewDesc = {};
  viewDesc.Format              = format;
  viewDesc.ViewDimension       = D3D11_SRV_DIMENSION_TEXTURE3D;
  viewDesc.Texture3D.MipLevels = 1;

  m_dxDevice->CreateShaderResourceView(m_texture, &viewDesc, &m_view);

  return;
}

CDxTextureRead3D::CDxTextureRead3D(void) :
  m_textureRead   (NULL)
{
  return;
}

CDxTextureRead3D::~CDxTextureRead3D(void)
{
  Release();

  return;
}

void CDxTextureRead3D::Release(void)
{
  SafeRelease(m_textureRead);

  return;
}

bool CDxTextureRead3D::CreateTextureRead(int* dims, DXGI_FORMAT format)
{
  D3D11_TEXTURE3D_DESC txtrDesc;
  ZeroMemory(&txtrDesc, sizeof(D3D11_TEXTURE3D_DESC));

  txtrDesc.Width          = dims[0];
  txtrDesc.Height         = dims[1];
  txtrDesc.Depth          = dims[2];
  txtrDesc.MipLevels      = 1;
  txtrDesc.Format         = format;
  txtrDesc.Usage          = D3D11_USAGE_STAGING;
  txtrDesc.BindFlags      = 0;
  txtrDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
  txtrDesc.MiscFlags      = 0;

  if (FAILED(m_dxDevice->CreateTexture3D(&txtrDesc, NULL, &m_textureRead)))
  {
    return false;
  }

  return true;
}

template <typename T> void CDxTextureRead3D::FindMax(ID3D11Texture3D* inputTexture, T* stats)
{

  D3D11_TEXTURE3D_DESC desc;
  inputTexture->GetDesc(&desc);

  D3D11_MAPPED_SUBRESOURCE mapResource;

  m_dxContext->CopyResource(m_textureRead, inputTexture);
  m_dxContext->Map(m_textureRead, 0, D3D11_MAP_READ, 0, &mapResource);

  T* xEnd;
  T* yEnd;
  T* xPtr;
  T* yPtr;
  T* zPtr;
  T* zStart = (T*)mapResource.pData;
  T* zEnd   = zStart + desc.Depth * (mapResource.DepthPitch / sizeof(T));
  int zStep = mapResource.DepthPitch / sizeof(T);
  int yStep = mapResource.RowPitch / sizeof(T);
  T max = *zStart;
  T min = *zStart;
  T doseSum = 0.0;

  for (zPtr = zStart; zPtr < zEnd; zPtr += zStep)
  {
    yEnd = zPtr + desc.Height * yStep;

    for (yPtr = zPtr; yPtr < yEnd; yPtr += yStep)
    {
      xEnd = yPtr + desc.Width;

      for (xPtr = yPtr; xPtr < xEnd; ++xPtr)
      {
        doseSum += *xPtr;

        if (*xPtr < min)
        {
          min = *xPtr;
        }

        if (*xPtr > max)
        {
          max = *xPtr;
        }
      }
    }
  }

  m_dxContext->Unmap(m_textureReadResults.m_textureRead, 0);

  TRACE("DoseBounds %f %f\n", min, max);

  return;
}

CDxBuffRead::CDxBuffRead(void) :
  m_bufferRead   (NULL)
{
  return;
}

CDxBuffRead::~CDxBuffRead(void)
{
  Release();

  return;
}

void CDxBuffRead::Release(void)
{
  SafeRelease(m_bufferRead);

  return;
}

bool CDxBuffRead::CreateBufferRead(unsigned int numItems, unsigned int itemSize)
{
  D3D11_BUFFER_DESC desc = {};
  desc = {};
  desc.Usage               = D3D11_USAGE_STAGING;
  desc.ByteWidth           = numItems * itemSize;
  desc.BindFlags           = 0;
  desc.CPUAccessFlags      = D3D11_CPU_ACCESS_READ;
  desc.StructureByteStride = itemSize;

  if (FAILED(m_dxDevice->CreateBuffer(&desc, NULL, &m_bufferRead)))
  {
    return false;
  }

  return true;
}

CDxTextureSampler::CDxTextureSampler(void) :
  m_sampler (NULL)
{

}

CDxTextureSampler::~CDxTextureSampler(void)
{
  SafeRelease(m_sampler);

}

bool CDxTextureSampler::CreateSamplerFloat(void)
{
  // Create the sampler
  D3D11_SAMPLER_DESC sampDesc;
  ZeroMemory(&sampDesc, sizeof(sampDesc));
  sampDesc.Filter         = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
  sampDesc.AddressU       = D3D11_TEXTURE_ADDRESS_BORDER;
  sampDesc.AddressV       = D3D11_TEXTURE_ADDRESS_BORDER;
  sampDesc.AddressW       = D3D11_TEXTURE_ADDRESS_BORDER;
  sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
  sampDesc.MinLOD         = 0;
  sampDesc.MaxLOD         = D3D11_FLOAT32_MAX;
  memset(sampDesc.BorderColor, 0, 4 * sizeof(float));

  if (FAILED(m_dxDevice->CreateSamplerState(&sampDesc, &m_sampler)))
  {
    return false;
  }

  return true;
}

CDxTimer::CDxTimer(void)
{

  m_queryDisjoint[0]  = NULL;
  m_queryDisjoint[1]  = NULL;
  m_queryTimestamp[0] = NULL;
  m_queryTimestamp[1] = NULL;

  return;
}

CDxTimer::~CDxTimer(void)
{

  SafeRelease(m_queryDisjoint[0]);
  SafeRelease(m_queryDisjoint[1]);
  SafeRelease(m_queryTimestamp[0]);
  SafeRelease(m_queryTimestamp[1]);

  return;
}

bool CDxTimer::Build(void)
{

  D3D11_QUERY_DESC queryDesc0;
  queryDesc0.Query     = D3D11_QUERY_TIMESTAMP_DISJOINT;
  queryDesc0.MiscFlags = 0;

  D3D11_QUERY_DESC queryDesc1;
  queryDesc1.Query     = D3D11_QUERY_TIMESTAMP;
  queryDesc1.MiscFlags = 0;

  HRESULT hr;
  for (int index = 0; index < 2; ++index)
  {
    hr = m_dxDevice->CreateQuery(&queryDesc0, &m_queryDisjoint[index]);
    hr = m_dxDevice->CreateQuery(&queryDesc1, &m_queryTimestamp[index]);
  }

  return true;
}

void CDxTimer::Start(void)
{
  m_dxContext->Begin(m_queryDisjoint[0]);
  m_dxContext->End(m_queryTimestamp[0]);

  return;
}

float CDxTimer::End(void)
{
  float timeDelta;
  UINT64 timeStamps[2];

  m_dxContext->End(m_queryTimestamp[1]);
  m_dxContext->End(m_queryDisjoint[0]);

  while (m_dxContext->GetData(m_queryDisjoint[0], NULL, 0, 0) == S_FALSE)
  {
    Sleep(1);
  }

  D3D11_QUERY_DATA_TIMESTAMP_DISJOINT timestampDisjoint;
  m_dxContext->GetData(m_queryDisjoint[0], &timestampDisjoint, sizeof(D3D11_QUERY_DATA_TIMESTAMP_DISJOINT), 0);

  if (timestampDisjoint.Disjoint)
  {
    return 0.0;
  }

  HRESULT hr;
  for (int index = 0; index < 2; ++index)
  {
    hr = m_dxContext->GetData(m_queryTimestamp[index], &timeStamps[index], sizeof(UINT64), 0);
  }

  timeDelta = (timeStamps[1] - timeStamps[0]) / float(timestampDisjoint.Frequency);

  return timeDelta;
}


