//=====================================================================
// Copyright 2016 (c), Advanced Micro Devices, Inc. All rights reserved.
//
/// \author AMD Developer Tools Team
/// \file osNULLSocket.h 
/// 
//=====================================================================

//------------------------------ osNULLSocket.h ------------------------------

#ifndef __OSNULLSOCKET
#define __OSNULLSOCKET

// Local:
#include <AMDTOSWrappers/Include/osSocket.h>


// ----------------------------------------------------------------------------------
// Class Name:           osNULLSocket : public osSocket
// General Description:
//   A NULL socket - a socket that does nothing and triggers an assertion when
//   reading or writing to it.
//
// Author:      AMD Developer Tools Team
// Creation Date:        25/8/2005
// ----------------------------------------------------------------------------------
class OS_API osNULLSocket : public osSocket
{
public:
    osNULLSocket(const gtString& socketName): osSocket(socketName) {};

    // Overrides osChannel:
    virtual osChannelType channelType() const;
    virtual bool write(const gtByte*, gtSize_t);
    virtual bool read(gtByte*, gtSize_t);
    virtual bool readAvailableData(gtByte*, gtSize_t, gtSize_t&);

    // Overrides osSocket:
    virtual bool open();
    virtual bool close();
    virtual bool isOpen() const;
};

#endif  // __OSNULLSOCKET
