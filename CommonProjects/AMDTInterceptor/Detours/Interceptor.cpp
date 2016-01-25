#ifndef _INTERCEPTOR_H
#define _INTERCEPTOR_H

#include <Windows.h>
#include <detours.h>

namespace AMDT
{
LONG BeginHook()
{
    LONG detoursError = DetourTransactionBegin();

    if (detoursError == NO_ERROR)
    {
        detoursError = DetourUpdateThread(GetCurrentThread());
    }

    return detoursError;
}

LONG EndHook()
{
    return DetourTransactionCommit();
}

// Hook an API call.
LONG WINAPI HookAPICall(_Inout_ PVOID* ppPointer, _In_ PVOID pDetour)
{
    return DetourAttach(ppPointer, pDetour);
}

// Unhook an API call.
LONG WINAPI UnhookAPICall(_Inout_ PVOID* ppPointer, _In_ PVOID pDetour)
{
    return DetourDetach(ppPointer, pDetour);
}

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
                                              _In_ LPCSTR lpDllName)
{
    return DetourCreateProcessWithDllExA(lpApplicationName, lpCommandLine, lpProcessAttributes, lpThreadAttributes, bInheritHandles,
                                         dwCreationFlags, lpEnvironment, lpCurrentDirectory, lpStartupInfo, lpProcessInformation, lpDllName, NULL);
}

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
                                              _In_ LPCSTR lpDllName)
{
    return DetourCreateProcessWithDllExW(lpApplicationName, lpCommandLine, lpProcessAttributes, lpThreadAttributes, bInheritHandles,
                                         dwCreationFlags, lpEnvironment, lpCurrentDirectory, lpStartupInfo, lpProcessInformation, lpDllName, NULL);
}

#ifdef UNICODE
#define CreateProcessAndInjectDll     CreateProcessAndInjectDllW
#else
#define CreateProcessAndInjectDll      CreateProcessAndInjectDllA
#endif // !UNICODE

// Call at the beginning of MicroDLL
bool InitHookDLL(DWORD reason_for_call)
{
    if (DetourIsHelperProcess())
    {
        return false;
    }

    if (reason_for_call == DLL_PROCESS_ATTACH)
    {
        DetourRestoreAfterWith();
    }
    return true;
}

}

#endif // def _INTERCEPTOR_H