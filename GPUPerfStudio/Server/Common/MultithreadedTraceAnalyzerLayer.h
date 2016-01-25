//==============================================================================
// Copyright (c) 2015 Advanced Micro Devices, Inc. All rights reserved.
/// \author AMD Developer Tools Team
/// \file
/// \brief  The baseclass for Mantle and DX12 TraceAnalyzerLayers.
//==============================================================================

#ifndef MULTITHREADEDTRACEANALYZERLAYER_H
#define MULTITHREADEDTRACEANALYZERLAYER_H

#include "../Common/ModernAPILayerManager.h"
#include "../Common/TraceAnalyzer.h"
#include "../Common/OSwrappers.h"
#include <map>

static const uint64 s_DummyTimestampValue = 666;

/// Forward declare this, since the definition exists in multiple places.
enum FuncId : int;

//--------------------------------------------------------------------------
/// Associate a FuncId with a string containing the name of the function.
//--------------------------------------------------------------------------
typedef std::map<FuncId, std::string> FuncIdToNamestringMap;

//--------------------------------------------------------------------------
/// The APIEntry structure is used to track all calls that are traced at runtime.
//--------------------------------------------------------------------------
class APIEntry
{
public:
    //--------------------------------------------------------------------------
    /// Constructor used to initialize members of new CallData instances.
    /// \param inThreadId The thread Id that the call was invoked from.
    /// \param inArguments A string containing the arguments of the invoked call.
    //--------------------------------------------------------------------------
    APIEntry(DWORD inThreadId, FuncId inFuncId, const std::string& inArguments)
    : mThreadId(inThreadId)
    , mFunctionId(inFuncId)
    {
        mParameters = inArguments.c_str();
    }

    //--------------------------------------------------------------------------
    /// Virtual destructor since this is subclassed elsewhere.
    //--------------------------------------------------------------------------
    virtual ~APIEntry() {}

    //--------------------------------------------------------------------------
    /// Use API-specific methods to determine the API name for this entry.
    //--------------------------------------------------------------------------
    virtual const char* GetAPIName() const = 0;

    //--------------------------------------------------------------------------
    /// Append this APIEntry's information to the end of the API Trace response.
    /// \param ioTraceResponse The API Trace response stream to append to.
    /// \param inStartTime The start time for the API call.
    /// \param inEndTime The end time for the API call.
    //--------------------------------------------------------------------------
    virtual void AppendAPITraceLine(std::stringstream& ioTraceResponse, double inStartTime, double inEndTime) const = 0;

    //--------------------------------------------------------------------------
    /// Check if this logged APIEntry is a Draw call.
    /// \returns True if the API is a draw call. False if it's not.
    //--------------------------------------------------------------------------
    virtual bool IsDrawCall() const = 0;

    //--------------------------------------------------------------------------
    /// The ID of the thread that the call was invoked/logged in.
    //--------------------------------------------------------------------------
    DWORD mThreadId;

    //--------------------------------------------------------------------------
    /// A string with the call parameters. Each API call is printed with its own format string.
    //--------------------------------------------------------------------------
    gtASCIIString mParameters;

    //--------------------------------------------------------------------------
    /// An instance of the FuncId enum. Will be converted to a string before seen in the client.
    //--------------------------------------------------------------------------
    FuncId mFunctionId;
};

//--------------------------------------------------------------------------
/// A buffer that can be used on a per-thread basis to log function calls without
/// having to deal with locking a single buffer and serializing the timing.
//--------------------------------------------------------------------------
class ThreadTraceData
{
public:
    //--------------------------------------------------------------------------
    /// Default constructor.
    //--------------------------------------------------------------------------
    ThreadTraceData()
    {
        // Initialize this to known garbage so we can check it later. It should *always* be overwritten by real data.
#ifdef _WIN32
        m_startTime.QuadPart = s_DummyTimestampValue;
#else
        m_startTime = s_DummyTimestampValue;
#endif

        // We should expect a lot of entries within this structure, so make it large enough up front.
        // @PERFORMANCE @TODO: Do we really need to reserve space for APIEntries?
        mLoggedCallVector.reserve(2048);
    }

    //--------------------------------------------------------------------------
    /// Destructor clears any remaining buffered data before the instance dies.
    //--------------------------------------------------------------------------
    virtual ~ThreadTraceData() { Clear(); }

    //--------------------------------------------------------------------------
    /// Insert the latest API call information into our list of traced calls.
    /// \param inStartTime The timestamp collected directly before the traced API call.
    /// \param inNewEntry An APIEntry instance containing the details of the traced call.
    //--------------------------------------------------------------------------
    void AddAPIEntry(GPS_TIMESTAMP inStartTime, APIEntry* inNewEntry)
    {
        // Insert a new logged duration into the call timer.
        mAPICallTimer.Add(inNewEntry->mThreadId, inStartTime);

        // Now insert the APIEntry into the list of traced API calls for this thread.
        mLoggedCallVector.push_back(inNewEntry);
    }

    //--------------------------------------------------------------------------
    /// Clear all logged data in the thread's collection buffer.
    //--------------------------------------------------------------------------
    void Clear()
    {
        // Invalidate all entries from the previous run.
        for (size_t callIndex = 0; callIndex < mLoggedCallVector.size(); ++callIndex)
        {
            // This instance isn't needed anymore, so destroy it and reclaim pool memory.
            APIEntry* thisEntry = mLoggedCallVector[callIndex];
            SAFE_DELETE(thisEntry);
        }

        mLoggedCallVector.clear();
        mAPICallTimer.Clear();
    }

    //--------------------------------------------------------------------------
    /// The start time used in computing the total duration of a logged call.
    //--------------------------------------------------------------------------
    GPS_TIMESTAMP m_startTime;

    //--------------------------------------------------------------------------
    /// An array of logged timings that can be linked to a logged call.
    //--------------------------------------------------------------------------
    TimingLog mAPICallTimer;

    //--------------------------------------------------------------------------
    /// Keep a list of InvocationData structure instances to keep track of CPU calls.
    //--------------------------------------------------------------------------
    std::vector<APIEntry*> mLoggedCallVector;
};

//--------------------------------------------------------------------------
/// A small structure that contains all information included within a trace metadata file.
//--------------------------------------------------------------------------
class TraceMetadata
{
public:
    //--------------------------------------------------------------------------
    /// Default constructor for the TraceMetadata class.
    //--------------------------------------------------------------------------
    TraceMetadata();

    //--------------------------------------------------------------------------
    /// Default destructor for the TraceMetadata class.
    //--------------------------------------------------------------------------
    ~TraceMetadata();

    //--------------------------------------------------------------------------
    /// Read a chunk of metadata XML to populate all members.
    /// \param inMetadataXML A string of XML metadata that will be parsed.
    /// \returns True if parsing was successful. False if an error occurred.
    //--------------------------------------------------------------------------
    bool ReadFromXML(const gtASCIIString& inMetadataXML);

    //--------------------------------------------------------------------------
    /// Write a chunk of XML that will ultimately be written to disk as a metadata file.
    /// \param outMetadataXML A generated chunk of XML based on the contents of this instance.
    //--------------------------------------------------------------------------
    void WriteToXML(gtASCIIString& outMetadataXML);

    //--------------------------------------------------------------------------
    /// Contains basic info about a rendered frame, including timing and API
    /// and draw call counts.
    //--------------------------------------------------------------------------
    FrameInfo mFrameInfo;

    //--------------------------------------------------------------------------
    /// The path to the cached trace file on disk.
    //--------------------------------------------------------------------------
    std::string mMetadataFilepath;

    //--------------------------------------------------------------------------
    /// The path to the cached trace file on disk.
    //--------------------------------------------------------------------------
    std::string mPathToTraceFile;

    //--------------------------------------------------------------------------
    /// The path to the captured frame buffer image on disk.
    //--------------------------------------------------------------------------
    std::string mPathToFrameBufferImage;

    //--------------------------------------------------------------------------
    /// The frame index of the traced frame.
    //--------------------------------------------------------------------------
    UINT mFrameIndex;

    //--------------------------------------------------------------------------
    /// The bitness of the instrumented application.
    //--------------------------------------------------------------------------
    osModuleArchitecture mArchitecture;
};

//--------------------------------------------------------------------------
/// A map used to associate a ThreadId with the structure that all the thread's results are stored in.
//--------------------------------------------------------------------------
typedef std::map<DWORD, ThreadTraceData*> ThreadIdToTraceData;

//--------------------------------------------------------------------------
/// Collects API Trace in a multi-threaded manner by mapping each submission
/// thread to its own buffer that it can dump logged calls to.
//--------------------------------------------------------------------------
class MultithreadedTraceAnalyzerLayer : public TraceAnalyzer
{
public:
    //--------------------------------------------------------------------------
    /// MultithreadedTraceAnalyzerLayer destructor. Will clean up trace data.
    //--------------------------------------------------------------------------
    virtual ~MultithreadedTraceAnalyzerLayer();

    //--------------------------------------------------------------------------
    /// Retrieve a string containing the current API.
    /// \returns A string containing the current API.
    //--------------------------------------------------------------------------
    virtual char const* GetAPIString() = 0;

    //--------------------------------------------------------------------------
    /// Begin the frame.
    //--------------------------------------------------------------------------
    virtual void BeginFrame();

    //--------------------------------------------------------------------------
    /// End the frame.
    //--------------------------------------------------------------------------
    virtual void EndFrame();

    //-----------------------------------------------------------------------------
    /// Overridden implementation that concatenates all thread trace logs together
    /// and returns them through a single combined string.
    /// \return A line-delimited, ASCII-encoded, version of the API Trace data.
    //-----------------------------------------------------------------------------
    virtual std::string GetAPITraceTXT();

    //-----------------------------------------------------------------------------
    /// Return GPU-time in text format, to be parsed by the Client and displayed as its own timeline.
    /// \return A line-delimited, ASCII-encoded, version of the GPU Trace data.
    //-----------------------------------------------------------------------------
    virtual std::string GetGPUTraceTXT();

    //-----------------------------------------------------------------------------
    /// Return the device used in the target application.
    //-----------------------------------------------------------------------------
    virtual void* GetActiveDevice() { return NULL; }

    //--------------------------------------------------------------------------
    /// Call this before invoking the actual implementation of an API call.
    //--------------------------------------------------------------------------
    virtual void BeforeAPICall();

    //--------------------------------------------------------------------------
    /// Invoked when the MantleTraceAnalyzerLayer is created.
    /// \param inType The incoming type of interface being created.
    /// \param pInPtr An interface pointer being created.
    /// \returns True if creation was successful.
    //--------------------------------------------------------------------------
    virtual bool OnCreate(CREATION_TYPE inType, void* pInPtr);

    //--------------------------------------------------------------------------
    /// Invoked when the MantleTraceAnalyzerLayer is finished and about to be destroyed.
    /// \param inType The incoming type of instance being destroyed.
    /// \param pInPtr An instance pointer being destroyed.
    /// \returns True if destruction was successful.
    //--------------------------------------------------------------------------
    virtual bool OnDestroy(CREATION_TYPE inType, void* pInPtr);

    //--------------------------------------------------------------------------
    /// Return the stringified function name based on input enum.
    /// \param inFunctionId An enumeration representing the function being invoked.
    /// \returns A string containing the function name.
    //--------------------------------------------------------------------------
    virtual const char* GetFunctionNameFromId(FuncId inFunctionId) = 0;

    //--------------------------------------------------------------------------
    /// Clear all logged data. Overridden because this TraceAnalyzer collects multiple
    /// instances of the same data, so it all has to be deallocated.
    //--------------------------------------------------------------------------
    void Clear();

    //--------------------------------------------------------------------------
    /// Check if the newly-started frame should be automatically traced at a specific frame.
    /// \returns An eTraceType value, corresponding to the value set in the server config.
    //--------------------------------------------------------------------------
    int GetTraceTypeFlags();

    //--------------------------------------------------------------------------
    /// Retrieve the number of API calls invoked within a traced frame.
    /// \returns The number of API calls invoked within a traced frame.
    //--------------------------------------------------------------------------
    uint32 GetNumTracedAPICalls();

    //--------------------------------------------------------------------------
    /// Retrieve the number of Draw calls that occurred within a traced frame.
    /// \returns The number of Draw calls that occurred within a traced frame.
    //--------------------------------------------------------------------------
    uint32 GetNumTracedDrawCalls();

protected:
    //--------------------------------------------------------------------------
    /// A protected constructor, because this will be used as the baseclass for a singleton.
    //--------------------------------------------------------------------------
    MultithreadedTraceAnalyzerLayer();

    //--------------------------------------------------------------------------
    /// Clear out all of the ProfilerResults collected by the DX12CmdListProfiler.
    //--------------------------------------------------------------------------
    virtual void ClearProfilingResults() = 0;

    //--------------------------------------------------------------------------
    /// Create a new instance of an API-specific ThreadTraceData object.
    /// \returns A new instance of a ThreadTraceData object.
    //--------------------------------------------------------------------------
    virtual ThreadTraceData* CreateThreadTraceDataInstance() = 0;

    //--------------------------------------------------------------------------
    /// Return a pointer to the interceptor instance being used to trace API calls.
    /// \returns A pointer to the API interceptor being used to trace calls.
    //--------------------------------------------------------------------------
    virtual InterceptorBase* GetInterceptor() = 0;

    //--------------------------------------------------------------------------
    /// Initialize any state before an API trace is performed.
    //--------------------------------------------------------------------------
    virtual void BeforeAPITrace() { }

    //--------------------------------------------------------------------------
    /// Cleanup that needs to happen after an API Trace should be implemented here.
    //--------------------------------------------------------------------------
    virtual void AfterAPITrace() { }

    //--------------------------------------------------------------------------
    /// A chance to initialize states before a GPU trace is performed.
    //--------------------------------------------------------------------------
    virtual void BeforeGPUTrace() { }

    //--------------------------------------------------------------------------
    /// Cleanup that needs to happen after a GPU Trace should be implemented here.
    //--------------------------------------------------------------------------
    virtual void AfterGPUTrace() { }

    //--------------------------------------------------------------------------
    /// No additional settings.
    /// \returns A string containing additional derived TraceAnalyzer settings.
    //--------------------------------------------------------------------------
    virtual string GetDerivedSettings() { return ""; }

    //--------------------------------------------------------------------------
    /// Clear and destroy all ThreadTraceData instances.
    //--------------------------------------------------------------------------
    void ClearCPUThreadTraceData();

    //--------------------------------------------------------------------------
    /// Handle what happens when a Linked Trace is requested. We can either:
    /// 1. Return the trace response as normal.
    /// 2. Cache the response to disk, and generate a "trace metadata" file used
    /// to retrieve the trace later.
    /// \param inFullResponseString The response string built by tracing the application.
    /// \param inbSaveResponseToFile A switch used to determine which response method to use.
    //--------------------------------------------------------------------------
    void HandleLinkedTraceResponse(std::stringstream& inFullResponseString, bool inbSaveResponseToFile);

    //--------------------------------------------------------------------------
    /// Find thread-private trace data to dump logged calls into.
    /// \param inThreadId A ThreadId used to lookup or create a corresponding ThreadTraceData instance.
    /// \returns A new or existing ThreadTraceData instance for use with a specific thread.
    //--------------------------------------------------------------------------
    ThreadTraceData* FindOrCreateThreadData(DWORD inThreadId);

    //--------------------------------------------------------------------------
    /// Read a trace's metadata file and store the parsed info into the output argument.
    /// \param inPathToMetadataFile The full path to the metadata file on disk.
    /// \param outTraceMetadata A structure containing all parsed info from the metadata file.
    /// \returns True if reading the metadata file was successful.
    //--------------------------------------------------------------------------
    bool ReadTraceMetadataFile(const std::string& inPathToMetadataFile, TraceMetadata& outTraceMetadata);

    //--------------------------------------------------------------------------
    /// Write a trace's metadata file and return the contenst through the out-param.
    /// \param inFullResponseString The full response string for a collected linked trace request.
    /// \param outMetadataXML The XML metadata string to return to the client.
    /// \returns True if writing the metadata file was succesful.
    //--------------------------------------------------------------------------
    bool WriteTraceAndMetadataFiles(const std::stringstream& inFullResponseString, std::string& outMetadataXML);

    //--------------------------------------------------------------------------
    /// Load a trace file from disk when given a valid path.
    /// \param inTraceFilepath The full filepath to a trace file.
    /// \param outTraceFileContents The full contents of the loaded trace file.
    /// \returns True if the trace file was loaded correctly.
    //--------------------------------------------------------------------------
    bool LoadTraceFile(const std::string& inTraceFilepath, gtASCIIString& outTraceFileContents);

    //--------------------------------------------------------------------------
    /// A CommandResponse used specifically for linked-mode trace viewing.
    //--------------------------------------------------------------------------
    CommandResponse mCmdLinkedTrace;

    //--------------------------------------------------------------------------
    /// A CommandResponse that's identical to normal linked trace mode, but will
    /// also save the trace response to a location on disk.
    //--------------------------------------------------------------------------
    CommandResponse mCmdLinkedTraceWithSave;

    //--------------------------------------------------------------------------
    /// A CommandResponse that accepts a path to a trace metadata file, and will
    /// handle loading/reading trace response text, and return to client.
    //--------------------------------------------------------------------------
    TextCommandResponse mRetrieveCachedTraceResponse;

    //--------------------------------------------------------------------------
    /// A CommandResponse used to retrieve the cached trace collected when AutoCapture is enabled.
    //--------------------------------------------------------------------------
    CommandResponse mCmdAutoCaptureCachedTrace;

    //--------------------------------------------------------------------------
    /// A flag to indicate if the GPU Trace has already been collected.
    //--------------------------------------------------------------------------
    bool mbGPUTraceAlreadyCollected;

    //--------------------------------------------------------------------------
    /// A flag to indicate if API calls are actively being traced during the frame.
    //--------------------------------------------------------------------------
    bool mbCollectingApiTrace;

    //--------------------------------------------------------------------------
    /// A flag to indicate if API calls are actively being profiled during the frame.
    //--------------------------------------------------------------------------
    bool mbCollectingGPUTrace;

    //--------------------------------------------------------------------------
    /// Only collect a FuncId while tracing. Build a client response using lookups in this maps.
    //--------------------------------------------------------------------------
    FuncIdToNamestringMap mFunctionIndexToNameString;

    //--------------------------------------------------------------------------
    /// A map of ThreadID -> TraceData, used to buffer logged API calls for each thread.
    //--------------------------------------------------------------------------
    ThreadIdToTraceData mThreadTraces;

    //--------------------------------------------------------------------------
    /// Mutex used in cases where the mThreadTraces map can potentially change.
    //--------------------------------------------------------------------------
    mutex mTraceMutex;

    //--------------------------------------------------------------------------
    /// A timer used to figure out what time the frame started at.
    //--------------------------------------------------------------------------
    GPS_TIMESTAMP mFramestartTime;

    //--------------------------------------------------------------------------
    /// A timer used to indicate exactly when the frame rendering begins.
    //--------------------------------------------------------------------------
    Timer mFramestartTimer;

    //--------------------------------------------------------------------------
    /// Store the last trace response here. This is necessary for AutoCapture so
    /// a client can connect after a trace completes and still collect and load the data.
    //--------------------------------------------------------------------------
    std::string mCachedTraceResponse;

    //--------------------------------------------------------------------------
    /// A flag that gets set when autocapture has occurred, but the client hasn't yet picked up the response.
    //--------------------------------------------------------------------------
    bool mbWaitingForAutocaptureClient;

};

#endif // MULTITHREADEDTRACEANALYZERLAYER_H
