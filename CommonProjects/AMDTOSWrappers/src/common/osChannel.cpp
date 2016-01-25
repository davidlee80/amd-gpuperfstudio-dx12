//=====================================================================
// Copyright 2016 (c), Advanced Micro Devices, Inc. All rights reserved.
//
/// \author AMD Developer Tools Team
/// \file osChannel.cpp 
/// 
//=====================================================================

//------------------------------ osChannel.cpp ------------------------------
//C++ standard
#include <memory>
// Infra:
#include <AMDTBaseTools/Include/gtAssert.h>

// Local:
#include <AMDTOSWrappers/Include/osChannel.h>
#include <AMDTOSWrappers/Include/osDebugLog.h>

// Default read and write time outs (applies when the sub class does not
// override defaultReadOperationTimeOut() and defaultWriteOperationTimeOut()):
#define OS_CHANNEL_DEFAULT_READ_TIMEOUT 15000
#define OS_CHANNEL_DEFAULT_WRITE_TIMEOUT 5000


// ---------------------------------------------------------------------------
// Name:        osChannel::osChannel
// Description: Constructor.
// Author:      AMD Developer Tools Team
// Date:        24/8/2005
// ---------------------------------------------------------------------------
osChannel::osChannel()
{
    // Initialize the read and write operations timeouts to be the default
    // timeouts for the specific sub-class:
    _readOperationTimeOut = defaultReadOperationTimeOut();
    _writeOperationTimeOut = defaultWriteOperationTimeOut();
}


// ---------------------------------------------------------------------------
// Name:        osChannel::~osChannel
// Description: Destructor
// Author:      AMD Developer Tools Team
// Date:        31/1/2004
// ---------------------------------------------------------------------------
osChannel::~osChannel()
{
}


// ---------------------------------------------------------------------------
// Name:        osChannel::setReadOperationTimeOut
// Description: Sets this channel read operation time out.
// Arguments:   timeout - The new timeout for this channel, measured in milliseconds.
// Author:      AMD Developer Tools Team
// Date:        24/8/2005
// ---------------------------------------------------------------------------
void osChannel::setReadOperationTimeOut(long timeout)
{
    // If the user requests the default timeout for this channel type:
    if (timeout == OS_CHANNEL_DEFAULT_TIME_OUT)
    {
        _readOperationTimeOut = defaultWriteOperationTimeOut();
    }
    else
    {
        _readOperationTimeOut = timeout;
    }
}


// ---------------------------------------------------------------------------
// Name:        osChannel::setWriteOperationTimeOut
// Description: Sets this channel write operation time out.
// Arguments:   timeout - The new timeout for this channel, measured in milliseconds.
// Author:      AMD Developer Tools Team
// Date:        24/8/2005
// ---------------------------------------------------------------------------
void osChannel::setWriteOperationTimeOut(long timeout)
{
    // If the user requests the default timeout for this channel type:
    if (timeout == OS_CHANNEL_DEFAULT_TIME_OUT)
    {
        _writeOperationTimeOut = defaultWriteOperationTimeOut();
    }
    else
    {
        _writeOperationTimeOut = timeout;
    }
}


// ---------------------------------------------------------------------------
// Name:        osChannel::defaultReadOperationTimeOut
// Description: Returns the default read operation time out (measured in milliseconds).
//              Sub-classes may override this method.
// Author:      AMD Developer Tools Team
// Date:        24/8/2005
// ---------------------------------------------------------------------------
long osChannel::defaultReadOperationTimeOut() const
{
    return OS_CHANNEL_DEFAULT_READ_TIMEOUT;
}


// ---------------------------------------------------------------------------
// Name:        osChannel::defaultWriteOperationTimeOut
// Description: Returns the default write operation time out (measured in milliseconds).
//              Sub-classes may override this method.
// Author:      AMD Developer Tools Team
// Date:        24/8/2005
// ---------------------------------------------------------------------------
long osChannel::defaultWriteOperationTimeOut() const
{
    return OS_CHANNEL_DEFAULT_WRITE_TIMEOUT;
}

// ---------------------------------------------------------------------------
// Name:        osChannel::readString
// Description: Write the string to the channel
// Arguments:   gtString& str
// Return Val:  bool - Success / failure.
// Author:      AMD Developer Tools Team
// Date:        7/9/2010
// ---------------------------------------------------------------------------
bool osChannel::writeString(const gtString& str)
{
    bool retVal = false;
    string converted_str = str.asASCIICharArray();
    gtInt32 stringLength = static_cast<gtInt32>(converted_str.length());

    // Do not write the string length into text channels:
    if (channelType() == osChannel::OS_BINARY_CHANNEL)
    {
        // Write the string length:
        (*this) << stringLength;
    }

    // Write the string content:
    if (stringLength > 0)
    {
        // Write the multi byte char pointer:
        bool rc = write(converted_str.c_str(), stringLength);
        GT_IF_WITH_ASSERT(rc)
        {
            retVal = true;
        }
    }
    else
    {
        retVal = true;
    }

    return retVal;
}


// ---------------------------------------------------------------------------
// Name:        osChannel::readString
// Description: Write the string to the channel - ASCII version
// Arguments:   gtASCIIString& str
// Return Val:  bool - Success / failure.
// Author:      AMD Developer Tools Team
// Date:        13/9/2010
// ---------------------------------------------------------------------------
bool osChannel::writeString(const gtASCIIString& str)
{
    bool retVal = false;

    // Writing an ASCII string to a unicode channel is not allowed:
    GT_IF_WITH_ASSERT(channelType() != osChannel::OS_UNICODE_TEXT_CHANNEL)
    {
        // Get the string length:
        gtInt32 stringLength = str.length();

        // Do not write the string length into text channels:
        if (channelType() == osChannel::OS_BINARY_CHANNEL)
        {
            // Write the string length:
            (*this) << str.length();
        }

        // Write the string content:
        if (stringLength > 0)
        {
            const char* pString = str.asCharArray();
            bool rc = write((const gtByte*)pString, stringLength);
            GT_IF_WITH_ASSERT(rc)
            {
                retVal = true;
            }
        }
        else
        {
            retVal = true;
        }
    }

    return retVal;
}

// ---------------------------------------------------------------------------
// Name:        osChannel::readString
// Description: Reads the string from the channel
// Arguments:   gtString& str
// Return Val:  bool - Success / failure.
// Author:      AMD Developer Tools Team
// Date:        7/9/2010
// ---------------------------------------------------------------------------
bool osChannel::readString(gtString& str)
{
    bool retVal = false;

    // We do not support reading from a text channel:
    if ((channelType() == osChannel::OS_ASCII_TEXT_CHANNEL) || (channelType() == osChannel::OS_UNICODE_TEXT_CHANNEL))
    {
        str = L"Error - operator>> is trying to read a string from a text channel !!";
        GT_ASSERT(0);
    }
    else
    {
        retVal = true;

        // Read the string length:
        gtInt32 stringLength = 0;
        (*this) >> stringLength;

        if (stringLength > 0)
        {
            // Read the string content:
            // Read the string as bytes pointer:
            // TO_DO: Unicode - use preprocessor definitions (http://www.firstobject.com/wchar_t-string-on-linux-osx-windows.htm)

            unique_ptr<gtByte[]> buf_ptr(new gtByte[stringLength + 1]());
            gtByte* buf = buf_ptr.get();


            bool rc = read(buf, stringLength);

            if (rc)
            {
                str.fromASCIIString(buf, stringLength);
            }
            else
            {
                // An error occurred:
                GT_ASSERT(0);
                retVal = false;
            }
        }
        else
        {
            // This is an empty string:
            gtString emptyString;
            str = emptyString;
        }
    }

    return retVal;
}


// ---------------------------------------------------------------------------
// Name:        osChannel::readString
// Description: Reads the string from the channel - ASCII version
// Arguments:   gtASCIIString& str
// Return Val:  bool - Success / failure.
// Author:      AMD Developer Tools Team
// Date:        13/9/2010
// ---------------------------------------------------------------------------
bool osChannel::readString(gtASCIIString& str)
{
    bool retVal = false;

    // We do not support reading from a text channel:
    if ((channelType() == osChannel::OS_ASCII_TEXT_CHANNEL) || (channelType() == osChannel::OS_UNICODE_TEXT_CHANNEL))
    {
        str = "Error - operator>> is trying to read a string from a text channel !!";
        GT_ASSERT(0);
    }
    else
    {
        retVal = true;

        // Read the string length:
        gtInt32 stringLength = 0;
        (*this) >> stringLength;

        if (stringLength > 0)
        {
            // Read the string content:
            int bufferSize = stringLength + 1;
            char* pStringContent = new char[bufferSize];


            // Read the string as bytes pointer:
            bool rc = read((gtByte*)pStringContent, stringLength);

            // Put null at the end of the buffer:
            pStringContent[stringLength] = 0;

            if (rc)
            {
                str = ((const char*)pStringContent);
            }
            else
            {
                // An error occurred:
                GT_ASSERT(0);
                retVal = false;
            }

            delete[] pStringContent;
        }
        else
        {
            // This is an empty string:
            gtASCIIString emptyString;
            str = emptyString;
        }
    }

    return retVal;
}


