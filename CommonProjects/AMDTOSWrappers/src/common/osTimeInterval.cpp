//=====================================================================
// Copyright 2016 (c), Advanced Micro Devices, Inc. All rights reserved.
//
/// \author AMD Developer Tools Team
/// \file osTimeInterval.cpp 
/// 
//=====================================================================

/// --------------------------------osTimeInterval.cpp-----------------------------------------



// Local:
#include <AMDTOSWrappers/Include/osTimeInterval.h>


osTimeInterval::osTimeInterval(const gtUInt64& intervalInNanoSeconds)
{
    m_timeIntervalNanoSeconds = intervalInNanoSeconds;
}

gtString osTimeInterval::AsString() const
{
    gtUInt64 timeInterval = m_timeIntervalNanoSeconds;
    gtUInt64 microseconds = timeInterval % 1000;
    timeInterval -= microseconds;
    gtUInt64 milliseconds = timeInterval % 1000000;
    timeInterval -= milliseconds;
    gtUInt64 seconds = timeInterval % 1000000000;
    timeInterval -= seconds;
    gtUInt64 minutes = timeInterval % (1000000000LL * 60L);
    timeInterval -= minutes;
    gtUInt64 hours = timeInterval / (1000000000LL * 60L * 60L);

    // HH:MM:SS:mmm.uuu
    gtString retVal;
    retVal.appendFormattedString(L"%0.2d:%0.2d:%0.2d:%0.3d.%0.3d", hours, minutes, seconds, milliseconds, microseconds);

    return retVal;
}
