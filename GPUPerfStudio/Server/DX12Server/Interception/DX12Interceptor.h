//==============================================================================
// Copyright (c) 2014-2015 Advanced Micro Devices, Inc. All rights reserved.
/// \author AMD Developer Tools Team
/// \file
/// \brief  The DX12Interceptor contains the mechanisms responsible for
///         instrumenting DX12 objects and function calls through hooking.
//==============================================================================

#ifndef DX12INTERCEPTOR_H
#define DX12INTERCEPTOR_H

#include "DX12Defines.h"
#include <GPUPerfAPI.h>
#include "GPUPerfAPILoader.h"
#include "Profiling/DX12CmdListProfiler.h"

class IDX12InstanceBase;
class GPS_ID3D12GraphicsCommandList;

//--------------------------------------------------------------------------
/// A map of ID3D12Device to the associated CommandListProfiler.
//--------------------------------------------------------------------------
typedef std::map<ID3D12Device*, DX12CmdListProfiler*> DeviceToProfilerMap;

//--------------------------------------------------------------------------
/// The DX12Interceptor contains the mechanisms responsible for instrumenting
/// DX12 objects and function calls through hooking.
//--------------------------------------------------------------------------
class DX12Interceptor : public InterceptorBase
{
public:
    //--------------------------------------------------------------------------
    /// DX12Interceptor constructor.
    //--------------------------------------------------------------------------
    DX12Interceptor();

    //--------------------------------------------------------------------------
    /// DX12Interceptor destructor.
    //--------------------------------------------------------------------------
    virtual ~DX12Interceptor();

    //--------------------------------------------------------------------------
    /// Set the internal flag that determines if API Trace collection is enabled.
    /// \param inbCollectTrace The flag used to enable or disable API tracing.
    //--------------------------------------------------------------------------
    virtual void SetCollectTrace(bool inbCollectTrace) { mbCollectApiTrace = inbCollectTrace; }

    //--------------------------------------------------------------------------
    /// Set the internal flag that determines if GPU command profiling is enabled.
    /// \param inbProfilingEnabled The flag used to enable or disable profiling.
    //--------------------------------------------------------------------------
    virtual void SetProfilingEnabled(bool inbProfilingEnabled) { mbProfilerEnabled = inbProfilingEnabled; }

    //--------------------------------------------------------------------------
    /// Retrieve a pointer to the LayerManager that owns this InterceptorBase instance.
    /// \returns A pointer to the DX12LayerManager.
    //--------------------------------------------------------------------------
    virtual ModernAPILayerManager* GetParentLayerManager();

    //--------------------------------------------------------------------------
    /// Initialize the DX12Interceptor component.
    /// \returns True if the DX12Interceptor was initialized successfully.
    //--------------------------------------------------------------------------
    bool Initialize();

    //--------------------------------------------------------------------------
    /// Attach API entrypoints for hooking.
    /// \returns True if entrypoints were successfully hooked.
    //--------------------------------------------------------------------------
    bool HookInterceptor();

    //--------------------------------------------------------------------------
    /// Detach all hooked API entrypoints.
    /// \returns True if entrypoints were successfully detached.
    //--------------------------------------------------------------------------
    bool UnhookInterceptor();

    //--------------------------------------------------------------------------
    /// Construct a measurement info structure for each call that will be profiled.
    /// \param inFuncId The function ID for the function being profiled.
    /// \param inSampleId The SampleID associated with the profiled command.
    /// \param pCmdList The command list that executed the profiled command.
    //--------------------------------------------------------------------------
    ProfilerMeasurementId ConstructMeasurementInfo(FuncId inFuncId, UINT64 inSampleId, ID3D12GraphicsCommandList* pCmdList);

    //--------------------------------------------------------------------------
    /// Initialize GPA.
    /// \param inAPI The API being initialized.
    /// \returns True if GPA initialization was successful.
    //--------------------------------------------------------------------------
    bool InitializeGPA(GPA_API_Type inAPI);

    //--------------------------------------------------------------------------
    /// Shutdown the DX12Interceptor.
    /// \returns True if shutdown was successful. False if it failed.
    //--------------------------------------------------------------------------
    bool Shutdown();

    //--------------------------------------------------------------------------
    /// Handler used before the real runtime implementation of an API call has been invoked.
    /// \param inWrappedInterface The interface pointer used to invoke the API call.
    /// \param inFunctionId The FuncId corresponding to the API call being traced.
    //--------------------------------------------------------------------------
    void PreCall(IUnknown* inWrappedInterface, FuncId inFunctionId);

    //--------------------------------------------------------------------------
    /// Handler used after the real runtime implementation of an API call has been invoked.
    /// \param inWrappedInterface The interface pointer used to invoke the API call.
    /// \param inFunctionId The FuncId corresponding to the API call being traced.
    /// \param inArgumentString A string containing the stringified call invocation arguments.
    /// \param inReturnValue The return value result of the real runtime call.
    //--------------------------------------------------------------------------
    void PostCall(IUnknown* inWrappedInterface, FuncId inFunctionId, const char* inArgumentString, INT64 inReturnValue = FUNCTION_RETURNS_VOID);

    //--------------------------------------------------------------------------
    /// Gather profiler results.
    /// \param inWrappedInterface The interface responsible for the profiled call.
    /// \paramNumCommandLists The number of CommandLists in the array being executed.
    /// \param ppCommandLists The array of CommandLists to be executed.
    //--------------------------------------------------------------------------
    void GatherProfilerResults(IUnknown* inWrappedInterface, UINT NumCommandLists, ID3D12CommandList* const* ppCommandLists);

    //--------------------------------------------------------------------------
    /// A function used to check if a function should be logged in the API Trace.
    /// \returns True if the function should be logged in the API Trace.
    //--------------------------------------------------------------------------
    inline bool ShouldCollectTrace() const { return mbCollectApiTrace; }

    //--------------------------------------------------------------------------
    /// A function used to check if GPU time is being collected while building the frame.
    /// \returns True if GPU Time collection is active. False if it is disabled.
    //--------------------------------------------------------------------------
    inline bool ShouldCollectGPUTime() const { return mbProfilerEnabled; }

    //--------------------------------------------------------------------------
    /// A GPA logging callback used to output GPA messages to the GPS log.
    /// \param messageType The type of message being logged.
    /// \param message The message string being logged.
    //--------------------------------------------------------------------------
    static void GPALoggingCallback(GPA_Logging_Type messageType, const char* message);

    //--------------------------------------------------------------------------
    /// Retrieve a reference to the GPA loader wrapper.
    /// \returns A reference to the GPA loader wrapper.
    //--------------------------------------------------------------------------
	GPUPerfAPILoader& GetGPALoader() { return mGPALoader; }

    //--------------------------------------------------------------------------
    /// Switch the context used by GPA to profile GPU time.
    /// \param inCurrentContext The commandlist to be used to set the new context.
    //--------------------------------------------------------------------------
    void SwitchGPAContext(ID3D12GraphicsCommandList* inCurrentContext);

    //--------------------------------------------------------------------------
    /// Lock GPA access mutex
    //--------------------------------------------------------------------------
    void LockGPA();

    //--------------------------------------------------------------------------
    /// unlock GPA access mutex
    //--------------------------------------------------------------------------
    void UnlockGPA();

    //--------------------------------------------------------------------------
    /// Reset the SampleId unique globally unique number generator.
    //--------------------------------------------------------------------------
    void ResetSampleIdCounter();

    //--------------------------------------------------------------------------
    /// A hook used to intercept D3D12GetDebugInterface.
    //--------------------------------------------------------------------------
    RealAndMineHook<PFN_D3D12_GET_DEBUG_INTERFACE> mHook_GetDebugInterface;

    //--------------------------------------------------------------------------
    /// A hook used to intercept D3D12CreateDevice.
    //--------------------------------------------------------------------------
    RealAndMineHook<PFN_D3D12_CREATE_DEVICE> mHook_CreateDevice;

    //--------------------------------------------------------------------------
    /// A hook used to intercept D3D12SerializeRootSignature.
    //--------------------------------------------------------------------------
    RealAndMineHook<PFN_D3D12_SERIALIZE_ROOT_SIGNATURE> mHook_SerializeRootSignature;

    //--------------------------------------------------------------------------
    /// A hook used to intercept D3D12CreateRootSignatureDeserializer.
    //--------------------------------------------------------------------------
    RealAndMineHook<PFN_D3D12_CREATE_ROOT_SIGNATURE_DESERIALIZER> mHook_CreateRootSignatureDeserializer;

#ifdef DLL_REPLACEMENT
    //--------------------------------------------------------------------------
    /// Set the handle to the real D3D12.DLL module.
    /// \param hRealD3D12 The HINSTANCE handle to the real D3D12.DLL module.
    //--------------------------------------------------------------------------
    void SetRealDllHandle(HINSTANCE hRealD3D12) { mRealD3D12 = hRealD3D12; }
#endif

private:
    //--------------------------------------------------------------------------
    /// Set the active profiler SampleId for the given thread ID.
    // \param inThreadId The CPU thread ID responsible for invoking the sampled API call.
    // \returns A new SampleId.
    //--------------------------------------------------------------------------
    gpa_uint32 SetNextSampleId(DWORD inThreadId);

    //--------------------------------------------------------------------------
    /// Generate the next globally-unique SampleId.
    /// \returns A new SampleId that can be used to profile a GPU command.
    //--------------------------------------------------------------------------
    gpa_uint32 GetNextSampleId();

    //--------------------------------------------------------------------------
    /// A small helper structure to track Begin/End Sample calls per thread.
    //--------------------------------------------------------------------------
    struct SampleInfo
    {
        //--------------------------------------------------------------------------
        /// The SampleId assigned to the GPA sample.
        //--------------------------------------------------------------------------
        gpa_uint32 mSampleId;

        //--------------------------------------------------------------------------
        /// Was BeginSample successful? If so, we can call EndSample.
        //--------------------------------------------------------------------------
        bool mbBeginSampleSuccessful;
    };

    //--------------------------------------------------------------------------
    /// A map used to associate a ThreadId with a SampleInfo structure.
    //--------------------------------------------------------------------------
    typedef std::map<DWORD, SampleInfo> ThreadIdToSampleIdMap;

    //--------------------------------------------------------------------------
    /// An association of CPU ThreadId to a SampleInfo structure.
    //--------------------------------------------------------------------------
    ThreadIdToSampleIdMap mSampleIdMap;

    //--------------------------------------------------------------------------
    /// A globally unique SampleId counter.
    //--------------------------------------------------------------------------
    gpa_uint32 mSampleIndex;

    //--------------------------------------------------------------------------
    /// A mutex used to lock the SampleId generator.
    //--------------------------------------------------------------------------
    mutex mUniqueSampleIdMutex;

    //--------------------------------------------------------------------------
    /// A mutex used to lock usage of GPA when switching contexts between threads.
    //--------------------------------------------------------------------------
    mutex mGPAPrePostMatcherMutex;

    //--------------------------------------------------------------------------
    /// A map typedef to associate a CommandList with the number of samples to collect.
    //--------------------------------------------------------------------------
    typedef std::map<GPS_ID3D12GraphicsCommandList*, gpa_uint32> CommandListToSampleCountMap;

    //--------------------------------------------------------------------------
    /// A map that associates a CommandList with a count of GPA samples gathered through the list.
    //--------------------------------------------------------------------------
    CommandListToSampleCountMap mSamplesPerCommandList;

    //--------------------------------------------------------------------------
    /// A GPA loader used as a convenience wrapper for GPA calls.
    //--------------------------------------------------------------------------
	GPUPerfAPILoader mGPALoader;

    //--------------------------------------------------------------------------
    /// A flag used to track if an API trace should be collected when building the frame.
    //--------------------------------------------------------------------------
    bool mbCollectApiTrace;

    //--------------------------------------------------------------------------
    /// A flag used to track if a command should be profiled when building the frame.
    //--------------------------------------------------------------------------
    bool mbProfilerEnabled;

    //--------------------------------------------------------------------------
    /// Handle to real D3D12.dll module, when operating in DLL_REPLACEMENT mode.
    //--------------------------------------------------------------------------
    HINSTANCE mRealD3D12;
};

#endif // DX12INTERCEPTOR_H
