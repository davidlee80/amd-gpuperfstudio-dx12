//=====================================================================
// Copyright 2016 (c), Advanced Micro Devices, Inc. All rights reserved.
//
/// \author AMD Developer Tools Team
/// \file osProcess.h 
/// 
//=====================================================================

//------------------------------ osProcess.h ------------------------------

#ifndef __OSPROCESS
#define __OSPROCESS

// Forward decelerations:
struct osEnvironmentVariable;

// Infra:
#include <AMDTBaseTools/Include/gtString.h>
#include <AMDTBaseTools/Include/gtVector.h>

// Local:
#include <AMDTOSWrappers/Include/osOSWrappersDLLBuild.h>
#include <AMDTOSWrappers/Include/osOSDefinitions.h>
#include <AMDTOSWrappers/Include/osFilePath.h>
#include <AMDTOSWrappers/Include/osModuleArchitecture.h>

#if (GR_BUILD_TARGET == GR_WINDOWS_OS)
    #include <TlHelp32.h>
#endif

enum osRuntimePlatform
{
    OS_NATIVE_PLATFORM,
    OS_JAVA_PLATFORM,
    OS_DOT_NET_PLATFORM,
    OS_UNKNOWN_PLATFORM
};

OS_API void osSetCodeXLExecutionPath(const osFilePath& codeXLExecutionPath);
OS_API bool osSetCurrentProcessEnvVariable(const osEnvironmentVariable& envVariable);
OS_API bool osRemoveCurrentProcessEnvVariable(const gtString& envVariableName);
OS_API bool osGetCurrentProcessEnvVariableValue(const gtString& envVariableName, gtString& envVariableValue);
OS_API bool osExpandEnvironmentStrings(const gtString& envVariableValue, gtString& expandedEnvVariableValue);
OS_API osProcessId osGetCurrentProcessId();
OS_API void osExitCurrentProcess(int exitCode);

OS_API bool osLaunchSuspendedProcess(const osFilePath& executablePath, const gtString& arguments,
                                     const osFilePath& workDirectory, osProcessId& processId, osProcessHandle& processHandle,
                                     osThreadHandle& processThreadHandle, bool createWindow = true, bool redirectFiles = false);
OS_API bool osResumeSuspendedProcess(const osProcessId& processId, const osProcessHandle& processHandle,
                                     const osThreadHandle& processThreadHandle, bool closeHandles);

OS_API bool osWaitForProcessToTerminate(osProcessId processId, unsigned long timeoutMsec, long* pExitCode = NULL, bool child = true);
OS_API bool osIsProcessAlive(osProcessId processId, bool& isAliveBuffer);
OS_API bool osTerminateProcess(osProcessId processId, long exitCode = 0);
OS_API void osCloseProcessRedirectionFiles();

OS_API bool osSetProcessAffinityMask(osProcessId processId, const osProcessHandle processHandle, gtUInt64 affinityMask);

OS_API bool osTerminateChildren(osProcessId processId);

OS_API bool osGetProcessType(osProcessId processId, osModuleArchitecture& arch, osRuntimePlatform& platform, bool setPrivilege = true);

OS_API bool osGetProcessLaunchInfo(osProcessId processId,
                                   osModuleArchitecture& arch, osRuntimePlatform& platform,
                                   gtString& executablePath, gtString& commandLine, gtString& workDirectory,
                                   bool setPrivilege = true);

OS_API bool osIsProcessAttachable(osProcessId processId);

OS_API bool osExecAndGrabOutput(const char* cmd, const bool& cancelSignal, gtString& cmdOutput);

class OS_API osProcessesEnumerator
{
public:
    osProcessesEnumerator();
    ~osProcessesEnumerator();

    bool initialize();
    void deinitialize();

    bool next(osProcessId& processId, gtString* pName = NULL);

private:
    void* m_pEnumHandler;

#if GR_BUILD_TARGET == GR_WINDOWS_OS
    PROCESSENTRY32 m_pe32;
#endif
};

// Mac OS X only:
#if ((GR_BUILD_TARGET == GR_LINUX_OS) && (GR_LINUX_VARIANT == GR_MAC_OS_X_LINUX_VARIANT))
    OS_API bool osShowOrHideProcess(osProcessId processId, bool showProcess);
    OS_API bool osTransformProcessToForegroundApplication(osProcessId processId);
#endif

#if (GR_BUILD_TARGET == GR_WINDOWS_OS)
OS_API bool osGetRegistryKeyValue(HKEY hKey, const gtString& keyPath, const gtString& keyName, gtString& keyValue);
#else
OS_API bool osGetProcessIdentificationInfo(osProcessId& processId,
                                           osProcessId* pParentProcessId = NULL,
                                           osProcessId* pGroupId = NULL,
                                           char* pName = NULL,
                                           gtSize_t* pNameLen = NULL);
#endif

#endif  // __OSPROCESS
