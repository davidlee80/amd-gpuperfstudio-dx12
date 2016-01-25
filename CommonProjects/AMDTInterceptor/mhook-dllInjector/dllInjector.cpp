#include <stdlib.h>
#include <windows.h>
#include <stdio.h>

#pragma comment(lib,"Advapi32.lib")

static LPCSTR static_LoadLibraryFunctionName =
#ifdef UNICODE
"LoadLibraryW";
#else
"LoadLibraryA";
#endif

// size of buffer required to store launcher command line
static const int COMMAND_LINE_SIZE = 1024;

#ifdef DEBUG_PRINT
#define DEBUG_OUTPUT(x) printf x
#else
#define DEBUG_OUTPUT(x)
#endif

class Injector
{
public:
    // constructor
    explicit Injector(LPCSTR dllPath)
        : m_dllPath(dllPath)
    {
    }

    // destructor
    ~Injector() { }

    // ---------------------------------------------------------------------------
    /// Does all the work in this class - loads the DLL into the target process.
    // ---------------------------------------------------------------------------
    bool Execute(DWORD processID)
    {
        m_hTargetProcHandle = OpenProcess(PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ, FALSE, processID);

        bool retVal = InjectDLLPathIntoTargetProcess();
        if (retVal)
        {
            retVal = LoadDLLIntoTargetProcess();
        }
        return retVal;
    }

private:
    // ---------------------------------------------------------------------------
    /// Injects the DLL path into the target process.
    // ---------------------------------------------------------------------------
    bool InjectDLLPathIntoTargetProcess()
    {
        bool retVal = false;
        m_sizeOfMemoryToInject = strlen(m_dllPath);
        if (m_sizeOfMemoryToInject == 0)
        {
            DEBUG_OUTPUT(("ERROR: dllPath length is 0\n"));
            return retVal;
        }

        retVal = AllocateSpaceForInjectedMemory();

        if (retVal)
        {
            retVal = InjectMemory();
        }

        return retVal;
    }

    // ---------------------------------------------------------------------------
    /// Allocated memory that will contain the injected memory
    /// This memory will be accessible by the target process.
    // ---------------------------------------------------------------------------
    bool AllocateSpaceForInjectedMemory()
    {
        bool retVal = false;

        DEBUG_OUTPUT(("Allocating space for the path of the DLL\n"));

        m_pTargetMemoryAddress = VirtualAllocEx(m_hTargetProcHandle, NULL, m_sizeOfMemoryToInject, MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE);
        if (m_pTargetMemoryAddress == NULL)
        {
            DEBUG_OUTPUT(("ERROR: VirtualAllocEx Failed [%u]\n", GetLastError()));
        }
        else
        {
            retVal = true;
        }
        return retVal;
    }

    // ---------------------------------------------------------------------------
    /// Injects the memory into the target process.
    /// This function should be called after allocateSpaceForInjectedMemory()
    /// was called.
    // ---------------------------------------------------------------------------
    bool InjectMemory()
    {
        bool retVal = false;

        // Verify that we have allocated space for the memory to inject:
        if (m_pTargetMemoryAddress)
        {
            // Inject the memory into the target process:
            SIZE_T numberOfBytesWritten = 0;

            int rc = WriteProcessMemory(m_hTargetProcHandle, m_pTargetMemoryAddress, m_dllPath, m_sizeOfMemoryToInject, &numberOfBytesWritten);
            retVal = ((rc != 0) && (numberOfBytesWritten == m_sizeOfMemoryToInject));

            if (retVal == false)
            {
                DEBUG_OUTPUT(("ERROR: WriteProcessMemory Failed [%u]\n", GetLastError()));
            }
        }
        return retVal;
    }

    // ---------------------------------------------------------------------------
    /// Loads the DLL into the target process.
    // ---------------------------------------------------------------------------
    bool LoadDLLIntoTargetProcess()
    {
        bool retVal = false;

        DEBUG_OUTPUT(("Looking for LoadLibrary in kernel32\n"));
        HMODULE handle = GetModuleHandle(TEXT("kernel32.dll"));
        FARPROC fpLoadLibrary = NULL;
        if (handle != NULL)
        {
            fpLoadLibrary = GetProcAddress(handle, static_LoadLibraryFunctionName);
            if (fpLoadLibrary == NULL)
            {
                DEBUG_OUTPUT(("ERROR: Failed to find LoadLibrary in Kernel32. Exiting...\n"));
                return retVal;
            }
        }
        else
        {
            DEBUG_OUTPUT(("ERROR: Failed to get module handle for Kernel32. Exiting...\n"));
            return retVal;
        }

        DEBUG_OUTPUT(("\t\tFound at %p\n", fpLoadLibrary));

        HANDLE remoteThreadHandle = NULL;
        DWORD remoteThreadId = 0;

        // Create a remote thread that loads the DLL into the target process:
        remoteThreadHandle = CreateRemoteThread(m_hTargetProcHandle, NULL, 0, (LPTHREAD_START_ROUTINE)fpLoadLibrary, m_pTargetMemoryAddress, 0, &remoteThreadId);

        if (remoteThreadHandle == NULL)
        {
            DEBUG_OUTPUT(("ERROR: CreateRemoteThread Failed [%u]. Exiting....\n", GetLastError()));
            return retVal;
        }
        else
        {
            // Wait for the thread to end its task:
            WaitForSingleObject(remoteThreadHandle, INFINITE);

            // Get the thread exit code:
            // (This is actually the LoadLibrary return code)
            DWORD threadExitCode = 0;
            BOOL rc = GetExitCodeThread(remoteThreadHandle, (LPDWORD)&threadExitCode);

            if (rc)
            {
                // If the remote LoadLibrary succeeded:
                if (threadExitCode != NULL)
                {
                    retVal = true;
                }
            }

            // Clean up:
            CloseHandle(remoteThreadHandle);
        }

        return retVal;
    }

    // The handle of the process into which we will inject the memory:
    HANDLE m_hTargetProcHandle;

    // name of the dll to inject
    LPCSTR m_dllPath;

    // The size of the memory to be injected:
    size_t m_sizeOfMemoryToInject;

    // The address of the injected memory (In target process address space):
    void* m_pTargetMemoryAddress;
};

BOOL InjectDLLIntoProcess(const char* lpDllName, DWORD processID)
{
    Injector injector(lpDllName);
    if (injector.Execute(processID) == false)
    {
        return FALSE;
    }
    return TRUE;
}

namespace AMDT
{
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
    // Get bitness of current module
    char currentFileName[MAX_PATH];
    GetModuleFileNameA(NULL, currentFileName, MAX_PATH);

    DEBUG_OUTPUT(("CreateProcessAndInjectDllA running from process name >>%s<<\n", currentFileName));

    DWORD currentType;
    GetBinaryTypeA(currentFileName, &currentType);

    // Get bitness of application name (if it isn't NULL)
    DWORD appType = currentType;

    if (lpApplicationName != NULL && strlen(lpApplicationName) > 0)
    {
        DEBUG_OUTPUT(("CreateProcessAndInjectDllA: application name >>%s<<\n", lpApplicationName));
        GetBinaryTypeA(lpApplicationName, &appType);
    }
    else
    {
        // if there's no application name, then get it from the command line.
        // Take into account whether the command line is in quotes and ignore spaces if so
        char buffer[MAX_PATH];
        buffer[0] = '\0';

        // if the command line string contains a '.exe' string, assume this is the application name and use that
        const char* pExe = strstr(lpCommandLine, ".exe");

        if (pExe != NULL)
        {
            // find length of string up to start of '.exe'
            int length = (int)(pExe - lpCommandLine);

            // add length of '.exe'
            length += 4;

            // copy the application name from the command line string, ignoring '"' characters
            int index = 0;

            for (int loop = 0; loop < length; loop++)
            {
                char currentChar = lpCommandLine[loop];

                if (currentChar != '\"')
                {
                    buffer[index++] = currentChar;
                }

                buffer[index] = '\0';
            }
        }
        else
        {
            int insideQuotes = 0;
            int index = 0;

            for (int loop = 0; loop < MAX_PATH; loop++)
            {
                char currentChar = lpCommandLine[loop];

                if (currentChar == '\"')
                {
                    // it's a '"', so toggle the quote flag. Don't copy character to the buffer
                    insideQuotes = 1 - insideQuotes;   // toggle value 0 -> 1, 1 -> 0
                }
                else if (currentChar == ' ')
                {
                    // it's a space character, so decide what to do. If inside a quoted string, treat it as a normal
                    // character. Otherwise, use it as a string terminator
                    if (insideQuotes == 0)
                    {
                        buffer[index] = '\0';
                        break;
                    }
                    else
                    {
                        buffer[index++] = currentChar;
                    }
                }
                else
                {
                    // copy the character
                    buffer[index++] = currentChar;
                }
            }
        }

        if (buffer != NULL)
        {
            DEBUG_OUTPUT(("CreateProcessAndInjectDllA: command line exe >>%s<<\n", buffer));
            GetBinaryTypeA(buffer, &appType);
        }
    }

    BOOL result = FALSE;

    // Create the process suspended
    DWORD dwFlags = dwCreationFlags | CREATE_SUSPENDED;
    result = CreateProcessA(lpApplicationName, lpCommandLine, lpProcessAttributes, lpThreadAttributes, bInheritHandles, dwFlags,
        lpEnvironment, lpCurrentDirectory, lpStartupInfo, lpProcessInformation);

    if (result == TRUE)
    {
        if (appType == currentType)
        {
            // This module and the application to be created are the same bitness, so just inject the dll into the new process
            // and resume it when done
            InjectDLLIntoProcess(lpDllName, lpProcessInformation->dwProcessId);
            ResumeThread(lpProcessInformation->hThread);
        }
        else
        {
            // This module and the application to be created have different bitness, so start a launcher process.
            // The launcher process will have the same bitness as the target app so will be able to inject the
            // dll into it
            DEBUG_OUTPUT(("CreateProcessAndInjectDllA: starting launcher\n"));

            // Get path of dll to get launcher process name.
            char dllPath[MAX_PATH];
            strcpy_s(dllPath, lpDllName);
            size_t index = strlen(dllPath);
            bool found = false;

            while (found == false && index > 0)
            {
                if (dllPath[index] == '\\')
                {
                    dllPath[index] = '\0';
                    found = true;
                }

                index--;
            }

            char launcherPath[MAX_PATH];
			ZeroMemory(launcherPath, MAX_PATH * sizeof(char));

            if (appType == SCS_64BIT_BINARY)
            {
                sprintf_s(launcherPath, "%s\\Launcher-x64.exe", dllPath);
            }
            else
            {
                sprintf_s(launcherPath, "%s\\Launcher.exe", dllPath);
            }

            // build the launcher command line args
            static char args[COMMAND_LINE_SIZE] = { 0 };
            sprintf_s(args, "%s %d", lpDllName, lpProcessInformation->dwProcessId);

            // create the launcher and wait for it to exit
            dwFlags = dwCreationFlags & (~CREATE_SUSPENDED);

            PROCESS_INFORMATION pi;
            ZeroMemory(&pi, sizeof(pi));

            STARTUPINFO si;
            ZeroMemory(&si, sizeof(si));
            si.cb = sizeof(si);

            result = CreateProcessA(launcherPath, args, NULL, NULL, TRUE, dwFlags,
                lpEnvironment, lpCurrentDirectory, &si, &pi);

            if (result == TRUE)
            {
                // Wait until launcher process exits.
                WaitForSingleObject(pi.hProcess, INFINITE);

                // Close process and thread handles. 
                CloseHandle(pi.hProcess);
                CloseHandle(pi.hThread);

                // resume the target application thread
                ResumeThread(lpProcessInformation->hThread);
            }
            else
            {
                DEBUG_OUTPUT(("CreateProcessAndInjectDllA failed to start launcher (%u).\n", GetLastError()));
            }
        }
    }

    return result;
}

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
    DEBUG_OUTPUT(("CreateProcessAndInjectDllW\n"));

    // Get bitness of current module
    WCHAR currentFileName[MAX_PATH];
    GetModuleFileNameW(NULL, currentFileName, MAX_PATH);
    DWORD currentType;
    GetBinaryTypeW(currentFileName, &currentType);

    // Get bitness of application name
    DWORD appType = currentType;

    if (lpApplicationName != NULL && lstrlenW(lpApplicationName) > 0)
    {
        DEBUG_OUTPUT(("CreateProcessAndInjectDllW: application name >>%S<<\n", lpApplicationName));
        GetBinaryTypeW(lpApplicationName, &appType);
    }
    else
    {
        // if there's no application name, then get it from the command line.
        // Take into account whether the command line is in quotes and ignore spaces if so
        WCHAR buffer[MAX_PATH];
        buffer[0] = '\0';

        // if the command line string contains a '.exe' string, assume this is the application name and use that
        const WCHAR* pExe = wcsstr(lpCommandLine, L".exe");

        if (pExe != NULL)
        {
            // find length of string up to start of '.exe'
            int length = (int)(pExe - lpCommandLine);

            // add length of '.exe'
            length += 4;

            // copy the application name from the command line string, ignoring '"' characters
            int index = 0;

            for (int loop = 0; loop < length; loop++)
            {
                WCHAR currentChar = lpCommandLine[loop];

                if (currentChar != '\"')
                {
                    buffer[index++] = currentChar;
                }

                buffer[index] = '\0';
            }
        }
        else
        {
            int insideQuotes = 0;
            int index = 0;

            for (int loop = 0; loop < MAX_PATH; loop++)
            {
                WCHAR currentChar = lpCommandLine[loop];

                if (currentChar == '\"')
                {
                    // it's a '"', so toggle the quote flag. Don't copy character to the buffer
                    insideQuotes = 1 - insideQuotes;   // toggle value 0 -> 1, 1 -> 0
                }
                else if (currentChar == ' ')
                {
                    // it's a space character, so decide what to do. If inside a quoted string, treat it as a normal
                    // character. Otherwise, use it as a string terminator
                    if (insideQuotes == 0)
                    {
                        buffer[index] = '\0';
                        break;
                    }
                    else
                    {
                        buffer[index++] = currentChar;
                    }
                }
                else
                {
                    // copy the character
                    buffer[index++] = currentChar;
                }
            }
        }

        if (buffer != NULL)
        {
            DEBUG_OUTPUT(("CreateProcessAndInjectDllW: command line exe >>%S<<\n", buffer));
            GetBinaryTypeW(buffer, &appType);
        }
    }

    BOOL result = FALSE;
    // Create the process suspended
    DWORD dwFlags = dwCreationFlags | CREATE_SUSPENDED;
    result = CreateProcessW(lpApplicationName, lpCommandLine, lpProcessAttributes, lpThreadAttributes, bInheritHandles, dwFlags,
        lpEnvironment, lpCurrentDirectory, lpStartupInfo, lpProcessInformation);

    if (result == TRUE)
    {
        if (appType == currentType)
        {
            // This module and the application to be created are the same bitness, so just inject the dll into the new process
            // and resume it when done
            InjectDLLIntoProcess(lpDllName, lpProcessInformation->dwProcessId);
            ResumeThread(lpProcessInformation->hThread);
        }
        else
        {
            // This module and the application to be created have different bitness, so start a launcher process.
            // The launcher process will have the same bitness as the target app so will be able to inject the
            // dll into it
            DEBUG_OUTPUT(("CreateProcessAndInjectDllW: starting launcher\n"));

            // Get path of dll to get launcher process name.
            char dllPath[MAX_PATH];
            strcpy_s(dllPath, lpDllName);
            size_t index = strlen(dllPath);
            bool found = false;

            while (found == false && index > 0)
            {
                if (dllPath[index] == '\\')
                {
                    dllPath[index] = '\0';
                    found = true;
                }

                index--;
            }

            WCHAR launcherPath[MAX_PATH];
			ZeroMemory(launcherPath, MAX_PATH * sizeof(WCHAR));

            if (appType == SCS_64BIT_BINARY)
            {
                swprintf_s(launcherPath, L"%S\\Launcher-x64.exe", dllPath);
            }
            else
            {
                swprintf_s(launcherPath, L"%S\\Launcher.exe", dllPath);
            }

            // build the command line args as a wide string
            static char args[COMMAND_LINE_SIZE] = { 0 };
            sprintf_s(args, "%s %d", lpDllName, lpProcessInformation->dwProcessId);
            static WCHAR wargs[COMMAND_LINE_SIZE] = { 0 };

			size_t pReturnValue;
			pReturnValue = 0;
			errno_t error;
			error = mbstowcs_s(&pReturnValue, wargs, COMMAND_LINE_SIZE, args, COMMAND_LINE_SIZE);

            // create the launcher and wait for it to exit
            dwFlags = dwCreationFlags & (~CREATE_SUSPENDED);

            PROCESS_INFORMATION pi;
            ZeroMemory(&pi, sizeof(pi));

            STARTUPINFOW si;
            ZeroMemory(&si, sizeof(si));
            si.cb = sizeof(si);

            result = CreateProcessW(launcherPath, wargs, NULL, NULL, TRUE, dwFlags,
                lpEnvironment, lpCurrentDirectory, &si, &pi);

            if (result == TRUE)
            {
                // Wait until launcher process exits.
                WaitForSingleObject(pi.hProcess, INFINITE);

                // Close process and thread handles. 
                CloseHandle(pi.hProcess);
                CloseHandle(pi.hThread);

                // resume the target application thread
                ResumeThread(lpProcessInformation->hThread);
            }
            else
            {
                DEBUG_OUTPUT(("CreateProcessAndInjectDllW failed to start launcher (%u).\n", GetLastError()));
            }
        }
    }

    return result;
}

} // namespace AMDT
