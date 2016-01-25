//=================================================================================================
// Copyright (c) 2015 Advanced Micro Devices, Inc. All rights reserved.
/// \author AMD Developer Tools Team
/// \file   DX12CmdListProfiler.h
/// \brief  DX12 command list profiler.
///         This standalone class injects queries into app command buffers
///         to determine GPU time and counter information.
//=================================================================================================

#ifndef __DX12_CMD_LIST_PROFILER_H__
#define __DX12_CMD_LIST_PROFILER_H__

#include <d3d12.h>
#include <unordered_map>

#ifdef _DEBUG
#define PROFILER_ASSERT(__expr__) if (!(__expr__)) __debugbreak();
#define PROFILER_ASSERT_ALWAYS() __debugbreak();
#else
#define PROFILER_ASSERT(__expr__)
#define PROFILER_ASSERT_ALWAYS()
#endif

/**
***************************************************************************************************
*   @brief  Define possible measurement types.
***************************************************************************************************
*/
enum ProfilerMeasurementTypeFlags
{
    PROFILER_MEASUREMENT_TYPE_NONE       = 0x0,
    PROFILER_MEASUREMENT_TYPE_TIMESTAMPS = 0x1,
    PROFILER_MEASUREMENT_TYPE_PIPE_STATS = 0x2,
};

/**
***************************************************************************************************
*   @brief  Each command list being executed has a state to help track correct start/stop order.
***************************************************************************************************
*/
enum ProfilerState
{
    PROFILER_STATE_INIT,
    PROFILER_STATE_STARTED,
    PROFILER_STATE_ENDED,
};

/**
***************************************************************************************************
*   @brief  Define return codes specific to profiler.
***************************************************************************************************
*/
enum ProfilerResultCode
{
    PROFILER_SUCCESS,
    PROFILER_FAIL,
    PROFILER_THIS_CMD_LIST_WAS_NOT_MEASURED,
    PROFILER_MEASUREMENT_NOT_STARTED,
};

/**
***************************************************************************************************
*   @brief  Configure the profiler to work in a certain way.
***************************************************************************************************
*/
struct DX12CmdListProfilerConfig
{
    UINT measurementsPerGroup;
    UINT measurementTypeFlags;
};

/**
***************************************************************************************************
*   @brief  Helper struct used to point to start and end values.
***************************************************************************************************
*/
struct ProfilerInterval
{
    UINT64 start;
    UINT64 end;
};

/**
***************************************************************************************************
*   @brief  The client should fill one of these in, to uniquely identify each measurement.
***************************************************************************************************
*/
struct ProfilerMeasurementId
{
    UINT64                     sampleId;
    UINT                       funcId;
    UINT                       frame;
    ID3D12GraphicsCommandList* pCmdList;
};

/**
***************************************************************************************************
*   @brief  Lets the client store information about the current measurement.
***************************************************************************************************
*/
struct ProfilerMeasurementInfo
{
    ProfilerMeasurementId idInfo;
    UINT64                measurementNum;
};

/**
***************************************************************************************************
*   @brief  Output structures containing results, to be read by client.
***************************************************************************************************
*/
struct ProfilerTimestampResult
{
    ProfilerInterval rawClocks;
    ProfilerInterval adjustedClocks;
    double           execMicroSecs;
};

/**
***************************************************************************************************
*   @brief  Hold results for a measurement and related info.
***************************************************************************************************
*/
struct ProfilerResult
{
    ProfilerMeasurementInfo              measurementInfo;
    ProfilerTimestampResult              timestampResult;
    D3D12_QUERY_DATA_PIPELINE_STATISTICS pipeStatsResult;
};

/**
***************************************************************************************************
*   @brief  Wrap what is required to timestamp commands in DX12.
***************************************************************************************************
*/
struct ProfilerMeasurementGroup
{
    ID3D12QueryHeap*                      pPipeStatsQueryHeap;
    ID3D12Resource*                       pPipeStatsBuffer;
    ID3D12QueryHeap*                      pTimestampQueryHeap;
    ID3D12Resource*                       pTimestampBuffer;
    std::vector<ProfilerMeasurementInfo>  measurementInfos;
    UINT                                  groupMeasurementCount;
};

/**
***************************************************************************************************
*   @brief  Holds information for each command list that is being measured.
***************************************************************************************************
*/
struct ProfilerCmdListData
{
    ProfilerState                         state;
    UINT                                  cmdListMeasurementCount;
    std::vector<ProfilerMeasurementGroup> measurementGroups;
    ProfilerMeasurementGroup*             pActiveMeasurementGroup;
};

/**
***************************************************************************************************
*   @brief  Utility typedefs
***************************************************************************************************
*/
typedef std::unordered_map<ID3D12GraphicsCommandList*, ProfilerCmdListData> CmdListDataMap;
typedef std::unordered_map<ID3D12CommandQueue*, UINT64> QueueFrequencies;

/**
***************************************************************************************************
*   DX12ProfilerCriticalSection
*
*   @brief
*       Helper class to wrap a Windows CRITICAL_SECTION.
***************************************************************************************************
*/
class DX12ProfilerCriticalSection
{
public:
    DX12ProfilerCriticalSection()
    {
        InitializeCriticalSection(&m_criticalSection);
    }

    ~DX12ProfilerCriticalSection()
    {
        DeleteCriticalSection(&m_criticalSection);
    }

    void Enter()
    {
        EnterCriticalSection(&m_criticalSection);
    }

    void Leave()
    {
        LeaveCriticalSection(&m_criticalSection);
    }

protected:
    CRITICAL_SECTION m_criticalSection;
};

/**
***************************************************************************************************
*   DX12ProfilerScopedLock
*
*   @brief
*       Helper class to provide locking mechanism using RAII.
***************************************************************************************************
*/
class DX12ProfilerScopedLock
{
public:
    DX12ProfilerScopedLock(DX12ProfilerCriticalSection& cs) :
        m_cs(cs)
    {
        cs.Enter();
    }

    ~DX12ProfilerScopedLock()
    {
        m_cs.Leave();
    }

private:
    DX12ProfilerScopedLock& operator=(const DX12ProfilerScopedLock& inOther) { UNREFERENCED_PARAMETER(inOther); }
    DX12ProfilerCriticalSection& m_cs;
};

/**
***************************************************************************************************
*   DX12CmdListProfiler
*
*   @brief
*       The DX12 command list profiler.
***************************************************************************************************
*/
class DX12CmdListProfiler
{
public:
    static DX12CmdListProfiler* Create(
        ID3D12Device*                    pDevice,
        const DX12CmdListProfilerConfig* pConfig);

    ~DX12CmdListProfiler();

    ProfilerResultCode BeginCmdMeasurement(
        ID3D12GraphicsCommandList*   pCmdList,
        const ProfilerMeasurementId* pIdInfo);

    ProfilerResultCode EndCmdMeasurement(ID3D12GraphicsCommandList* pCmdList);

    ProfilerResultCode GetCmdListResults(
        ID3D12GraphicsCommandList*   pCmdList,
        ID3D12CommandQueue*          pCmdQueue,
        std::vector<ProfilerResult>& results);

    // Use this to change whether we want timestamps/counters or both
    void UpdateMeasurementType(ProfilerMeasurementTypeFlags measurementTypeFlags)
    {
        m_config.measurementTypeFlags = measurementTypeFlags;
    }

private:
    DX12CmdListProfiler(ID3D12Device* pDevice);

    HRESULT Init(
        ID3D12Device*                    pDevice,
        const DX12CmdListProfilerConfig* pConfig);

    HRESULT SetupNewMeasurementGroup(ProfilerCmdListData& cmdListData);

    UINT64 GetQueueFrequency(ID3D12CommandQueue* pCmdQueue);

    HRESULT CreateQueryBuffer(ID3D12Resource** pResource, UINT size);

    // This determines if a command list has already been seen
    inline bool IsCmdListBeingMeasured(ID3D12GraphicsCommandList* pCmdList)
    {
        return (m_cmdListMap.find(pCmdList) != m_cmdListMap.end());
    }

    // Maximum of two numbers
    inline UINT64 Max(UINT64 val1, UINT64 val2)
    {
        return (val1 > val2) ? val1 : val2;
    }

    bool IsFenceComplete(UINT64 FenceValue);

    void WaitForFence(UINT64 FenceValue);

    HRESULT ReleaseProfilerData(ProfilerCmdListData& data);

    // DX12 device
    ID3D12Device* m_pDevice;

    // Holds per-command list information for each begin-end measurement
    CmdListDataMap m_cmdListMap;

    // Holds per-command list information for each begin-end measurement
    QueueFrequencies m_queueFrequencies;

    // Critical section object
    DX12ProfilerCriticalSection m_cs;

    // Profiler configuration
    DX12CmdListProfilerConfig m_config;

    // Synchronization members used to wait for a command list to finish
    ID3D12Fence* m_pFence;
    UINT64 m_nextFenceValue;
    UINT64 m_lastCompletedFenceValue;
    HANDLE m_fenceEvent;
};
#endif // __DX12_CMD_LIST_PROFILER_H__
