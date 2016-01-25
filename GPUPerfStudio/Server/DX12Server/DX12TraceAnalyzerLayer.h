//==============================================================================
// Copyright (c) 2015 Advanced Micro Devices, Inc. All rights reserved.
/// \author AMD Developer Tools Team
/// \file
/// \brief  File contains DX12 implementation of the MultithreadedTraceAnalyzerLayer.
//==============================================================================

#ifndef DX12TRACEANALYZERLAYER_H
#define DX12TRACEANALYZERLAYER_H

#include "../Common/MultithreadedTraceAnalyzerLayer.h"
#include "../Common/TSingleton.h"
#include "Profiling/DX12CmdListProfiler.h"
#include "DX12Defines.h"
#include <GPUPerfAPITypes.h>
#include <set>

// Forward declarations.
class IDX12InstanceBase;
class GPS_ID3D12GraphicsCommandList;
class GPS_ID3D12CommandQueue;

//--------------------------------------------------------------------------
/// A set of command lists that have been executed through a Queue.
//--------------------------------------------------------------------------
typedef std::set<GPS_ID3D12GraphicsCommandList*> WrappedCommandListSet;

//--------------------------------------------------------------------------
/// A map of wrapped CommandQueue to a set of CommandLists that it has executed.
//--------------------------------------------------------------------------
typedef std::map <GPS_ID3D12CommandQueue*, WrappedCommandListSet>  CommandQueueToCommandListMap;

//--------------------------------------------------------------------------
/// Used to associate a SampleId with a ProfilerResult for a GPU-profiled function call.
//--------------------------------------------------------------------------
typedef std::map<UINT64, ProfilerResult*> SampleIdToProfilerResultMap;

//--------------------------------------------------------------------------
/// Associate a Queue type with all of the profiled calls that were submitted with the queue.
//--------------------------------------------------------------------------
typedef std::map<GPS_ID3D12CommandQueue*, SampleIdToProfilerResultMap*> QueueWrapperToProfilingResultsMap;

//--------------------------------------------------------------------------
/// The DX12APIEntry class is used to track all DX12 API calls that are traced at runtime.
//--------------------------------------------------------------------------
class DX12APIEntry : public APIEntry
{
public:
    //--------------------------------------------------------------------------
    /// Constructor used to initialize members of new DX12APIEntry instances.
    /// \param inThreadId The thread Id that the call was invoked from.
    /// \param inArguments The arguments used in the invocation of the API function.
    //--------------------------------------------------------------------------
    DX12APIEntry(DWORD inThreadId, IUnknown* inInterfaceWrapper, FuncId inFunctionId, const std::string& inArguments, INT64 inReturnValue)
        : APIEntry(inThreadId, inFunctionId, inArguments)
        , mWrapperInterface(inInterfaceWrapper)
        , mReturnValue(inReturnValue)
        , mSampleId(0)
        , preBottomTimestamp(0)
        , postBottomTimestamp(0)
    {
    }

    //--------------------------------------------------------------------------
    /// Virtual destructor since this is a derived class.
    //--------------------------------------------------------------------------
    virtual ~DX12APIEntry() {}

    //--------------------------------------------------------------------------
    /// Use API-specific methods to determine the API name for this entry.
    /// \returns A null-terminated string containing the API name.
    //--------------------------------------------------------------------------
    virtual const char* GetAPIName() const;

    //--------------------------------------------------------------------------
    /// Append this APIEntry's information to the end of the API Trace response.
    /// \param ioTraceResponse The API Trace response stream to append to.
    /// \param inStartTime The start time for the API call.
    /// \param inEndTime The end time for the API call.
    //--------------------------------------------------------------------------
    virtual void AppendAPITraceLine(std::stringstream& ioTraceResponse, double inStartTime, double inEndTime) const;

    //--------------------------------------------------------------------------
    /// Check if this logged APIEntry is a Draw call.
    /// \returns True if the API is a draw call. False if it's not.
    //--------------------------------------------------------------------------
    bool IsDrawCall() const;

    //--------------------------------------------------------------------------
    /// The return value for this API call.
    //--------------------------------------------------------------------------
    INT64 mReturnValue;

    //--------------------------------------------------------------------------
    /// The optional sample Id used to associate this API call with GPA profiling results.
    //--------------------------------------------------------------------------
    gpa_uint32 mSampleId;

    //--------------------------------------------------------------------------
    /// This is an IUnknown* to the GPS_ID3D12 wrapper interface.
    //--------------------------------------------------------------------------
    IUnknown* mWrapperInterface;

    //--------------------------------------------------------------------------
    /// The pre-Bottom timestamp for this entry
    //--------------------------------------------------------------------------
    gpa_float64 preBottomTimestamp;

    //--------------------------------------------------------------------------
    /// The post-Bottom timestamp for this entry
    //--------------------------------------------------------------------------
    gpa_float64 postBottomTimestamp;
};

//--------------------------------------------------------------------------
/// A map of SampleID to DX12APIEntry. Used for fast lookup of entries with profiling results.
//--------------------------------------------------------------------------
typedef std::map < gpa_uint32, DX12APIEntry* > SampleIdToAPIEntryMap;

//--------------------------------------------------------------------------
/// A map of ID3D12GraphicsCommandList to list of GPA sample Ids DX12APIEntry. Used for fast lookup of entries with profiling results.
//--------------------------------------------------------------------------
typedef std::map < const ID3D12GraphicsCommandList* const, std::vector<gpa_uint32> > CommandListToSampleIdMap;

//--------------------------------------------------------------------------
/// A buffer that can be used on a per-thread basis to log function calls without
/// having to deal with locking a single buffer and serializing call timings.
//--------------------------------------------------------------------------
class DX12ThreadTraceData : public ThreadTraceData
{
public:
    //--------------------------------------------------------------------------
    /// Default constructor does nothing except initialize the baseclass.
    //--------------------------------------------------------------------------
    DX12ThreadTraceData()
        : ThreadTraceData()
    {
    }

    //--------------------------------------------------------------------------
    /// Destructor does nothing at this level.
    //--------------------------------------------------------------------------
    virtual ~DX12ThreadTraceData() { }

    //--------------------------------------------------------------------------
    /// Given a SampleId, find the DX12APIEntry that was logged while API tracing.
    /// \param inSampleId The SampleId to search with.
    /// \return The buffered DX12APIEntry with a matching SampleId, or NULL if it doesn't exist for some reason.
    //--------------------------------------------------------------------------
    DX12APIEntry* FindInvocationBySampleId(UINT64 inSampleId)
    {
        DX12APIEntry* resultInvocation = NULL;

        for (unsigned int i = 0; i < mLoggedCallVector.size(); i++)
        {
            DX12APIEntry* entry = static_cast<DX12APIEntry*>(mLoggedCallVector[i]);

            if (entry->mSampleId == inSampleId)
            {
                resultInvocation = entry;
                break;
            }
        }

        return resultInvocation;
    }
};

//--------------------------------------------------------------------------
/// Collects API/GPU Trace in a multi-threaded manner by mapping each submission
/// thread to its own buffer that it can dump logged calls to.
//--------------------------------------------------------------------------
class DX12TraceAnalyzerLayer : public MultithreadedTraceAnalyzerLayer, public TSingleton < DX12TraceAnalyzerLayer >
{
    friend TSingleton < DX12TraceAnalyzerLayer >;

public:
    //--------------------------------------------------------------------------
    /// DX12TraceAnalyzerLayer destructor. Will clean up trace data.
    //--------------------------------------------------------------------------
    virtual ~DX12TraceAnalyzerLayer();

    //--------------------------------------------------------------------------
    /// Retrieve a pointer to the parent LayerManager used by this tool.
    /// \returns A pointer to the parent LayerManager used by this tool.
    //--------------------------------------------------------------------------
    ModernAPILayerManager* GetParentLayerManager();

    //--------------------------------------------------------------------------
    /// Identify which API this TraceAnalyzer subclass handles.
    /// \returns A string containing "DX12".
    //--------------------------------------------------------------------------
    virtual char const* GetAPIString() { return "DX12"; }

    //--------------------------------------------------------------------------
    /// Provides a chance to initialize states before a GPU trace is performed.
    //--------------------------------------------------------------------------
    virtual void BeforeGPUTrace();

    //-----------------------------------------------------------------------------
    /// Return GPU-time in text format, to be parsed by the Client and displayed as its own timeline.
    /// \return A line-delimited, ASCII-encoded, version of the GPU Trace data.
    //-----------------------------------------------------------------------------
    virtual std::string GetGPUTraceTXT();

    //--------------------------------------------------------------------------
    /// Return the stringified function name based on input enum.
    /// \param inFunctionId An enumeration representing the function being invoked.
    //--------------------------------------------------------------------------
    virtual const char* GetFunctionNameFromId(FuncId inFunctionId);

    //--------------------------------------------------------------------------
    /// Retrieve the API group that an API call has been classified into.
    /// \param inAPIFuncId The FunctionId of an API call to retrieve the group for.
    /// \returns An API Type that a call has been classified as being part of.
    //--------------------------------------------------------------------------
    eAPIType GetAPIGroupFromAPI(FuncId inAPIFuncId) const;

    //--------------------------------------------------------------------------
    /// Retrieve the interceptor instance responsible for managing tracing in this API.
    /// \returns The InterceptorBase that's used to control tracing for this API.
    //--------------------------------------------------------------------------
    virtual InterceptorBase* GetInterceptor();

    //--------------------------------------------------------------------------
    /// Insert an APIEntry into the list of entries with profiler results.
    /// \param inEntry An APIEntry with a sampled GPU time.
    //--------------------------------------------------------------------------
    void StoreProfilerResult(DX12APIEntry* inEntry);

    //--------------------------------------------------------------------------
    /// Store results obtained from the DX12CmdListProfiler.
    /// \param pQueue The Queue that the profiled calls were executed through.
    /// \param inProfilerResults The set of results collected after executed profiled commands.
    //--------------------------------------------------------------------------
    void StoreProfilerResults(GPS_ID3D12CommandQueue* pQueue, const std::vector<ProfilerResult>& inProfilerResults);

    //--------------------------------------------------------------------------
    /// Get the Profiler Results for the specified command lists
    /// \param numCommandLists the number of command lists contained in ppCommandLists
    /// \prarm ppCommandLists the set of command lists to get the profiler results from
    //--------------------------------------------------------------------------
    virtual void GetProfileResultsForCmdLists(UINT numCommandLists, ID3D12CommandList* const* ppCommandLists);

    //--------------------------------------------------------------------------
    /// Opens a GPA context for the specified command list and initializes it for counter collection
    /// \param pCommandList the command list to use to open a GPA context
    //--------------------------------------------------------------------------
    void OpenAndInitializeGPAContext(const ID3D12GraphicsCommandList* const pCommandList);

    //--------------------------------------------------------------------------
    /// Associates the specified sample id with the specified command list
    /// \param pCommandList the command list with which to associate the sampleId
    /// \param sampleId the sample id to associate with the specified command list
    //--------------------------------------------------------------------------
    void AddSampleToCommandList(const ID3D12GraphicsCommandList* const pCommandList, gpa_uint32 sampleId);

    //--------------------------------------------------------------------------
    /// Checks whether the specified sampleId is associated with the specified commandList
    /// \param pCommandList the command list to check
    /// \param sampleId the sample id to check
    /// \return true if the sample id is associated with the command list
    //--------------------------------------------------------------------------
    bool CommandListContainsSample(const ID3D12GraphicsCommandList* const pCommandList, gpa_uint32 sampleId);

    //--------------------------------------------------------------------------
    /// Given a SampleId, find the DX12APIEntry that was logged while API tracing.
    /// \param inSampleId The SampleId to search with.
    /// \return The buffered DX12APIEntry with a matching SampleId, or NULL if it doesn't exist for some reason.
    //--------------------------------------------------------------------------
    DX12APIEntry* FindInvocationBySampleId(UINT64 inSampleId);

    //--------------------------------------------------------------------------
    /// Call this to complete the call log. This will dump the logged data into a buffer for later use.
    /// Log a DX12 API call within the Trace Analyzer.
    /// \param inInterface The interface wrapper being used to invoke the API call.
    /// \param inFunctionId The FuncId for the API function that's being logged.
    /// \param inArguments The stringified API call arguments for the function that's being logged.
    /// \param inReturnValue The return value for the API call, or FUNCTION_RETURNS_VOID if there is none.
    /// \returns The newly-created DX12APIEntry instance used to track the info for this API call.
    //--------------------------------------------------------------------------
    DX12APIEntry* LogAPICall(IUnknown* inWrappedInterface, FuncId inFunctionId, const char* inArguments, INT64 inReturnValue);

    //--------------------------------------------------------------------------
    /// Associate a set of CommandLists with the CommandQueue that they'll be executed through.
    /// \param inCommandQueueWrapper A CommandQueue instance used to execute CommandLists.
    /// \param inNumCommandLists The number of CommandLists in the incoming array.
    /// \param inCommandLists An array of CommandLists to associate with the incoming CommandQueue.
    //--------------------------------------------------------------------------
    void TrackParentCommandQueue(GPS_ID3D12CommandQueue* inCommandQueueWrapper, UINT inNumCommandLists, GPS_ID3D12GraphicsCommandList* const* inCommandLists);



    //--------------------------------------------------------------------------
    /// A function used to check if an API function should be profiled.
    /// \param inFuncId The FuncId of the API call to check for profilabity.
    /// \returns True if the function is able to be profiled.
    //--------------------------------------------------------------------------
    inline bool ShouldProfileFunction(FuncId inFuncId) const { return (true == mbProfiledFuncs[inFuncId]); }

protected:
    //--------------------------------------------------------------------------
    /// Protected constructor, because this is a singleton.
    //--------------------------------------------------------------------------
    DX12TraceAnalyzerLayer();

    //--------------------------------------------------------------------------
    /// Clear out all of the ProfilerResults collected by the DX12CmdListProfiler.
    //--------------------------------------------------------------------------
    virtual void ClearProfilingResults();

    //--------------------------------------------------------------------------
    /// Create a new ThreadTraceData instance for use specifically with DX12 APIs.
    /// \returns A new ThreadTraceData instance to trace DX12 API calls.
    //--------------------------------------------------------------------------
    virtual ThreadTraceData* CreateThreadTraceDataInstance();

private:
    //--------------------------------------------------------------------------
    /// Initialize the set of functions that are able to be profiled.
    //--------------------------------------------------------------------------
    void SetProfiledFuncs();

    //--------------------------------------------------------------------------
    /// Find the CommandQueue associated with a profiled DX12APIEntry instance.
    /// \param inEntry The DX12 API Entry structure to find a CommandQueue for.
    /// \returns The CommandQueue associated with the execution of the given API Entry.
    //--------------------------------------------------------------------------
    GPS_ID3D12CommandQueue* FindCommandQueueForAPIEntry(DX12APIEntry* inEntry);

    //--------------------------------------------------------------------------
    /// Retrieve the type of CommandList when given the Queue that executed it.
    /// \param inQueue A CommandQueue to retrieve the command list type for.
    /// \returns The type of CommandList utilized by the given queue.
    //--------------------------------------------------------------------------
    D3D12_COMMAND_LIST_TYPE GetCommandListTypeFromCommandQueue(GPS_ID3D12CommandQueue* inQueue);

    //--------------------------------------------------------------------------
    /// Copy a profiler result from one chunk of memory to another.
    /// \param inDestination The destination ProfilerResults memory to copy to.
    /// \param inSource The source memory to copy from.
    //--------------------------------------------------------------------------
    void CopyProfilerResult(ProfilerResult* inDestination, const ProfilerResult* inSource)
    {
        // Copy the measurement info.
        memcpy(&inDestination->measurementInfo, &inSource->measurementInfo, sizeof(ProfilerMeasurementInfo));
        memcpy(&inDestination->timestampResult, &inSource->timestampResult, sizeof(ProfilerTimestampResult));
    }

    //--------------------------------------------------------------------------
    /// Create a new map to associate a LinkId with profiling results.
    /// \param inWrappedQueue The Queue that was submitted in order to retrieve profiling results.
    //--------------------------------------------------------------------------
    SampleIdToProfilerResultMap* FindOrCreateProfilerResultsMap(GPS_ID3D12CommandQueue* pWrappedQueue);

    //--------------------------------------------------------------------------
    /// The session ID returned from GPA_BeginSession. There's only ever one session active.
    //--------------------------------------------------------------------------
    gpa_uint32 mCurrentSessionId;

    //--------------------------------------------------------------------------
    /// A map that associates a GPA SampleID with the APIEntry for the call.
    //--------------------------------------------------------------------------
    SampleIdToAPIEntryMap mSampleIdToEntry;

    //--------------------------------------------------------------------------
    /// A map that associates a command list with a list of samples
    //--------------------------------------------------------------------------
    CommandListToSampleIdMap mCommandListToSampleId;

    //--------------------------------------------------------------------------
    /// A mutex used to lock the CommandQueue->CommandLists association map.
    //--------------------------------------------------------------------------
    mutex mCommandQueueLockMutex;

    //--------------------------------------------------------------------------
    /// A map that associates CommandQueue instances with the CommandLists that they execute.
    //--------------------------------------------------------------------------
    CommandQueueToCommandListMap mCommandQueueTracker;

    //--------------------------------------------------------------------------
    /// A mutex to lock the profiling results container.
    //--------------------------------------------------------------------------
    mutex mProfilingResultsMutex;

    //--------------------------------------------------------------------------
    /// Entries with profiling results. The key is a command queue,
    /// and the value is a map of profiler results.
    //--------------------------------------------------------------------------
    QueueWrapperToProfilingResultsMap mEntriesWithProfilingResults;

    //--------------------------------------------------------------------------
    /// An array of bools used to determine which FunctionIds are available for profiling.
    //--------------------------------------------------------------------------
    bool mbProfiledFuncs[FuncId_MAX];
};

#endif // DX12TRACEANALYZERLAYER_H
