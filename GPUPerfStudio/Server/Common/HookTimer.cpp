//==============================================================================
// Copyright (c) 2013-2015 Advanced Micro Devices, Inc. All rights reserved.
/// \author AMD Developer Tools Team
/// \file
/// \brief  TODO add documentation
//==============================================================================

#include "HookTimer.h"
#include <AMDTOSWrappers/Include/osSystemError.h>
#include <AMDTOSWrappers/Include/osProcess.h>

#ifdef _WIN32
    #pragma warning (disable: 4201) // suppress for next line only
    #include <Mmsystem.h>
    #pragma warning (default: 4201)
#else
    #include <dlfcn.h>                      // header required for dlsym()
    #include "WinDefs.h"
#endif

#include "Interceptor.h"
#include "OSWrappers.h"

#include "timer.h"
#include "SharedGlobal.h"

#ifdef _WIN32
    BOOL WINAPI Mine_QueryPerformanceCounter(LARGE_INTEGER* lpCount);
    DWORD WINAPI Mine_GetTickCount();
    DWORD WINAPI Mine_timeGetTime();

    QueryPerformanceCounter_type Real_QueryPerformanceCounter = QueryPerformanceCounter;
    GetTickCount_type Real_GetTickCount = GetTickCount;
    timeGetTime_type Real_timeGetTime = timeGetTime;
#else
    gettimeofday_type    Real_gettimeofday = NULL;
    ftime_type           Real_ftime = NULL;
    clock_gettime_type   Real_clock_gettime = NULL;
#endif // _WIN32

bool attachedViaHooks = false;

bool HookTimer()
{
    LogTrace(traceMESSAGE, "HookTimer()");

    // Set the RealPause bool and speed float from the command line or config file
    TimeControl::Singleton().SetRealPause(SG_GET_BOOL(OptionRealPause));
    TimeControl::Singleton().SetPlaySpeed(SG_GET_FLOAT(OptionSpeed));


    LONG error = NO_ERROR;
#ifdef _WIN32
    AMDT::BeginHook();
    error = AMDT::HookAPICall(&(PVOID&)Real_QueryPerformanceCounter, Mine_QueryPerformanceCounter);
    PsAssert(error == NO_ERROR);

    if (error != NO_ERROR)
    {
        LogTrace(traceMESSAGE, "HookTimer() false (QueryPerformanceCounter)");
        return false;
    }

    error = AMDT::HookAPICall(&(PVOID&)Real_GetTickCount, Mine_GetTickCount);
    PsAssert(error == NO_ERROR);

    if (error != NO_ERROR)
    {
        LogTrace(traceMESSAGE, "HookTimer() false (GetTickCount)");
        return false;
    }

    error = AMDT::HookAPICall(&(PVOID&)Real_timeGetTime, Mine_timeGetTime);
    PsAssert(error == NO_ERROR);

    if (error != NO_ERROR)
    {
        LogTrace(traceMESSAGE, "HookTimer() false (timeGetTime)");
        return false;
    }

    error = AMDT::EndHook();

    if (error == NO_ERROR)
    {
        attachedViaHooks = true;
    }

#else
    Real_gettimeofday = (gettimeofday_type)dlsym(RTLD_NEXT, "gettimeofday");
    Real_ftime = (ftime_type)dlsym(RTLD_NEXT, "ftime");
    Real_clock_gettime = (clock_gettime_type)dlsym(RTLD_NEXT, "clock_gettime");
#endif // _WIN32

    return error == NO_ERROR;
}

bool UnhookTimer()
{
    LogTrace(traceMESSAGE, "UnhookTimer()");

    if (attachedViaHooks)
    {
#ifdef _WIN32
        AMDT::BeginHook();
        LONG error = NO_ERROR;
        error = AMDT::UnhookAPICall(&(PVOID&)Real_QueryPerformanceCounter, Mine_QueryPerformanceCounter);
        PsAssert(error == NO_ERROR);

        if (error != NO_ERROR)
        {
            LogTrace(traceMESSAGE, "UnhookTimer() false (QueryPerformanceCounter)");
            return false;
        }

        error = AMDT::UnhookAPICall(&(PVOID&)Real_GetTickCount, Mine_GetTickCount);
        PsAssert(error == NO_ERROR);

        if (error != NO_ERROR)
        {
            LogTrace(traceMESSAGE, "UnhookTimer() false (GetTickCount)");
            return false;
        }

        error = AMDT::UnhookAPICall(&(PVOID&)Real_timeGetTime, Mine_timeGetTime);
        PsAssert(error == NO_ERROR);

        if (error != NO_ERROR)
        {
            LogTrace(traceMESSAGE, "UnhookTimer() false (timeGetTime)");
            return false;
        }

        error = AMDT::EndHook();

        if (error != NO_ERROR)
        {
            LogTrace(traceMESSAGE, "UnhookTimer() false (EndHook)");
            return false;
        }

#endif // _WIN32
    }

#ifdef _WIN32
    /* Hooking can mess with these function pointers - restore for consistency */
    Real_QueryPerformanceCounter = QueryPerformanceCounter;
    Real_GetTickCount = GetTickCount;
    Real_timeGetTime = timeGetTime;
#endif // _WIN32
    return true;
}

