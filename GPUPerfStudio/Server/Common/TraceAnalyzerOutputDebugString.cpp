//==============================================================================
// Copyright (c) 2013-2015 Advanced Micro Devices, Inc. All rights reserved.
/// \author AMD Developer Tools Team
/// \file
/// \brief  Hooks the OutputDebugString function and adds the output to the API Trace
//==============================================================================

#if defined (_WIN32)
    #include <windows.h>
#endif
#include "Interceptor.h"
#include "Logger.h"
#include "misc.h"
#include "mymutex.h"
#include "TraceAnalyzerOutputDebugString.h"
#include "TraceAnalyzer.h"

typedef void (WINAPI* OutputDebugStringA_type)(LPCSTR lpOutputString);

static OutputDebugStringA_type Real_OutputDebugStringA = NULL;
static DWORD s_dwAttached = 0;
static mutex s_mtx;

/// The trace analyzer object which gets informed of the call to OutputDebugString
static TraceAnalyzer* s_pTraceAnalyzer = NULL;

//-----------------------------------------------------------
// Mine_OutputDebugStringA()
//-----------------------------------------------------------
static void WINAPI Mine_OutputDebugStringA(LPCSTR lpOutputString)
{
    ScopeLock t(&s_mtx);

    if (s_pTraceAnalyzer != NULL && s_pTraceAnalyzer->IsCollectingAPICalls())
    {
        s_pTraceAnalyzer->BeforeAPICall();
        s_pTraceAnalyzer->AddDebugString(UrlEncode(std::string(lpOutputString)));
    }

    Real_OutputDebugStringA(lpOutputString);
}


//-----------------------------------------------------------
// Attaches to the function
//-----------------------------------------------------------
long Hook_OutputDebugString(TraceAnalyzer* pTraceAnalyzer)
{
    LogTrace(traceENTER, "");

    ScopeLock t(&s_mtx);

    if (s_dwAttached > 0)
    {
        LogTrace(traceEXIT, "Trying to attach twice!\n");
        return -1;
    }

    LONG error;
    AMDT::BeginHook();

    Real_OutputDebugStringA = OutputDebugStringA;

    error = AMDT::HookAPICall(&(PVOID&)Real_OutputDebugStringA, Mine_OutputDebugStringA);
    PsAssert(error == NO_ERROR);
    error = AMDT::EndHook();

    if (error != NO_ERROR)
    {
        Log(logERROR, "failed\n");
    }
    else
    {
        s_dwAttached++;
    }

    s_pTraceAnalyzer = pTraceAnalyzer;

    LogTrace(traceEXIT, "");

    return error;
}

//-----------------------------------------------------------
// Detaches from the function
//-----------------------------------------------------------
long Unhook_OutputDebugString()
{
    LogTrace(traceENTER, "");

    ScopeLock t(&s_mtx);

    if (s_dwAttached <= 0)
    {
        LogTrace(traceEXIT, "Trying to Detach from OutputDebugString twice!\n");
        return -1;
    }

    LONG error;
    AMDT::BeginHook();

    error = AMDT::UnhookAPICall(&(PVOID&)Real_OutputDebugStringA, Mine_OutputDebugStringA);
    PsAssert(error == NO_ERROR);

    error = AMDT::EndHook();

    if (error != NO_ERROR)
    {
        Log(logERROR, "failed\n");
    }
    else
    {
        s_dwAttached--;
    }

    LogTrace(traceEXIT, "");

    return error;
}

