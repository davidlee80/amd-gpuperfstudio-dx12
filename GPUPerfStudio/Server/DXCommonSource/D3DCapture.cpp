//=====================================================================
//
/// \author GPU Developer Tools
//
//
//=====================================================================
//
// Last checkin:  $DateTime: 2015/02/12 16:14:48 $
// Last edited by: $Author: gselley $
//=====================================================================
//   (c) 2015 Advanced Micro Devices, Inc.
//=====================================================================
#include <d3d11_1.h>
#include "D3DCapture.h"
#include "../Common/Logger.h"

long D3DCapture::m_sCaptureCount = 0;

D3DCapture::D3DCapture()
{
    m_pObj = NULL;

    // Debug variable used to give each capture an index value
    //m_sCaptureCount++;

    // Debug code used to locate which capture is a problem
    //if (m_sCaptureCount % 500 == 0)
    //{
    //    Log(logDEBUG, "gCaptureCount: %ld\n", m_sCaptureCount);
    //}
}

/// \return the interface
IUnknown* D3DCapture::GetInterface()
{
    return m_pObj;
}

/// \return the class type
CaptureClassType D3DCapture::GetClassType()
{
    return CCT_RegularClass;
}