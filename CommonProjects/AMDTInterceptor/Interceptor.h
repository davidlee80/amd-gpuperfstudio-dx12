//==============================================================================
// Copyright (c) 2015 Advanced Micro Devices, Inc. All rights reserved.
/// \author AMD Developer Tools Team
/// \file
/// \brief  Interceptor Interface function declarations
//==============================================================================

#ifndef _INTERCEPTOR_H
#define _INTERCEPTOR_H

#include <Windows.h>

namespace AMDT
{
LONG BeginHook();
LONG EndHook();

// Hook an API call.
LONG WINAPI HookAPICall(_Inout_ PVOID *ppRealFn, _In_ PVOID pMineFn);

// Unhook an API call.
LONG WINAPI UnhookAPICall(_Inout_ PVOID *ppRealFn, _In_ PVOID pMineFn);

// Create a process and inject a dll into it
BOOL WINAPI CreateProcessAndInjectDllA(_In_opt_ LPCSTR lpApplicationName,
    _Inout_opt_ LPSTR lpCommandLine,
    _In_opt_ LPSECURITY_ATTRIBUTES lpProcessAttributes,
    _In_opt_ LPSECURITY_ATTRIBUTES lpThreadAttributes,
    _In_ BOOL bInheritHandles,
    _In_ DWORD dwCreationFlags,
    _In_opt_ LPVOID lpEnvironment,
    _In_opt_ LPCSTR lpCurrentDirectory,
    _In_ LPSTARTUPINFOA lpStartupInfo,
    _Out_ LPPROCESS_INFORMATION lpProcessInformation,
    _In_ LPCSTR lpDllName);

// Create a process and inject a dll into it
BOOL WINAPI CreateProcessAndInjectDllW(_In_opt_ LPCWSTR lpApplicationName,
    _Inout_opt_  LPWSTR lpCommandLine,
    _In_opt_ LPSECURITY_ATTRIBUTES lpProcessAttributes,
    _In_opt_ LPSECURITY_ATTRIBUTES lpThreadAttributes,
    _In_ BOOL bInheritHandles,
    _In_ DWORD dwCreationFlags,
    _In_opt_ LPVOID lpEnvironment,
    _In_opt_ LPCWSTR lpCurrentDirectory,
    _In_ LPSTARTUPINFOW lpStartupInfo,
    _Out_ LPPROCESS_INFORMATION lpProcessInformation,
    _In_ LPCSTR lpDllName);

#ifdef UNICODE
#define CreateProcessAndInjectDll     CreateProcessAndInjectDllW
#else
#define CreateProcessAndInjectDll      CreateProcessAndInjectDllA
#endif // !UNICODE

// Call at the beginning of MicroDLL
bool InitHookDLL(DWORD reason_for_call);

}

// Legacy detours equivalents until new interface complete
// TODO: remove later

// called from microdll.def and can't be inlined
VOID CALLBACK DetourFinishHelperProcess(_In_ HWND,
    _In_ HINSTANCE,
    _In_ LPSTR,
    _In_ INT);

#endif // def _INTERCEPTOR_H