//=====================================================================
// Copyright 2016 (c), Advanced Micro Devices, Inc. All rights reserved.
//
/// \author AMD Developer Tools Team
/// \file osChannel.h 
/// 
//=====================================================================

//------------------------------ osChannel.h ------------------------------

#ifndef __OSCHANNEL
#define __OSCHANNEL

// Local:
#include <AMDTOSWrappers/Include/osOSDefinitions.h>
#include <AMDTOSWrappers/Include/osChannelOperators.h>

// Infinite time out:
#define OS_CHANNEL_INFINIT_TIME_OUT LONG_MAX

// Use no time out (return immediately):
#define OS_CHANNEL_NO_TIME_OUT 0

// Use the per channel type default time out:
#define OS_CHANNEL_DEFAULT_TIME_OUT -1


// ----------------------------------------------------------------------------------
// Class Name:           osChannel
// General Description:
//   Base class for all channel types.
//   A channel enables communication between two entities (processes / algorithms / etc).
//   Data can be written into one side of the channel and read on the other side of
//   the channel.
//
// Author:      AMD Developer Tools Team
// Creation Date:        6/12/2003
// ----------------------------------------------------------------------------------
class OS_API osChannel
{
public:
    // Describes the channel type.
    enum osChannelType
    {
        OS_BINARY_CHANNEL,
        OS_ASCII_TEXT_CHANNEL,
        OS_UNICODE_TEXT_CHANNEL
    };

    // Self functions:
    osChannel();
    virtual ~osChannel();

    long readOperationTimeOut() const { return _readOperationTimeOut;};
    void setReadOperationTimeOut(long timeout);

    long writeOperationTimeOut() const { return _writeOperationTimeOut; };
    void setWriteOperationTimeOut(long timeout);

    // Can be overridden by sub-classes:
    virtual long defaultReadOperationTimeOut() const;
    virtual long defaultWriteOperationTimeOut() const;

    // Must be implemented by sub-classes:
    virtual osChannelType channelType() const = 0;
    virtual bool write(const gtByte* pDataBuffer, gtSize_t dataSize) = 0;
    virtual bool read(gtByte* pDataBuffer, gtSize_t dataSize) = 0;
    virtual bool readAvailableData(gtByte* pDataBuffer, gtSize_t bufferSize, gtSize_t& amountOfDataRead) = 0;
    virtual bool writeString(const gtString& str);
    virtual bool readString(gtString& str);

    virtual bool writeString(const gtASCIIString& str);
    virtual bool readString(gtASCIIString& str);

protected:
    // Read and write operations time out (measured in milliseconds):
    long _readOperationTimeOut;
    long _writeOperationTimeOut;
};


#endif  // __OSCHANNEL
