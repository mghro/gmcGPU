#pragma once

#include <d3d11.h>
#include <D2d1.h>
#include <D2d1helper.h>
#include <DXGI1_2.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include "Heapster.h"
#include "UniArray.h"


#define GPU_VIS3D       (0x01)
#define GPU_VIS2D       (0x02)
#define GPU_VIS_3D2D    (GPU_VIS3D | GPU_VIS2D)

// GPU constants
#define NUM_DISPATCH_GROUPS   (D3D11_CS_DISPATCH_MAX_THREAD_GROUPS_PER_DIMENSION)
#define NUM_GROUP_THREADS     (D3D11_CS_THREAD_GROUP_MAX_THREADS_PER_GROUP)
#define NUM_DISPATCH_THREADS  (NUM_DISPATCH_GROUPS * NUM_GROUP_THREADS)

// USAGE
#define APPEND_BUFFER (1)

class CDirectX
{
public:

  CDirectX(void);
  ~CDirectX(void);

  bool Initialize(CString* gpuName);
  bool InitializeGraphics(UINT flags, CWnd* graphicsWnd);
  bool InitializeGPU(IDXGIAdapter* adapter);
  bool CalcMaxBufferSize(void);

private:

  IDXGIAdapter* SelectDevice(LPCWSTR deviceName);
  bool ListDevices(void);
  bool InitializeVis3D(CWnd* graphicsWnd);
  bool InitializeVis2D(void);

  ID3D11Device*           m_device;
  ID3D11DeviceContext*    m_context;
  IDXGISwapChain1*        m_swapChain;
  ID2D1RenderTarget*      m_renderTarget;
  ID3D11RenderTargetView* m_renderTargetView;

  UINT64                  m_numBufferBytesMax;

  CRect m_displayRect;

};

class CDxObj
{
public:

  CDxObj(void)  {};
  ~CDxObj(void) {};

  virtual void Release(void) {};

protected:

  static ID3D11Device*           m_dxDevice;
  static ID3D11DeviceContext*    m_dxContext;
  static IDXGISwapChain1*        m_dxSwapChain;
  static ID2D1RenderTarget*      m_dxRenderTarget;
  static ID3D11RenderTargetView* m_dxRenderTargetView;
  static CRect*                  m_dxDisplayRect;

  static ID3D11UnorderedAccessView* const m_nullViewRW;
  static ID3D11ShaderResourceView*  const m_nullView;

  static UINT64 m_dxNumBufferBytesMax;

  friend CDirectX;
};

class CDxBuffConstant : public CDxObj
{
public:

  CDxBuffConstant(void);
  ~CDxBuffConstant(void);

  void Release(void);

  void CreateConstantBufferDyn(unsigned int byteWidth, void* data = NULL);
  void CreateConstantBufferFixed(unsigned int byteWidth, void* data);
  void CreateVertexBuffer(unsigned int byteWidth, void* data, D3D11_USAGE usage = D3D11_USAGE_IMMUTABLE);
  void Upload(void* data);

  ID3D11Buffer* m_buffer;
  unsigned int  m_bufferSize;
};

class CDxBuffResource : public CDxObj
{
public:

  CDxBuffResource(void);
  ~CDxBuffResource(void);

  void Release(void);

  void CreateStructuredBufferFixed(unsigned int numItems, unsigned int itemSize, void* data);
  void CreateResourceBufferFixed(unsigned int numItems, unsigned int itemSize, void* data, 
    DXGI_FORMAT format = DXGI_FORMAT_R32_FLOAT);
  void CreateStructuredBufferDefault(unsigned int numItems, unsigned int itemSize, void* data = NULL);

  template <class T> void CreateStructuredBufferFixed(CUniArray<T>& array)
  {
    CreateStructuredBufferFixed(array.m_numItems, array.GetSizeItems(), array(0));
  }

  template <class T> void CreateResourceBufferFixed(CUniArray<T>& array, DXGI_FORMAT format = DXGI_FORMAT_R32_FLOAT)
  {
    CreateResourceBufferFixed(array.m_numItems, array.GetSizeItems(), array(0), format);
  }

  ID3D11Buffer*             m_buffer;
  ID3D11ShaderResourceView* m_view;
};


class CDxBuffRW : public CDxObj
{
public:

  CDxBuffRW(void);
  ~CDxBuffRW(void);

  void Release(void);

  void CreateBuffRWStruct(unsigned int numItems, unsigned int itemSize, void* data = NULL, bool appendFlag = false);
  void CreateBuffRWTyped(unsigned int numItems, DXGI_FORMAT format, void* data = NULL);
  bool CreateBufferRead(void);

  ID3D11Buffer* m_buffer;
  ID3D11Buffer* m_bufferRead;

  ID3D11UnorderedAccessView* m_viewRW;
  ID3D11ShaderResourceView*  m_view;

protected:

  int m_sizeItem;
  int m_numItems;

};

//class CDxBuffAppend : public CDxObj
//{
//public:
//
//  CDxBuffAppend(void);
//  ~CDxBuffAppend(void);
//
//  void Release(void);
//
//  void CreateBuffRWStruct(unsigned int numItems, unsigned int itemSize, void* data = NULL);
//  void CreateView(void);
//
//  ID3D11Buffer* m_buffer;
//  ID3D11Buffer* m_bufferRead;
//
//  ID3D11UnorderedAccessView* m_viewRW;
//
//private:
//
//  int m_sizeItem;
//  int m_numItems;
//};

class CDxTexture3D : public CDxObj
{
public:

  CDxTexture3D(void);
  ~CDxTexture3D(void);

  void Release(void);

  void CreateTexture3D(int* dims, float* data, float* bounds = NULL);
  void CreateRWTextureAll3D(int* dims, DXGI_FORMAT format);
  void CreateRWTextureDual3D(int* dims, DXGI_FORMAT format, DXGI_FORMAT format2);
  void CreateRWTexture3D(int* dims, DXGI_FORMAT format);
  void CreateResView(DXGI_FORMAT format);
  bool CreateTextureRead(int* dims, DXGI_FORMAT format);

  ID3D11Texture3D*            m_texture;
  ID3D11Texture3D*            m_textureRead;
  ID3D11ShaderResourceView*   m_view;
  ID3D11UnorderedAccessView*  m_viewRW;

  float m_dataMin;
  float m_dataMax;

};

class CDxTextureRead3D : public CDxObj
{
public:

  CDxTextureRead3D(void);
  ~CDxTextureRead3D(void);

  void Release(void);

  bool CreateTextureRead(int* dims, DXGI_FORMAT format);
  template <typename T> void FindMax(ID3D11Texture3D* inputTexture, T* stats);

  ID3D11Texture3D* m_textureRead;

};

class CDxBuffRead : public CDxObj
{
public:

  CDxBuffRead(void);
  ~CDxBuffRead(void);

  void Release(void);

  bool CreateBufferRead(unsigned int numItems, unsigned int itemSize);

  ID3D11Buffer* m_bufferRead;

};

class CDxTextureSampler : public CDxObj
{
public:

  CDxTextureSampler(void);
  ~CDxTextureSampler(void);

  bool CreateSamplerFloat(void);

  ID3D11SamplerState* m_sampler;

};

class CDxShader : public CDxObj
{
public:

  CDxShader(void);
  ~CDxShader(void);

  virtual bool BuildShader(const BYTE* shaderBody, size_t shaderSize) = 0;

  void SetConstantBuffers(ID3D11Buffer** buffers, size_t numBuffers);
  void SetResourceViews(ID3D11ShaderResourceView** views, size_t numViews);
  void SetUAVViews(ID3D11UnorderedAccessView** views, size_t numViews);
  void SetSamplers(ID3D11SamplerState** samplers, size_t numSamplers);

  virtual void LoadResources(void) = 0;

protected:

  ID3D11Buffer**              m_constantBuffers;
  ID3D11ShaderResourceView**  m_bufferViews;
  ID3D11UnorderedAccessView** m_uavViews;
  ID3D11SamplerState**        m_textureSamplers;

  int m_numConstantBuffers;
  int m_numBufferViews;
  int m_numUAVViews;
  int m_numSamplers;
};

class CDxShaderCompute : public CDxShader
{
public:

  CDxShaderCompute(void);
  ~CDxShaderCompute(void);

  bool BuildShader(const BYTE* shaderBody, size_t shaderSize);

  void LoadResources(void);

  void LoadUAView(ID3D11UnorderedAccessView* view, int viewIndex)
  {
    m_dxContext->CSSetUnorderedAccessViews(viewIndex, 1, &view, NULL);
  }

  ID3D11ComputeShader* m_computeShader;

};

class CDxShaderVertex : public CDxShader
{
public:

  CDxShaderVertex(void);
  ~CDxShaderVertex(void);

  bool BuildShader(const BYTE* shaderBody, size_t shaderSize);
  void LoadResources(void);

  ID3D11VertexShader* m_vertexShader;

};

class CDxShaderPixel : public CDxShader
{
public:

  CDxShaderPixel(void);
  ~CDxShaderPixel(void);

  bool BuildShader(const BYTE* shaderBody, size_t shaderSize);
  void LoadResources(void);

  ID3D11PixelShader* m_pixelShader;

};

class CDxTimer : public CDxObj
{
public:

  CDxTimer(void);
  ~CDxTimer(void);

  bool  Build(void);
  void  Start(void);
  float End(void);

protected:

  ID3D11Query* m_queryDisjoint[2];
  ID3D11Query* m_queryTimestamp[2];

};