//==============================================================================
// Copyright (c) 2015 Advanced Micro Devices, Inc. All rights reserved.
/// \author AMD Developer Tools Team
/// \file
/// \brief  TODO add documentation
//==============================================================================
 
#ifndef CAPTURE_CLASS_TYPES_H
#define CAPTURE_CLASS_TYPES_H

typedef enum
{
    CCT_RegularClass,
    CCT_DeviceContext,
    CCT_CommandList,
    CCT_SwapChain,
    CCT_PerfMarker,
    CCT_Buffer,
} CaptureClassType;

#endif // CAPTURE_CLASS_TYPES_H