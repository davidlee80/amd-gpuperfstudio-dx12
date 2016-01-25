//==============================================================================
// Copyright (c) 2015 Advanced Micro Devices, Inc. All rights reserved.
/// \author AMD Developer Tools Team
/// \file   DX12ImageRenderer.cpp
/// \brief  Header file for DX12ImageRenderer.
///         This class helps to render D3D12 resources into RGBA8 CPU buffers.
//==============================================================================

#include <d3d12.h>
#include <vector>

#ifndef __DX12_IMAGE_RENDERER_H__
#define __DX12_IMAGE_RENDERER_H__

#define OVERWRITE_SRC_RES 0

#define SAFE_DX_RELEASE(s) if (s) { s->Release(); s = nullptr; }

enum RootParameters : UINT32
{
    RootParameterSRV,
    RootParameterUAV,
    RootParameterCBV,
    RootParametersCount
};

struct CpuImage
{
    void* pData;
    UINT  pitch;
    UINT  width;
    UINT  height;
};

static const float ClearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
static const UINT BytesPerPixel = 4;
static const UINT ConstBufSize = 0x10000;

struct ConstantBuffer
{
    UINT rtWidth;
    UINT flipX;
    UINT flipY;
};

struct DX12ImageRendererConfig
{
    ID3D12Device*       pDevice;
    ID3D12CommandQueue* pCmdQueue;
};

class DX12ImageRenderer
{
public:
    static bool CpuImageToPng(
        CpuImage*          pImage,
        UCHAR**            ppPngMem,
        UINT*              pMemSize,
        const std::string& fileName = ""); // Optional

    static DX12ImageRenderer* Create(const DX12ImageRendererConfig& config);

    ~DX12ImageRenderer();

    HRESULT CaptureImage(
        ID3D12Resource*       pRes,
        D3D12_RESOURCE_STATES prevState,
        UINT                  newWidth,
        UINT                  newHeight,
        CpuImage*             pImgOut,
        bool                  bFlipX,
        bool                  bFlipY);

private:
    DX12ImageRenderer();
    HRESULT Init(const DX12ImageRendererConfig& config);

    HRESULT CreateCaptureAssets(
        ID3D12Resource* pRes,
        UINT            newWidth,
        UINT            newHeight);

    void FreeCaptureAssets();
    void WaitCmdListFinish();

    DX12ImageRendererConfig m_config;

    // Command list stuff
    ID3D12CommandAllocator*    m_pCmdAllocator;
    ID3D12GraphicsCommandList* m_pCmdList;

    // Pipeline state
    ID3D12RootSignature* m_pRootSignatureGraphics;
    ID3D12PipelineState* m_pPipelineStateGraphics;

    // Resource descriptor stuff
    ID3D12DescriptorHeap* m_pSrvUavCbHeap;
    UINT                  m_srvUavCbDescriptorSize;

    // Synchronization objects
    ID3D12Fence* m_pFence;
    HANDLE       m_fenceEvent;
    UINT64       m_fenceValue;

    // UAV
    ID3D12Resource* m_pPSWriteBuf;
    ID3D12Resource* m_pPSWriteBufReadBack;

    // Const buffer
    ID3D12Resource* m_pConstantBuffer;
    ConstantBuffer  m_constantBufferData;
    UINT8*          m_pCbvDataBegin;

    // Internal RTs
    ID3D12Resource*       m_pInternalRT;
    ID3D12DescriptorHeap* m_pInternalRtvHeap;
};

#endif