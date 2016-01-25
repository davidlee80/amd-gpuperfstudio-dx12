//==============================================================================
// Copyright (c) 2015 Advanced Micro Devices, Inc. All rights reserved.
/// \author AMD Developer Tools Team
/// \file
/// \brief  The location where hand-written custom implementations of DX12 wrapper functions live.
//==============================================================================

#include "DX12CustomWrappers.h"
#include "../DX12TraceAnalyzerLayer.h"
#include "../Interception/DX12Interceptor.h"
#include "../DX12LayerManager.h"
#include "DX12Defines.h"

HRESULT STDMETHODCALLTYPE GPS_ID3D12DeviceCustom::CreateCommandList(UINT nodeMask, D3D12_COMMAND_LIST_TYPE type, ID3D12CommandAllocator* pCommandAllocator, ID3D12PipelineState* pInitialState, REFIID riid, void** ppCommandList)
{
    HRESULT result = GPS_ID3D12Device::CreateCommandList(nodeMask, type, pCommandAllocator, pInitialState, riid, ppCommandList);

#if USE_GPA_PROFILING
    if (S_OK == result)
    {
        GPS_ID3D12GraphicsCommandListCustom* pwrappedCommandList = (GPS_ID3D12GraphicsCommandListCustom*)*ppCommandList;
        DX12Interceptor* dx12Interceptor = static_cast<DX12Interceptor*>(DX12TraceAnalyzerLayer::Instance()->GetInterceptor());
        dx12Interceptor->LockGPA();
        DX12TraceAnalyzerLayer::Instance()->OpenAndInitializeGPAContext(pwrappedCommandList->mRealGraphicsCommandList);
        dx12Interceptor->UnlockGPA();
    }
#endif

    return result;
}

//--------------------------------------------------------------------------
/// A custom implementation of ID3D12Device::Release. Neccessary to release close GPA context
/// \returns D3D12 device reference count
//--------------------------------------------------------------------------


ULONG STDMETHODCALLTYPE GPS_ID3D12GraphicsCommandListCustom::Release()
{
    ULONG result = GPS_ID3D12GraphicsCommandList::Release();

#if USE_GPA_PROFILING
    if (0 == result)
    {
        ID3D12CommandList* pUnwrappedCommandList = this->mRealGraphicsCommandList;
        DX12Interceptor* dx12Interceptor = static_cast<DX12Interceptor*>(DX12TraceAnalyzerLayer::Instance()->GetInterceptor());
        GPUPerfAPILoader& gpaLoader = dx12Interceptor->GetGPALoader();
        Log(logWARNING, "Closing GPA context '0x%p'\n", pUnwrappedCommandList);
        gpaLoader.GPA_SelectContext(pUnwrappedCommandList);
        gpaLoader.GPA_CloseContext();
    }
#endif

    return result;
}

GPS_ID3D12CommandQueueCustom::GPS_ID3D12CommandQueueCustom(ID3D12CommandQueue* inRealCommandQueue) : GPS_ID3D12CommandQueue(inRealCommandQueue)
#if USE_GPA_PROFILING
    , m_pFence(nullptr)
    , m_nextFenceValue(1)
    , m_lastCompletedFenceValue(0)
    , m_fenceEvent(0)
#endif
{
#if USE_GPA_PROFILING
    ID3D12Device* pDevice = nullptr;
    HRESULT result = GetDevice(__uuidof(*pDevice), reinterpret_cast<void**>(&pDevice));
    if (S_OK == result)
    {
        result = pDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_pFence));

        if (S_OK == result)
        {
            m_pFence->SetName(L"GPS_ID3D12CommandQueueCustom::m_pFence");
            m_pFence->Signal(0);

            m_fenceEvent = CreateEvent(nullptr, false, false, nullptr);
        }
    }
#endif
}

GPS_ID3D12CommandQueueCustom::~GPS_ID3D12CommandQueueCustom()
{
#if USE_GPA_PROFILING
    CloseHandle(m_fenceEvent);
    m_pFence->Release();
#endif
}

#if USE_GPA_PROFILING
//--------------------------------------------------------------------------
/// Check whether the specified fence is complete.
/// \param fenceValue The fence to check
/// \return true if the fence is complete
//--------------------------------------------------------------------------
bool GPS_ID3D12CommandQueueCustom::IsFenceComplete(UINT64 fenceValue)
{
    if (fenceValue > m_lastCompletedFenceValue)
    {
        UINT64 lastCompletedValue = m_pFence->GetCompletedValue();
        m_lastCompletedFenceValue = (m_lastCompletedFenceValue > lastCompletedValue) ? m_lastCompletedFenceValue : lastCompletedValue;
    }

    return fenceValue <= m_lastCompletedFenceValue;
}

//--------------------------------------------------------------------------
/// Waits for the specified fence to complete.
/// \param fenceValue The fence to check
//--------------------------------------------------------------------------
void GPS_ID3D12CommandQueueCustom::WaitForFence(UINT64 fenceValue)
{
    if (IsFenceComplete(fenceValue) == false)
    {
        m_pFence->SetEventOnCompletion(fenceValue, m_fenceEvent);
        WaitForSingleObject(m_fenceEvent, INFINITE);
        m_lastCompletedFenceValue = fenceValue;
    }
}
#endif

//--------------------------------------------------------------------------
/// A custom override used to track which Queue is responsible for executing CommandLists.
/// \param NumCommandLists The number of CommandLists in the array to execute.
/// \param ppCommandLists An array of CommandLists to execute through the Queue.
//--------------------------------------------------------------------------
void STDMETHODCALLTYPE GPS_ID3D12CommandQueueCustom::ExecuteCommandLists(UINT NumCommandLists, ID3D12CommandList* const* ppCommandLists)
{
    GPS_ID3D12CommandQueue::ExecuteCommandLists(NumCommandLists, ppCommandLists);

    DX12Interceptor* interceptor = GetDX12LayerManager()->GetInterceptor();

#if USE_GPA_PROFILING
    if (interceptor->ShouldCollectTrace() && interceptor->ShouldCollectGPUTime())
    {
        // Signal the next fence value (with the GPU)
        Signal(m_pFence, m_nextFenceValue);

        WaitForFence(m_nextFenceValue++);

        DX12TraceAnalyzerLayer::Instance()->GetProfileResultsForCmdLists(NumCommandLists, ppCommandLists);
    }
#endif

#if USE_GPA_PROFILING == 0
    if (interceptor->ShouldCollectTrace() && interceptor->ShouldCollectGPUTime())
    {
        interceptor->GatherProfilerResults(this, NumCommandLists, ppCommandLists);
    }
#endif

    DX12TraceAnalyzerLayer* traceAnalyzer = DX12TraceAnalyzerLayer::Instance();

    if (traceAnalyzer != NULL)
    {
        // The incoming array of CommandLists is tracked order to know which Queue executed which CommandLists.
        traceAnalyzer->TrackParentCommandQueue(this, NumCommandLists, reinterpret_cast<GPS_ID3D12GraphicsCommandList* const*>(ppCommandLists));
    }
}
