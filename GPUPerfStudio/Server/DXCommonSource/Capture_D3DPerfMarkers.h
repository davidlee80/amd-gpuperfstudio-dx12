//=====================================================================
//
/// \author GPU Developer Tools
//
// Captures the PerfMarkers metadata info
//
//=====================================================================
//
// Last checkin:  $DateTime: 2015/12/08 13:22:59 $
// Last edited by: $Author: gselley $
//=====================================================================
//   (c) 2013 Advanced Micro Devices, Inc.
//=====================================================================
#ifndef CAPTUREPERFMARKERS_H
#define CAPTUREPERFMARKERS_H

#if defined (_WIN32)
    #include <windows.h>

    //-----------------------------------------------------------
    /// hooks all the D3D PerfMarkers calls into functions that
    /// will capture these calls and will add them to a list
    //-----------------------------------------------------------
    LONG CAPTURE_D3DPerfMarkers_Hook(CaptureLayer* pCaptureLayer);

    //-----------------------------------------------------------
    /// Detaches the capturing functions
    //-----------------------------------------------------------
    LONG CAPTURE_D3DPerfMarkers_Unhook();
#endif //_WIN32

#endif //ID3D10LOGGERIFACES_H