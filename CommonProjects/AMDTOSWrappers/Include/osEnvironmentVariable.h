//=====================================================================
// Copyright 2016 (c), Advanced Micro Devices, Inc. All rights reserved.
//
/// \author AMD Developer Tools Team
/// \file osEnvironmentVariable.h 
/// 
//=====================================================================

//------------------------------ osEnvironmentVariable.h ------------------------------

#ifndef __OSENVIRONMENTVARIABLE
#define __OSENVIRONMENTVARIABLE

// Infra:
#include <AMDTBaseTools/Include/gtString.h>

// Local:
#include <AMDTOSWrappers/Include/osEnvironmentVariable.h>
#include <AMDTOSWrappers/Include/osOSWrappersDLLBuild.h>

// ----------------------------------------------------------------------------------
// Class Name:           osEnvironmentVariable
// General Description: Represents an environment variable.
// Author:      AMD Developer Tools Team
// Creation Date:        6/9/2005
// ----------------------------------------------------------------------------------
struct OS_API osEnvironmentVariable
{
    osEnvironmentVariable(const gtString& varName, const gtString& varValue) : _name(varName), _value(varValue) {}
    osEnvironmentVariable() {}
    // The environment variable name:
    gtString _name;

    // The environment variable value:
    gtString _value;
};


#endif  // __OSENVIRONMENTVARIABLE
