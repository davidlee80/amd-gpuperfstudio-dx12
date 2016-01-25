//=====================================================================
//
/// \author GPU Developer Tools
//
//
//=====================================================================
//
// Last checkin:  $DateTime: 2015/05/26 12:22:57 $
// Last edited by: $Author: gselley $
//=====================================================================
//   (c) 2013 Advanced Micro Devices, Inc.
//=====================================================================
#ifndef D3DCAPTURE_H
#define D3DCAPTURE_H
#include <string>

#include "../common/Capture.h"
#include <d3d11_1.h>

//-----------------------------------------------------------
/// D3D Capture clases device from this class rather than from Capture
/// This class takes care of the interface pointer and is also able to return
/// The class type
//-----------------------------------------------------------
class D3DCapture : public Capture
{
public:

    /// D3D Interfaces pointer, could be D3DDeviceContext, D3DContext... any COM pointer
    IUnknown* m_pObj;

    static long m_sCaptureCount;

    /// Initializes variables to NULL
    D3DCapture();

    /// \return the interface
    IUnknown* GetInterface();

    /// \return the class type
    virtual CaptureClassType GetClassType();
};

#endif //D3DCAPTURE_H