//=====================================================================
// Copyright 2016 (c), Advanced Micro Devices, Inc. All rights reserved.
//
/// \author AMD Developer Tools Team
/// \file osFileLauncherThread.cpp 
/// 
//=====================================================================

//------------------------------ osFileLauncherThread.cpp ------------------------------

// Local:
#include <common/osFileLauncherThread.h>
#include <AMDTOSWrappers/Include/osFileLauncher.h>


// ---------------------------------------------------------------------------
// Name:        osFileLauncherThread::osFileLauncherThread
// Description: Constructor
// Arguments:   fileToBeLaunched - The path / URL of the file / HTML page to be launched.
// Author:      AMD Developer Tools Team
// Date:        28/3/2005
// ---------------------------------------------------------------------------
osFileLauncherThread::osFileLauncherThread(const gtString& fileToBeLaunched)
    : osThread(L"osFileLauncherThread"), _fileToBeLaunched(fileToBeLaunched)
{
}


// ---------------------------------------------------------------------------
// Name:        osFileLauncherThread::entryPoint
// Description: Does this class work - Launch the file / HTML page.
// Return Val:  int - 1 - Success.
//                    0 - Failure
// Author:      AMD Developer Tools Team
// Date:        28/3/2005
// ---------------------------------------------------------------------------
int osFileLauncherThread::entryPoint()
{
    int retVal = 0;

    // Launch the URL using the shell:
    osFileLauncher fileLaucher(_fileToBeLaunched, false);
    bool rc = fileLaucher.launchFile();

    if (rc)
    {
        retVal = 1;
    }

    return retVal;
}


// ---------------------------------------------------------------------------
// Name:        osFileLauncherThread::beforeTermination
// Description: This class is responsible for deleting itself. We do this
//              before we are terminated.
// Return Val:  bool - Success / failure.
// Author:      AMD Developer Tools Team
// Date:        28/3/2005
// ---------------------------------------------------------------------------
void osFileLauncherThread::beforeTermination()
{

}


