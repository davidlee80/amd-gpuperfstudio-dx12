//=====================================================================
// Copyright 2016 (c), Advanced Micro Devices, Inc. All rights reserved.
//
/// \author AMD Developer Tools Team
/// \file osAtomic.h 
/// 
//=====================================================================
#ifndef __OSATOMIC
#define __OSATOMIC

#include <AMDTBaseTools/Include/gtDefinitions.h>

#if (GR_BUILD_TARGET == GR_WINDOWS_OS)
    #include "../src/win32/osAtomicImpl.h"
#elif (GR_BUILD_TARGET == GR_LINUX_OS)
    #if (GR_LINUX_VARIANT == GR_GENERIC_LINUX_VARIANT)
        #include "../src/linux/osAtomicImpl.h"
    #else
        #       error Error: no atomic operations defined for build target!
    #endif
#endif

#endif // __OSATOMIC
