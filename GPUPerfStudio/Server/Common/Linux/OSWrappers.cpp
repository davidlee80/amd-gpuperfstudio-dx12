//==============================================================================
// Copyright (c) 2013-2015 Advanced Micro Devices, Inc. All rights reserved.
/// \author AMD Developer Tools Team
/// \file
/// \brief  Linux-Specific wrapper functions. Use a generic platform-agnostic function
///         to perform OS-Specific functionality
//==============================================================================

#include <sys/time.h>
#include "WinDefs.h"
#include "OSWrappers.h"
#include "Logger.h"
#include "misc.h"

#include <AMDTBaseTools/Include/gtVector.h>
#include <AMDTOSWrappers/Include/osSystemError.h>
#include <AMDTOSWrappers/Include/osProcess.h>
#include <AMDTOSWrappers/Include/osModuleArchitecture.h>
#include <AMDTOSWrappers/Include/osModule.h>
#include <AMDTOSWrappers/Include/osFilePath.h>

namespace OSWrappers
{

//--------------------------------------------------------------------------
/// Get whether an executable file is 32 or 64 bit. Used to verify if an
/// app is compatible with the 32 or 64 bit build of PerfStudio.
///
/// \param moduleName name of executable to test
/// \arch whether this executable is 32 or 64 bit, indirectly returned.
/// \return true if successful, in which case arch will contain a valid
///  value or false if error
//--------------------------------------------------------------------------
bool GetBinaryType(const char* moduleName, osModuleArchitecture* arch)
{
    gtVector<osModuleArchitecture> archList;
    gtString fileName;
    fileName.fromASCIIString(moduleName);
    osFilePath modulePath(fileName);

    if (modulePath.exists())
    {
        if (osGetModuleArchitectures(modulePath, archList))
        {
            *arch = archList[0];
            return true;
        }
    }

    arch = NULL;
    return false;
}

//--------------------------------------------------------------------------
/// Get the current thread handle
/// \return Handle to the current thread
//--------------------------------------------------------------------------
osThreadHandle GetCurrentThreadHandle()
{
    return osGetCurrentThreadHandle();
}

//--------------------------------------------------------------------------
/// Retrieves the current value of the high-resolution performance counter
///
/// \param lpPerformanceCount Pointer to a variable that receives the
///  current performance-counter value, in counts
/// \return true if successful, false if error.
//--------------------------------------------------------------------------
bool  QueryPerformanceCounter(LARGE_INTEGER* lpPerformanceCount)
{
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    *lpPerformanceCount = (GPS_TIMESTAMP)(ts.tv_sec * 1000000000) + (GPS_TIMESTAMP)ts.tv_nsec;
    return true;
}

//--------------------------------------------------------------------------
/// Displays a modal dialog box that contains a system icon, a set of
/// buttons, and a brief application-specific message, such as status or
/// error information.
///
/// \param text The message to be displayed. If the string consists of more
///  than one line, the lines can be separated using a carriage return
///  and/or linefeed character between each line.
/// \param caption The dialog box title. If this parameter is NULL, the
///  default title is Error.
/// \param flags The contents and behavior of the dialog box.
/// \return An integer value that indicates which button the user clicked
//--------------------------------------------------------------------------
int   MessageBox(const char* text, const char* caption, unsigned int flags)
{
    PS_UNREFERENCED_PARAMETER(caption);
    PS_UNREFERENCED_PARAMETER(flags);
    LogConsole(logMESSAGE, "%s\n", text);
    //   #warning IMPLEMENT ME!
    return 0;
}

//--------------------------------------------------------------------------
/// Write basic system and device info into the incoming string.
/// \param ioInfoString The string to be filled with formatted system information.
//--------------------------------------------------------------------------
void WriteSystemInfoString(std::string& outSystemInfo)
{
    // @TODO: Must implement this for Linux.
    outSystemInfo = "";
}

} // namespace OSWrappers

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
void OutputDebugString(const char* lpOutputString)
{
    PS_UNREFERENCED_PARAMETER(lpOutputString);
    //   printf("%s\n", lpOutputString);
}

