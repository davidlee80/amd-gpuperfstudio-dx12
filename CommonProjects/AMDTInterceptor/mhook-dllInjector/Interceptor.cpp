//==============================================================================
// Copyright (c) 2015 Advanced Micro Devices, Inc. All rights reserved.
/// \author AMD Developer Tools Team
/// \file
/// \brief  Interceptor Interface function definitions
//==============================================================================

#include <Windows.h>
#include "Interceptor.h"
#include "mhook-lib/mhook.h"

LONG AMDT::BeginHook()
{
    Mhook_SuspendOtherThreads();
    return 0;
}

LONG AMDT::EndHook()
{
    Mhook_ResumeOtherThreads();
    return 0;
}

LONG WINAPI AMDT::HookAPICall(_Inout_ PVOID* ppRealFn, _In_ PVOID pMineFn)
{
    if (Mhook_SetHook(ppRealFn, pMineFn) == TRUE)
    {
        return 0;
    }

    return 1;
}

// Unhook an API call.
LONG WINAPI AMDT::UnhookAPICall(_Inout_ PVOID* ppRealFn, _In_ PVOID pMineFn)
{
    UNREFERENCED_PARAMETER(pMineFn);

    if (Mhook_Unhook(ppRealFn) == TRUE)
    {
        return 0;
    }

    return 1;
}

bool AMDT::InitHookDLL(DWORD reason_for_call)
{
    UNREFERENCED_PARAMETER(reason_for_call);
    return true;
}

VOID CALLBACK DetourFinishHelperProcess(_In_ HWND,
                                        _In_ HINSTANCE,
                                        _In_ LPSTR,
                                        _In_ INT)
{
}
