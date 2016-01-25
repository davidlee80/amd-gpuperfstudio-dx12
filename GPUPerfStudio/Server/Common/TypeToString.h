//==============================================================================
// Copyright (c) 2013-2015 Advanced Micro Devices, Inc. All rights reserved.
/// \author AMD Developer Tools Team
/// \file
/// \brief  Functions to convert value to string
//==============================================================================

#ifndef TYPETOSTRING_H
#define TYPETOSTRING_H

#include <AMDTBaseTools/Include/gtASCIIString.h>

#include "misc.h"

inline gtASCIIString BoolToString(DWORD bVal)
{
    BOOL b = (BOOL)bVal;
    return b ? "TRUE" : "FALSE";
}

inline gtASCIIString FloatToString(float fVal)
{
    float* pF = (float*) & fVal;
    return FormatText("%f", *pF);
}

inline gtASCIIString Float4ToString(float fVal[4])
{
    return FormatText("%f, %f, %f, %f", fVal[ 0 ],  fVal[ 1 ], fVal[ 2 ], fVal[ 3 ]);
}

inline gtASCIIString FloatNToString(float* fVal, int N)
{
    if (N < 1)
    {
        return "N/A";
    }

    gtASCIIString out = FloatToString(fVal[0]);

    for (int j = 1; j < N; j++)
    {
        out += FormatText(", %f", fVal[j]);
    }

    return out;
}

inline gtASCIIString DWORDToString(DWORD dwVal)
{
    return FormatText("%u", dwVal);
}

inline gtASCIIString IntToString(INT nVal)
{
    return FormatText("%d", nVal);
}

inline gtASCIIString UINT8ToString(UINT8 ui8Val)
{
    return FormatText("0x%02x", ui8Val);
}

inline gtASCIIString UINTToString(UINT uVal)
{
    return FormatText("0x%x", uVal);
}
#endif // TYPETOSTRING_H
