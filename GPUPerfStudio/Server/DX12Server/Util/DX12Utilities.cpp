//==============================================================================
// Copyright (c) 2015 Advanced Micro Devices, Inc. All rights reserved.
/// \author AMD Developer Tools Team
/// \file
/// \brief  Provides a set of utility functions for use in the DX12 server plugin.
//==============================================================================

#include "DX12Utilities.h"
#include "../../Common/xml.h"

namespace DX12Util
{
//--------------------------------------------------------------------------
/// A custom GUID-printing implementation.
/// \param inGUID The GUID to append into a formatted string.
//--------------------------------------------------------------------------
char* PrintGUID(REFGUID inGUID)
{
    (void)inGUID;

    // @DX12TODO: Compare the incoming GUID to __uuidof(all DX12 interface types).
    // If match is found, use the interface name string instead of the numeric GUID.
    /*
    outGuidString.appendFormattedString("%08lX-%04hX-%04hX-%02hhX%02hhX-%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX",
        inGUID.Data1, inGUID.Data2, inGUID.Data3,
        inGUID.Data4[0], inGUID.Data4[1], inGUID.Data4[2], inGUID.Data4[3],
        inGUID.Data4[4], inGUID.Data4[5], inGUID.Data4[6], inGUID.Data4[7]);
    */

    return  "GUIDHERE";
}

//--------------------------------------------------------------------------
/// Print a REFIID into the output string.
/// \param inREFIID The REFIID to print.
/// \returns A string containing the human-readable REFIID.
//--------------------------------------------------------------------------
void PrintREFIID(REFIID inREFIID, gtASCIIString& ioREFIIDString)
{
    ioREFIIDString.appendFormattedString("%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
                                         inREFIID.Data1,
                                         inREFIID.Data2,
                                         inREFIID.Data3,
                                         inREFIID.Data4[0],
                                         inREFIID.Data4[1],
                                         inREFIID.Data4[2],
                                         inREFIID.Data4[3],
                                         inREFIID.Data4[4],
                                         inREFIID.Data4[5],
                                         inREFIID.Data4[6],
                                         inREFIID.Data4[7]);
    ioREFIIDString.toUpperCase();
}

//--------------------------------------------------------------------------
/// Convert a DX12 bool into a string for display in client.
/// \param inBool The boolean variable to print.
/// \returns A string containing "true" or "false" based on the input.
//--------------------------------------------------------------------------
const char* PrintBool(BOOL inBool)
{
    return (inBool == TRUE) ? "TRUE" : "FALSE";
}

//--------------------------------------------------------------------------
/// Convert a numeric return value into a human-readable string.
/// \param inReturnValue A numeric return value. Could be an HRESULT, or a number.
//--------------------------------------------------------------------------
void PrintReturnValue(INT64 inReturnValue, gtASCIIString& ioReturnValue)
{
    const char* resultString = NULL;

    switch (inReturnValue)
    {
        // *INDENT-OFF*  - don't let astyle indent this next section
        // @TODO: Can we handle the entire set of possible HRESULTs?
        PRINTENUMCASE(S_OK, resultString);
        // *INDENT-ON*  
    }

    // Handle cases where the HRESULT wasn't formatted into a string above.
    if (resultString == NULL)
    {

        if (inReturnValue == FUNCTION_RETURNS_VOID)
        {
            ioReturnValue.appendFormattedString("%s", "void");
        }
        else
        {
            ioReturnValue.appendFormattedString("%I64d", inReturnValue);
        }
    }
    else
    {
        ioReturnValue.appendFormattedString("%s", resultString);
    }
}

//--------------------------------------------------------------------------
/// Decompose the incoming packed flags into a pipe-separated string of enum strings.
/// \param inFlags A UINT instance where packed flags have been bitwise-OR'd into the variable.
/// \param ioFlagsString The string that the result will be packed into.
/// \param inWriteHook A pointer to the function responsible for writing the given enumeration into a string.
/// \param inMinFlag An enumeration member that controls which value to start decomposing flags from.
/// \param inMaxFlag An enumeration member that controls which value to stop decomposing flags from.
//--------------------------------------------------------------------------
void DecomposeFlags(DWORD inFlags, gtASCIIString& ioFlagsString, WriteEnum_Hook inWriteHook, DWORD inMinFlag, DWORD inMaxFlag)
{
    // Initialize the flag to the minimum enum value.
    DWORD currentFlag = inMinFlag;
    bool bFlagWritten = false;

    do
    {
        // If there's an overlap between the input flags and the current flag bit, append it to the output string.
        if ((currentFlag & inFlags) == currentFlag)
        {
            // Append a spacer between the new and previous flag string (but only if this isn't the first flag).
            if ((currentFlag != inMinFlag) && bFlagWritten)
            {
                ioFlagsString.append(" | ");
            }

            bFlagWritten = true;
            ioFlagsString.appendFormattedString("%s", inWriteHook(currentFlag));
        }

        // If the min flag is zero, we're going to loop forever. Increment to "1" to check the next bit, and we'll be able to shift to check the remaining flags.
        if (currentFlag == 0)
        {
            currentFlag = 1;
        }
        else
        {
            // Shift the current flag to the left to check the for the presence of the next flag.
            currentFlag = (currentFlag << 1);
        }
    }
    while (currentFlag <= inMaxFlag);
}

//--------------------------------------------------------------------------
/// Surround the given "inElementContents" XML with a root element that includes an optional 'name' attribute.
/// \param inStructureName The name of the type of structure being serialized.
/// \param inElementContents The XML contents that will be surrounded by a new parent element.
/// \param inOptionalNameAttribute An optional string to insert as the 'name' attribute for the new parent element.
/// \returns An XML string containing a new root element with name 'inStructureName', surrounding the original XML in 'inElementContents'.
//--------------------------------------------------------------------------
gtASCIIString& SurroundWithNamedElement(const char* inStructureName, gtASCIIString& inElementContents, const char* inOptionalNameAttribute)
{
    // If the name argument is non-null, we can insert it as a name attribute.
    if (inOptionalNameAttribute != NULL)
    {
        inElementContents = XMLAttrib(inStructureName, FormatText("name='%s'", inOptionalNameAttribute).asCharArray(), inElementContents.asCharArray());
    }
    else
    {
        inElementContents = XML(inStructureName, inElementContents.asCharArray());
    }

    return inElementContents;
}
}
