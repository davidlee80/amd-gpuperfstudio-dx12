//==============================================================================
// Copyright (c) 2015 Advanced Micro Devices, Inc. All rights reserved.
/// \author AMD Developer Tools Team
/// \file   DX12CmdListProfiler.cpp
/// \brief  DX12 command list profiler.
///         This standalone class injects queries into app command buffers
///         to determine GPU time and counter information.
//==============================================================================

#include "DX12CmdListProfiler.h"

/**
***************************************************************************************************
*   DX12CmdListProfiler::Create
*
*   @brief
*       Static method that instantiates a DX12 profiler.
*
*   @return
*       Pointer to the instantiated profiler. Return nullptr if invalid params were passed in.
***************************************************************************************************
*/
DX12CmdListProfiler* DX12CmdListProfiler::Create(
    ID3D12Device*                    pDevice,    ///< [in] Handle to the device
    const DX12CmdListProfilerConfig* pConfig)   ///< [in] Pointer to profiler configuration
{
    DX12CmdListProfiler* pOut = new DX12CmdListProfiler(pDevice);

    if (pOut != nullptr)
    {
        if (pOut->Init(pDevice, pConfig) != S_OK)
        {
            delete pOut;
            pOut = nullptr;
        }
    }

    return pOut;
}

/**
***************************************************************************************************
*   DX12CmdListProfiler::DX12CmdListProfiler
*
*   @brief
*       Constructor.
***************************************************************************************************
*/
DX12CmdListProfiler::DX12CmdListProfiler(ID3D12Device* pDevice)
    :
    m_pDevice(pDevice),
    m_nextFenceValue(1),
    m_lastCompletedFenceValue(0)
{
}

/**
***************************************************************************************************
*   DX12CmdListProfiler::Init
*
*   @brief
*       Perform all profiler initialization.
*
*   @return
*       S_OK if everything was init correctly.
***************************************************************************************************
*/
HRESULT DX12CmdListProfiler::Init(
    ID3D12Device*                    pDevice,   ///< [in] Handle to the device
    const DX12CmdListProfilerConfig* pConfig)   ///< [in] Pointer to profiler configuration
{
    HRESULT result = E_FAIL;

    if ((pDevice != nullptr) && (pConfig != nullptr))
    {
        if (pConfig->measurementsPerGroup > 0)
        {
            result = m_pDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_pFence));

            if (result == S_OK)
            {
                m_pFence->SetName(L"DX12CmdListProfiler::m_pFence");
                m_pFence->Signal(0);

                m_fenceEvent = CreateEvent(nullptr, false, false, nullptr);

                if (m_fenceEvent != INVALID_HANDLE_VALUE)
                {
                    result = pDevice->SetStablePowerState(true);

                    if (result == S_OK)
                    {
                        memcpy(&m_config, pConfig, sizeof(DX12CmdListProfilerConfig));
                    }
                }
            }
        }
    }

    return result;
}

/**
***************************************************************************************************
*   DX12CmdListProfiler::~DX12CmdListProfiler
*
*   @brief
*       Destructor.
***************************************************************************************************
*/
DX12CmdListProfiler::~DX12CmdListProfiler()
{
    DX12ProfilerCriticalSection lock(m_cs);

    for (CmdListDataMap::iterator iterator = m_cmdListMap.begin();
         iterator != m_cmdListMap.end();
         iterator++)
    {
        HRESULT result = E_FAIL;

        result = ReleaseProfilerData(iterator->second);

        PROFILER_ASSERT(result == S_OK);
    }

    CloseHandle(m_fenceEvent);

    m_pFence->Release();
}

/**
***************************************************************************************************
*   DX12CmdListProfiler::BeginGrCmdMeasurement
*
*   @brief
*       Begin profiling a grCmd* function.
***************************************************************************************************
*/
ProfilerResultCode DX12CmdListProfiler::BeginCmdMeasurement(
    ID3D12GraphicsCommandList*   pCmdList,  ///< [in] Handle to cmd buf being measured
    const ProfilerMeasurementId* pIdInfo)   ///< [in] Pointer to measurement id data
{
    DX12ProfilerCriticalSection lock(m_cs);

    PROFILER_ASSERT(pCmdList != nullptr);
    PROFILER_ASSERT(pIdInfo != nullptr);

    ProfilerResultCode profilerResultCode = PROFILER_FAIL;

    // If command list has not yet been measured, allocate memory objects for it
    if (IsCmdListBeingMeasured(pCmdList) == false)
    {
        ProfilerCmdListData newCmdListData = {};
        m_cmdListMap[pCmdList] = newCmdListData;
    }

    ProfilerCmdListData& cmdListData = m_cmdListMap[pCmdList];

    PROFILER_ASSERT(cmdListData.state != PROFILER_STATE_STARTED);

    const UINT measurementId = cmdListData.cmdListMeasurementCount % m_config.measurementsPerGroup;

    // Create new measurement group if full
    if (measurementId == 0)
    {
        HRESULT result = E_FAIL;
        result = SetupNewMeasurementGroup(cmdListData);
        PROFILER_ASSERT(result == S_OK);
    }

    if (m_config.measurementTypeFlags & PROFILER_MEASUREMENT_TYPE_PIPE_STATS)
    {
        // Begin pipe stats query
        pCmdList->BeginQuery(cmdListData.pActiveMeasurementGroup->pPipeStatsQueryHeap, D3D12_QUERY_TYPE_PIPELINE_STATISTICS, measurementId);
    }

    if (m_config.measurementTypeFlags & PROFILER_MEASUREMENT_TYPE_TIMESTAMPS)
    {
        const UINT offset = measurementId * 2;

        // Inject timestamp
        pCmdList->EndQuery(cmdListData.pActiveMeasurementGroup->pTimestampQueryHeap, D3D12_QUERY_TYPE_TIMESTAMP, offset);
    }

    cmdListData.cmdListMeasurementCount++;

    // Add a new measurement
    ProfilerMeasurementInfo clientData = {};
    clientData.measurementNum = cmdListData.cmdListMeasurementCount;
    if (pIdInfo != nullptr)
    {
        memcpy(&clientData.idInfo, pIdInfo, sizeof(ProfilerMeasurementId));
    }
    cmdListData.pActiveMeasurementGroup->measurementInfos.push_back(clientData);

    cmdListData.pActiveMeasurementGroup->groupMeasurementCount++;

    cmdListData.state = PROFILER_STATE_STARTED;

    profilerResultCode = PROFILER_SUCCESS;

    return profilerResultCode;
}

/**
***************************************************************************************************
*   DX12CmdListProfiler::EndGrCmdMeasurement
*
*   @brief
*       End profiling a grCmd* function.
***************************************************************************************************
*/
ProfilerResultCode DX12CmdListProfiler::EndCmdMeasurement(
    ID3D12GraphicsCommandList* pCmdList) ///< [in] Handle to cmd buf being measured
{
    DX12ProfilerCriticalSection lock(m_cs);

    PROFILER_ASSERT(pCmdList != nullptr);

    ProfilerResultCode profilerResultCode = PROFILER_FAIL;

    if (IsCmdListBeingMeasured(pCmdList) == true)
    {
        ProfilerCmdListData& cmdListData = m_cmdListMap[pCmdList];

        PROFILER_ASSERT(cmdListData.state == PROFILER_STATE_STARTED);

        if (cmdListData.state == PROFILER_STATE_STARTED)
        {
            const UINT measurementId = (cmdListData.cmdListMeasurementCount - 1) % m_config.measurementsPerGroup;

            // Inject timestamp
            if (m_config.measurementTypeFlags & PROFILER_MEASUREMENT_TYPE_TIMESTAMPS)
            {
                const UINT offset = (measurementId * 2) + 1;

                pCmdList->EndQuery(cmdListData.pActiveMeasurementGroup->pTimestampQueryHeap, D3D12_QUERY_TYPE_TIMESTAMP, offset);

                pCmdList->ResolveQueryData(
                    cmdListData.pActiveMeasurementGroup->pTimestampQueryHeap,
                    D3D12_QUERY_TYPE_TIMESTAMP,
                    (offset-1),
                    2,
                    cmdListData.pActiveMeasurementGroup->pTimestampBuffer,
                    (offset-1) * sizeof(UINT64));
            }

            // End pipe stats query
            if (m_config.measurementTypeFlags & PROFILER_MEASUREMENT_TYPE_PIPE_STATS)
            {
                pCmdList->EndQuery(cmdListData.pActiveMeasurementGroup->pPipeStatsQueryHeap, D3D12_QUERY_TYPE_PIPELINE_STATISTICS, measurementId);

                pCmdList->ResolveQueryData(
                    cmdListData.pActiveMeasurementGroup->pPipeStatsQueryHeap,
                    D3D12_QUERY_TYPE_PIPELINE_STATISTICS,
                    measurementId,
                    1,
                    cmdListData.pActiveMeasurementGroup->pPipeStatsBuffer,
                    measurementId * sizeof(D3D12_QUERY_DATA_PIPELINE_STATISTICS));
            }

            cmdListData.state = PROFILER_STATE_ENDED;

            profilerResultCode = PROFILER_SUCCESS;
        }
        else
        {
            profilerResultCode = PROFILER_MEASUREMENT_NOT_STARTED;
        }
    }
    else
    {
        profilerResultCode = PROFILER_THIS_CMD_LIST_WAS_NOT_MEASURED;
    }

    return profilerResultCode;
}

/**
***************************************************************************************************
*   DX12CmdListProfiler::GetCmdListResults
*
*   @brief
*       We assume this will be called immediately after a command list has been submitted.
*
*   @return
*       A vector containing performance information for a given function.
***************************************************************************************************
*/
ProfilerResultCode DX12CmdListProfiler::GetCmdListResults(
    ID3D12GraphicsCommandList*   pCmdList,       ///< [in]  Handle to cmd buf being measured
    ID3D12CommandQueue*          pCmdQueue,      ///< [in]  Handle to cmd queue
    std::vector<ProfilerResult>& results)        ///< [out] Vector with profiler results
{
    DX12ProfilerCriticalSection lock(m_cs);

    PROFILER_ASSERT(pCmdList != nullptr);
    PROFILER_ASSERT(pCmdQueue != nullptr);

    // Signal the next fence value (with the GPU)
    pCmdQueue->Signal(m_pFence, m_nextFenceValue);

    WaitForFence(m_nextFenceValue++);

    ProfilerResultCode profilerResultCode = PROFILER_SUCCESS;

    HRESULT result = E_FAIL;

    if (IsCmdListBeingMeasured(pCmdList) == true)
    {
        ProfilerCmdListData& cmdListData = m_cmdListMap[pCmdList];

        const UINT64 queueFrequency = GetQueueFrequency(pCmdQueue);

        // Loop through all measurements for this command list
        for (UINT i = 0; i < cmdListData.measurementGroups.size(); i++)
        {
            ProfilerMeasurementGroup& currGroup = cmdListData.measurementGroups[i];

            ProfilerInterval* pTimestampData = nullptr;
            D3D12_QUERY_DATA_PIPELINE_STATISTICS* pPipelineStatsData = nullptr;

            if (m_config.measurementTypeFlags & PROFILER_MEASUREMENT_TYPE_TIMESTAMPS)
            {
                D3D12_RANGE mapRange = {};
                mapRange.Begin = 0;
                mapRange.End   = m_config.measurementsPerGroup * sizeof(ProfilerInterval);
                result = currGroup.pTimestampBuffer->Map(0, &mapRange, reinterpret_cast<void**>(&pTimestampData));
            }

            if (m_config.measurementTypeFlags & PROFILER_MEASUREMENT_TYPE_PIPE_STATS)
            {
                D3D12_RANGE mapRange = {};
                mapRange.Begin = 0;
                mapRange.End   = m_config.measurementsPerGroup * sizeof(D3D12_QUERY_DATA_PIPELINE_STATISTICS);
                result = currGroup.pPipeStatsBuffer->Map(0, &mapRange, reinterpret_cast<void**>(&pPipelineStatsData));
            }

            // Report no results
            if (m_config.measurementTypeFlags == PROFILER_MEASUREMENT_TYPE_NONE)
            {
                for (UINT j = 0; j < currGroup.groupMeasurementCount; j++)
                {
                    ProfilerResult profilerResult = {};
                    results.push_back(profilerResult);
                }
            }

            // Fetch our results
            else
            {
                for (UINT j = 0; j < currGroup.groupMeasurementCount; j++)
                {
                    ProfilerResult profilerResult = {};

                    memcpy(&profilerResult.measurementInfo, &currGroup.measurementInfos[j], sizeof(ProfilerMeasurementInfo));

                    if (pTimestampData != nullptr)
                    {
                        UINT64* pTimerBegin = &pTimestampData[j].start;
                        UINT64* pTimerEnd = &pTimestampData[j].end;
                        UINT64 baseClock = pTimestampData[0].start;

                        // Make sure the reported clock values aren't zero
                        PROFILER_ASSERT((*pTimerBegin != 0) && (*pTimerEnd != 0));

                        // Store raw clocks
                        profilerResult.timestampResult.rawClocks.start = *pTimerBegin;
                        profilerResult.timestampResult.rawClocks.end = *pTimerEnd;

                        // Calculate adjusted clocks
                        profilerResult.timestampResult.adjustedClocks.start = *pTimerBegin - baseClock;
                        profilerResult.timestampResult.adjustedClocks.end = *pTimerEnd - baseClock;

                        // Calculate exec time
                        profilerResult.timestampResult.execMicroSecs = static_cast<double>(*pTimerEnd - *pTimerBegin) / queueFrequency;
                        profilerResult.timestampResult.execMicroSecs *= 1000000;
                    }

                    if (pPipelineStatsData != nullptr)
                    {
                        memcpy(&profilerResult.pipeStatsResult, pPipelineStatsData, sizeof(D3D12_QUERY_DATA_PIPELINE_STATISTICS));
                    }

                    results.push_back(profilerResult);
                }

                D3D12_RANGE unmapRange = {};

                if (pPipelineStatsData != nullptr)
                {
                    currGroup.pPipeStatsBuffer->Unmap(0, &unmapRange);
                }

                if (pTimestampData != nullptr)
                {
                    currGroup.pTimestampBuffer->Unmap(0, &unmapRange);
                }
            }
        }

        // We're done profiling this command list, so free up used resources
        result = ReleaseProfilerData(cmdListData);
        PROFILER_ASSERT(result == S_OK);

        m_cmdListMap.erase(pCmdList);
    }
    else
    {
        profilerResultCode = PROFILER_THIS_CMD_LIST_WAS_NOT_MEASURED;
    }

    return profilerResultCode;
}

/**
***************************************************************************************************
*   DX12CmdListProfiler::GetQueueFrequency
*
*   @brief
*       Get timestamp frequency for a given queue.
*
*   @return
*       Queue frequency in a UINT64
***************************************************************************************************
*/
UINT64 DX12CmdListProfiler::GetQueueFrequency(ID3D12CommandQueue* pCmdQueue)
{
    UINT64 freq = 0;

    if (m_queueFrequencies.count(pCmdQueue) == 0)
    {
        pCmdQueue->GetTimestampFrequency(&freq);
        m_queueFrequencies[pCmdQueue] = freq;
    }
    else
    {
        freq = m_queueFrequencies[pCmdQueue];
    }

    return freq;
}

/**
***************************************************************************************************
*   DX12CmdListProfiler::CreateQueryBuffer
*
*   @brief
*       Create a buffer to hold query results.
*
*   @return
*       S_OK if successful.
***************************************************************************************************
*/
HRESULT DX12CmdListProfiler::CreateQueryBuffer(ID3D12Resource** pResource, UINT size)
{
    HRESULT result = E_FAIL;

    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type                 = D3D12_HEAP_TYPE_READBACK;
    heapProps.CPUPageProperty      = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    heapProps.CreationNodeMask     = 1;
    heapProps.VisibleNodeMask      = 1;

    D3D12_RESOURCE_DESC bufferDesc = {};
    bufferDesc.Dimension          = D3D12_RESOURCE_DIMENSION_BUFFER;
    bufferDesc.Alignment          = 0;
    bufferDesc.Width              = size;
    bufferDesc.Height             = 1;
    bufferDesc.DepthOrArraySize   = 1;
    bufferDesc.MipLevels          = 1;
    bufferDesc.Format             = DXGI_FORMAT_UNKNOWN;
    bufferDesc.SampleDesc.Count   = 1;
    bufferDesc.SampleDesc.Quality = 0;
    bufferDesc.Layout             = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    bufferDesc.Flags              = D3D12_RESOURCE_FLAG_NONE;

    result = m_pDevice->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &bufferDesc,
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr,
        IID_PPV_ARGS(&(*pResource)));

    return result;
}

/**
***************************************************************************************************
*   DX12CmdListProfiler::SetupNewMeasurementGroup
*
*   @brief
*       Create a new query heap and memory pair for time stamping.
*
*   @return
*       S_OK if successful
***************************************************************************************************
*/
HRESULT DX12CmdListProfiler::SetupNewMeasurementGroup(ProfilerCmdListData& cmdListData)
{
    HRESULT result = S_OK;

    ProfilerMeasurementGroup measurementGroup = {};

    if (m_config.measurementTypeFlags & PROFILER_MEASUREMENT_TYPE_TIMESTAMPS)
    {
        result = CreateQueryBuffer(&measurementGroup.pTimestampBuffer,
                                   m_config.measurementsPerGroup * sizeof(ProfilerInterval));

        if (result == S_OK)
        {
            measurementGroup.pTimestampBuffer->SetName(L"Timestamp Buffer");

            D3D12_QUERY_HEAP_DESC queryHeapDesc;
            queryHeapDesc.Count    = m_config.measurementsPerGroup * 2;
            queryHeapDesc.NodeMask = 1;
            queryHeapDesc.Type     = D3D12_QUERY_HEAP_TYPE_TIMESTAMP;
            result = m_pDevice->CreateQueryHeap(&queryHeapDesc, IID_PPV_ARGS(&measurementGroup.pTimestampQueryHeap));

            if (result == S_OK)
            {
                measurementGroup.pTimestampQueryHeap->SetName(L"Timestamp QueryHeap");
            }
        }
    }

    if (m_config.measurementTypeFlags & PROFILER_MEASUREMENT_TYPE_PIPE_STATS)
    {
        result = CreateQueryBuffer(&measurementGroup.pPipeStatsBuffer,
                                   m_config.measurementsPerGroup * sizeof(D3D12_QUERY_DATA_PIPELINE_STATISTICS));

        if (result == S_OK)
        {
            measurementGroup.pPipeStatsBuffer->SetName(L"PipeStats Buffer");

            D3D12_QUERY_HEAP_DESC queryHeapDesc;
            queryHeapDesc.Count    = m_config.measurementsPerGroup;
            queryHeapDesc.NodeMask = 1;
            queryHeapDesc.Type     = D3D12_QUERY_HEAP_TYPE_PIPELINE_STATISTICS;
            result = m_pDevice->CreateQueryHeap(&queryHeapDesc, IID_PPV_ARGS(&measurementGroup.pPipeStatsQueryHeap));

            if (result == S_OK)
            {
                measurementGroup.pPipeStatsQueryHeap->SetName(L"PipeStats QueryHeap");
            }
        }
    }

    if (result == S_OK)
    {
        cmdListData.measurementGroups.push_back(measurementGroup);
        cmdListData.pActiveMeasurementGroup = &cmdListData.measurementGroups.back();
    }

    return result;
}

/**
***************************************************************************************************
*   DX12CmdListProfiler::ReleaseProfilerData
*
*   @brief
*       Destroy DX12 objects and memory created by the profiler.
***************************************************************************************************
*/
HRESULT DX12CmdListProfiler::ReleaseProfilerData(
    ProfilerCmdListData& data) ///< [in/out] Reference to struct containing to-be-freed objects
{
    UINT successfulFrees = 0;
    UINT totalFrees = 0;

    for (UINT i = 0; i < data.measurementGroups.size(); i++)
    {
        if (data.measurementGroups[i].pTimestampQueryHeap != nullptr)
        {
            totalFrees++;
            if (data.measurementGroups[i].pTimestampQueryHeap->Release() == S_OK)
            {
                data.measurementGroups[i].pTimestampQueryHeap = nullptr;
                successfulFrees++;
            }
        }

        if (data.measurementGroups[i].pTimestampBuffer != nullptr)
        {
            totalFrees++;
            if (data.measurementGroups[i].pTimestampBuffer->Release() == S_OK)
            {
                data.measurementGroups[i].pTimestampBuffer = nullptr;
                successfulFrees++;
            }
        }

        if (data.measurementGroups[i].pPipeStatsQueryHeap != nullptr)
        {
            totalFrees++;
            if (data.measurementGroups[i].pPipeStatsQueryHeap->Release() == S_OK)
            {
                data.measurementGroups[i].pPipeStatsQueryHeap = nullptr;
                successfulFrees++;
            }
        }

        if (data.measurementGroups[i].pPipeStatsBuffer != nullptr)
        {
            totalFrees++;
            if (data.measurementGroups[i].pPipeStatsBuffer->Release() == S_OK)
            {
                data.measurementGroups[i].pPipeStatsBuffer = nullptr;
                successfulFrees++;
            }
        }
    }

    data.measurementGroups.clear();

    data.pActiveMeasurementGroup = nullptr;

    return (totalFrees == successfulFrees) ? S_OK : E_FAIL;
}

/**
***************************************************************************************************
*   DX12CmdListProfiler::IsFenceComplete
*
*   @brief
*       Determine if a fence has come back.
*
*   @return
*       True if fence is complete.
***************************************************************************************************
*/
bool DX12CmdListProfiler::IsFenceComplete(UINT64 fenceValue)
{
    if (fenceValue > m_lastCompletedFenceValue)
    {
        m_lastCompletedFenceValue = Max(m_lastCompletedFenceValue, m_pFence->GetCompletedValue());
    }

    return fenceValue <= m_lastCompletedFenceValue;
}

/**
***************************************************************************************************
*   DX12CmdListProfiler::WaitForFence
*
*   @brief
*       Wait for a fence to come back.
***************************************************************************************************
*/
void DX12CmdListProfiler::WaitForFence(UINT64 fenceValue)
{
    if (IsFenceComplete(fenceValue) == false)
    {
        m_pFence->SetEventOnCompletion(fenceValue, m_fenceEvent);
        WaitForSingleObject(m_fenceEvent, INFINITE);
        m_lastCompletedFenceValue = fenceValue;
    }
}
