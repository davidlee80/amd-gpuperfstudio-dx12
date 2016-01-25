//==============================================================================
// Copyright (c) 2015 Advanced Micro Devices, Inc. All rights reserved.
/// \author AMD Developer Tools Team
/// \file
/// \brief  Provides a set of utility functions for use in the DX12 server plugin.
//==============================================================================

#ifndef DX12UTILITIES_H
#define DX12UTILITIES_H

#include "DX12Defines.h"
#include "DX12DeepCopy.h"

namespace DX12Util
{
// A function pointer used in place of an enumeration writing function.
typedef const char* (WriteEnum_Hook)(DWORD inFlags);

//--------------------------------------------------------------------------
/// A custom GUID-printing implementation.
/// \param inGUID The GUID to append into a formatted string.
//--------------------------------------------------------------------------
char* PrintGUID(REFGUID inGUID);

//--------------------------------------------------------------------------
/// Print a REFIID into the output string.
/// \param inREFIID The REFIID to print.
/// \param ioREFIIDString A string containing the human-readable REFIID.
//--------------------------------------------------------------------------
void PrintREFIID(REFIID inREFIID, gtASCIIString& ioREFIIDString);

//--------------------------------------------------------------------------
/// Convert a DX12 bool into a string for display in client.
/// \param inBool The boolean variable to print.
/// \returns A string containing "true" or "false" based on the input.
//--------------------------------------------------------------------------
const char* PrintBool(BOOL inBool);

//--------------------------------------------------------------------------
/// Convert a numeric return value into a human-readable string.
/// \param inReturnValue A numeric return value. Could be an HRESULT, or a number.
//--------------------------------------------------------------------------
void PrintReturnValue(INT64 inReturnValue, gtASCIIString& ioReturnValue);

//--------------------------------------------------------------------------
/// Decompose the incoming packed flags into a pipe-separated string of enum strings.
/// \param inFlags A UINT instance where packed flags have been bitwise-OR'd into the variable.
/// \param ioFlagsString The string that the result will be packed into.
/// \param inWriteHook A pointer to the function responsible for writing the given enumeration into a string.
/// \param inMinFlag An enumeration member that controls which value to start decomposing flags from.
/// \param inMaxFlag An enumeration member that controls which value to stop decomposing flags from.
//--------------------------------------------------------------------------
void DecomposeFlags(DWORD inFlags, gtASCIIString& ioFlagsString, WriteEnum_Hook inWriteHook, DWORD inMinFlag, DWORD inMaxFlag);

//--------------------------------------------------------------------------
/// Surround the given "inElementContents" XML with a root element that includes an optional 'name' attribute.
/// \param inStructureName The name of the type of structure being serialized.
/// \param inElementContents The XML contents that will be surrounded by a new parent element.
/// \param inOptionalNameAttribute An optional string to insert as the 'name' attribute for the new parent element.
/// \returns An XML string containing a new root element with name 'inStructureName', surrounding the original XML in 'inElementContents'.
//--------------------------------------------------------------------------
gtASCIIString& SurroundWithNamedElement(const char* inStructureName, gtASCIIString& inElementContents, const char* inOptionalNameAttribute = NULL);
}

#endif // DX12UTILITIES_H
