//=====================================================================
// Copyright 2016 (c), Advanced Micro Devices, Inc. All rights reserved.
//
/// \author AMD Developer Tools Team
/// \file osTCPSocket.cpp 
/// 
//=====================================================================

//------------------------------ osTCPSocket.cpp ------------------------------

// Win32:
#include <Winsock2.h>

// For TCP Keep Alive support.
#include <WS2tcpip.h>
#include <MSTcpIP.h>

// Infra
#include <AMDTBaseTools/Include/gtAssert.h>

// Local:
#include <AMDTOSWrappers/Include/osPortAddress.h>
#include <AMDTOSWrappers/Include/osSystemError.h>
#include <AMDTOSWrappers/Include/osDebugLog.h>
#include <AMDTOSWrappers/Include/osTime.h>
#include <AMDTOSWrappers/Include/osTCPSocket.h>
#include <AMDTOSWrappers/Include/osStringConstants.h>
#include <sstream>

// Static members initializations:
osSocketDescriptor osTCPSocket::NO_OS_SOCKET_DESCRIPTOR = INVALID_SOCKET;


// ---------------------------------------------------------------------------
// Name:        osTCPSocket::osTCPSocket
// Description: Constructor
// Author:      AMD Developer Tools Team
// Date:        3/1/2004
// ---------------------------------------------------------------------------
osTCPSocket::osTCPSocket(const gtString& socketName)
    : osSocket(socketName), _blockOnDNS(false), _socketDescriptor(NO_OS_SOCKET_DESCRIPTOR),
      _isOpen(false)
{
}


// ---------------------------------------------------------------------------
// Name:        osTCPSocket::osTCPSocket
// Description: Constructor
// Arguments:   osSocketDescriptor - The OS socket descriptor.
// Author:      AMD Developer Tools Team
// Date:        3/1/2004
// ---------------------------------------------------------------------------
osTCPSocket::osTCPSocket(unsigned int osSocketDescriptor, const gtString& socketName)
    : osSocket(socketName), _blockOnDNS(false), _socketDescriptor(NO_OS_SOCKET_DESCRIPTOR), _isOpen(false)
{
    setOSDescriptor(osSocketDescriptor);
}


// ---------------------------------------------------------------------------
// Name:        osTCPSocket::getPeerHostAddress
// Description:
//   Retrieves the peer host address. (The address of the host to which
//   this socket is connected).
//   Notice that this method can only be called after the socket
//   connection was established (connect / accept succeeded).
// Arguments: peerHostAddress - Will get the peer host address.
// Return Val: bool  - Success / failure.
// Author:      AMD Developer Tools Team
// Date:        27/4/2008
// ---------------------------------------------------------------------------
bool osTCPSocket::getPeerHostAddress(osPortAddress& peerHostAddress) const
{
    bool retVal = false;

    // Get the peer host address:
    sockaddr_in peerSocketAddress;
    int sockaddrLen = sizeof(peerSocketAddress);
    int rc1 = ::getpeername(_socketDescriptor, (sockaddr*)&peerSocketAddress, &sockaddrLen);
    GT_IF_WITH_ASSERT(rc1 == 0)
    {
        // Output the peer host address:
        bool rc2 = peerHostAddress.setFromSocaddr(peerSocketAddress);
        GT_IF_WITH_ASSERT(rc2)
        {
            retVal = true;
        }
    }

    return retVal;
}


// ---------------------------------------------------------------------------
// Name:        osTCPSocket::getCurrenttAddress
// Description:
//   Retrieves the local address.
// Arguments: peerHostAddress - Will get the current address.
// Return Val: bool  - Success / failure.
// Author:      AMD Developer Tools Team
// Date:        07/08/2013
// ---------------------------------------------------------------------------
bool osTCPSocket::getCurrentAddress(osPortAddress& peerHostAddress) const
{
    bool retVal = false;

    // Get the peer host address:
    sockaddr_in peerSocketAddress;
    int sockaddrLen = sizeof(peerSocketAddress);
    int rc1 = ::getsockname(_socketDescriptor, (sockaddr*)&peerSocketAddress, &sockaddrLen);
    GT_IF_WITH_ASSERT(rc1 == 0)
    {
        // Output the peer host address:
        bool rc2 = peerHostAddress.setFromSocaddr(peerSocketAddress);
        GT_IF_WITH_ASSERT(rc2)
        {
            retVal = true;
        }
    }

    return retVal;
}

// ---------------------------------------------------------------------------
// Name:        osTCPSocket::setOSDescriptor
// Description: Sets my OS descriptor.
// Author:      AMD Developer Tools Team
// Date:        18/1/2004
// ---------------------------------------------------------------------------
void osTCPSocket::setOSDescriptor(osSocketDescriptor socketDescriptor)
{
    // If I already have an associated OS socket - release it:
    if (_isOpen)
    {
        close();
    }

    // Set my OS socket descriptor:
    _socketDescriptor = socketDescriptor;

    // If we got a NULL socket descriptor:
    if (socketDescriptor == NO_OS_SOCKET_DESCRIPTOR)
    {
        _isOpen = false;
    }
    else
    {
        _isOpen = true;
    }
}


// ---------------------------------------------------------------------------
// Name:        osTCPSocket::~osTCPSocket
// Description: Destructor
// Author:      AMD Developer Tools Team
// Date:        3/1/2004
// ---------------------------------------------------------------------------
osTCPSocket::~osTCPSocket()
{
    // If the OS socket is open - close it:
    if (_isOpen)
    {
        close();
    }
}


// ---------------------------------------------------------------------------
// Name:        osTCPSocket::channelType
// Description: Returns my channel type - TCP sockets are binary channels.
// Author:      AMD Developer Tools Team
// Date:        4/8/2004
// ---------------------------------------------------------------------------
osChannel::osChannelType osTCPSocket::channelType() const
{
    return osChannel::OS_BINARY_CHANNEL;
}


// ---------------------------------------------------------------------------
// Name:        osTCPSocket::open
// Description: Opens the socket end point.
// Return Val:  bool - Success / failure.
// Author:      AMD Developer Tools Team
// Date:        3/1/2004
// ---------------------------------------------------------------------------
bool osTCPSocket::open()
{
    bool retVal = false;

    // Verify that the socket was not already opened:
    if (!_isOpen)
    {
        // Use "Internetwork" address family (TCP, UDP, etc):
        int socketAddressFamily = AF_INET;

        // This is a TCP socket:
        int socketType = SOCK_STREAM;

        // Use TCP protocol:
        int usedProtocol = IPPROTO_TCP;

        // Create the win32 socket:
        _socketDescriptor = socket(socketAddressFamily, socketType, usedProtocol);

        if (_socketDescriptor != INVALID_SOCKET)
        {
            _isOpen = true;
            retVal = true;

            // Performance optimization: Avoiding Nagle algorithm to solve a severe
            // performance issue encountered in remote debugging sessions.
            BOOL flag = 1;
            int result = setsockopt(_socketDescriptor,            /* socket affected */
                                    IPPROTO_TCP,                                      /* set option at TCP level */
                                    TCP_NODELAY,                                      /* name of option */
                                    (char*) &flag,                                    /* the cast is historical cruft */
                                    sizeof(BOOL));                                    /* length of option value */

            GT_ASSERT(result >= 0);
        }
        else
        {
            // Get the system's last recorded error:
            gtString systemErrorAsString;
            osGetLastSystemErrorAsString(systemErrorAsString);

            // Output the error to the log file:
            gtString errorMsg = L"Socket open error: ";
            errorMsg += systemErrorAsString;
            OS_OUTPUT_DEBUG_LOG(errorMsg.asCharArray(), OS_DEBUG_LOG_ERROR);
        }
    }

    return retVal;
}


// ---------------------------------------------------------------------------
// Name:        osTCPSocket::close
// Description: Close this socket
// Return Val:  bool - Success / failure.
// Author:      AMD Developer Tools Team
// Date:        3/1/2004
// ---------------------------------------------------------------------------
bool osTCPSocket::close()
{
    bool retVal = false;

    // Sanity check:
    if (_socketDescriptor != NO_OS_SOCKET_DESCRIPTOR)
    {
        // Close the win32 socket:
        if (closesocket(_socketDescriptor) == 0)
        {
            _isOpen = false;
            _socketDescriptor = NO_OS_SOCKET_DESCRIPTOR;
            retVal = true;
        }
    }

    return retVal;
}


// ---------------------------------------------------------------------------
// Name:        osTCPSocket::isOpen
// Description: Returns true iff the socket is open.
// Author:      AMD Developer Tools Team
// Date:        31/1/2004
// ---------------------------------------------------------------------------
bool osTCPSocket::isOpen() const
{
    return _isOpen;
}


// ---------------------------------------------------------------------------
// Name:        osTCPSocket::write
// Description: Write data into the socket.
// Arguments:   pDataBuffer - A buffer that contains the data to be written.
//              dataSize - The size of the data.
// Return Val:  bool - Success / failure.
// Author:      AMD Developer Tools Team
// Date:        24/1/2004
// ---------------------------------------------------------------------------
bool osTCPSocket::write(const gtByte* pDataBuffer, gtSize_t dataSize)
{
    bool retVal = false;

    // A set of sockets that we will enable write on:
    fd_set fdWriteEnabledSocketDescriptorsSet  = { 0 };
    FD_ZERO(&fdWriteEnabledSocketDescriptorsSet);

    // Add this socket to the above set:
    FD_SET(_socketDescriptor, &fdWriteEnabledSocketDescriptorsSet);

    // Convert the timeout to a timeval structure:
    TIMEVAL timeOutAsTimeVal;
    osTimeValFromMilliseconds(_writeOperationTimeOut, timeOutAsTimeVal);

    // Enable writing to our socket:
    int rc = select(NULL, NULL, &fdWriteEnabledSocketDescriptorsSet, NULL, &timeOutAsTimeVal);

    if (rc > 0)
    {
#if GR_ADDRESS_SPACE_TYPE == GR_64_BIT_ADDRESS_SPACE
        // In 64-bit, size_t is larger than int, so the dataSize could theoreticly be larger
        // than what the winsock send function accepts.
        GT_ASSERT(dataSize < INT_MAX);
#endif
        // Write the data into the socket:
        rc = send(_socketDescriptor, (LPCSTR)pDataBuffer, (int)dataSize, 0);

        if (rc != SOCKET_ERROR)
        {
            // Verify that all the data was written:
            if (((unsigned int)(rc)) == dataSize)
            {
                retVal = true;
            }
        }
    }

    // Clean up:
    FD_ZERO(&fdWriteEnabledSocketDescriptorsSet);

    GT_ASSERT(retVal);
    return retVal;
}


// ---------------------------------------------------------------------------
// Name:        osTCPSocket::read
// Description: Reads data from the socket.
// Arguments:   pDataBuffer - Buffer to read the data into.
//              dataSize - The size of the data to be read.
//              amountOfDataRead - The amount of data read by this function call.
// Return Val:  bool - Success / failure.
// Author:      AMD Developer Tools Team
// Date:        24/1/2004
// ---------------------------------------------------------------------------
bool osTCPSocket::read(gtByte* pDataBuffer, gtSize_t dataSize)
{
    bool retVal = false;

    // Sanity check:
    GT_IF_WITH_ASSERT(0 < dataSize)
    {
        gtSize_t readDataSize = 0;
        retVal = readDataFromSocket(pDataBuffer, dataSize, false, readDataSize);
    }

    return retVal;
}


// ---------------------------------------------------------------------------
// Name:        osTCPSocket::readAvailableData
// Description:
//   Reads and outputs the data that is currently available in the socket.
//   If no data is available in the pipe for reading, waits _readOperationTimeOut
//   milliseconds until data is available.
//   If more data than bufferSize is available, reads only bufferSize bytes of data.
//
// Arguments: pDataBuffer - A buffer that will receive the data.
//            bufferSize - The buffer size.
//            amountOfDataRead - The amount of data actually read.
//
// Return Val: bool  - Success / failure.
// Author:      AMD Developer Tools Team
// Date:        10/2/2008
// ---------------------------------------------------------------------------
bool osTCPSocket::readAvailableData(gtByte* pDataBuffer, gtSize_t bufferSize, gtSize_t& amountOfDataRead)
{
    bool retVal = readDataFromSocket(pDataBuffer, bufferSize, true, amountOfDataRead);
    return retVal;
}


// ---------------------------------------------------------------------------
// Name:        osTCPSocket::readDataFromSocket
// Description:
//   Reads data from the pipe.
//   readOnlyAvailableData - true - Causes this function to implement readAvailableData().
//                         - false - Causes this function to implement read().
//
// Author:      AMD Developer Tools Team
// Date:        10/2/2008
// ---------------------------------------------------------------------------
bool osTCPSocket::readDataFromSocket(gtByte* pDataBuffer, gtSize_t bufferSize, bool readOnlyAvailableData, gtSize_t& readDataSize)
{
    bool retVal = false;
    readDataSize = 0;

    // A set of sockets that we will enable read on:
    fd_set fdReadEnabledSocketDescriptorsSet  = { 0 };
    FD_ZERO(&fdReadEnabledSocketDescriptorsSet);

    // Add this socket to the above set:
    FD_SET(_socketDescriptor, &fdReadEnabledSocketDescriptorsSet);

    // Convert the timeout to a timeval structure:
    TIMEVAL timeOutAsTimeVal;
    osTimeValFromMilliseconds(_readOperationTimeOut, timeOutAsTimeVal);

    // Enable reading from our socket:
    int rc = select(NULL, &fdReadEnabledSocketDescriptorsSet, NULL, NULL, &timeOutAsTimeVal);

    // If there is a socket that we can read from:
    if (rc > 0)
    {
        bool goOn = true;

        while (goOn)
        {
#if GR_ADDRESS_SPACE_TYPE == GR_64_BIT_ADDRESS_SPACE
            // In 64-bit, size_t is larger than int, so the dataSize could theoreticly be larger
            // than what the winsock send function accepts.
            GT_ASSERT(bufferSize < INT_MAX);
#endif

            // Read data from the socket:
            rc = recv(_socketDescriptor, (LPSTR)pDataBuffer, (int)bufferSize, 0);

            // If an error occurred:
            if (rc == SOCKET_ERROR)
            {
                GT_ASSERT(false);
                goOn = false;
            }
            else if (rc == 0)
            {
                // If the socket was closed gracefully:
                // (We get 0 when the socket was closed gracefully):
                _isOpen = false;
                goOn = false;
                retVal = true;
            }
            else
            {
                // Log the amount of data read so far:
                readDataSize += rc;

                // If the entire data was read:
                if (bufferSize == ((unsigned long)rc))
                {
                    retVal = true;
                    goOn = false;
                }
                else if (((unsigned long)rc) < bufferSize)
                {
                    // We still have data to read:

                    // If we were asked to read only the available data:
                    if (readOnlyAvailableData)
                    {
                        retVal = true;
                        goOn = false;
                    }
                    else
                    {
                        bufferSize -= rc;
                        (char*)pDataBuffer += rc;

                        // Let other threads with the same priority as mine run
                        // (maybe one of them is the thread that fills my input data)
                        ::Sleep(0);
                    }
                }
                else
                {
                    // Error - we read more data that we asked for:
                    GT_ASSERT(0);
                    goOn = false;
                }
            }
        }
    }

    // Clean up:
    FD_ZERO(&fdReadEnabledSocketDescriptorsSet);

    GT_RETURN_WITH_ASSERT(retVal);
}

// ---------------------------------------------------------------------------
// Name:        osTCPSocket::getIpAddresses
// Description:
//   Gets a list of ip addresses of the current machine.
// Author:      AMD Developer Tools Team
// Date:        15/9/2013
// ---------------------------------------------------------------------------
bool osTCPSocket::getIpAddresses(gtVector<gtString>& o_addresses)
{
    const unsigned BUFFER_SIZE = 1024;
    char ac[BUFFER_SIZE];
    bool ret = (::gethostname(ac, sizeof(ac)) != SOCKET_ERROR);
    GT_ASSERT_EX(ret, OS_TCP_ERR_FAILED_TO_GET_HOSTNAME);

    if (ret)
    {
        struct hostent* phe = ::gethostbyname(ac);
        ret = (phe != 0);
        GT_ASSERT_EX(ret, OS_TCP_ERR_FAILED_TO_GET_HOSTENT_FROM_HOSTNAME);

        if (ret)
        {
            for (int i = 0; phe->h_addr_list[i] != 0; ++i)
            {
                struct in_addr addr;
                ::memcpy(&addr, phe->h_addr_list[i], sizeof(struct in_addr));
                std::wstringstream stream;
                stream << ::inet_ntoa(addr);
                o_addresses.push_back(stream.str().c_str());
            }
        }
    }

    return ret;
}

// ---------------------------------------------------------------------------
// Name:        osTCPSocket::setKeepAlive
// Description: Turns on the TCP KeepAlive setting for this specific socket.
// Arguments:   timeoutMs  - the timeout, in milliseconds, with no activity until
//                           the first keep-alive packet is sent.
//              intervalMs - the interval, in milliseconds, between when successive
//                           keep-alive packets are sent if no acknowledgment is
//                           received.
// Author:      AMD Developer Tools Team
// Date:        6/2/2014
// ---------------------------------------------------------------------------
bool osTCPSocket::setKeepAlive(unsigned long timeoutMs, unsigned long intervalMs)
{
    // Prepare the parameters.
    DWORD dwBytesRet = 0;
    struct tcp_keepalive   alive;
    alive.onoff = TRUE;
    alive.keepalivetime = timeoutMs;
    alive.keepaliveinterval = intervalMs;

    // Configure the socket.
    bool isKeepAliveSucceeded = (WSAIoctl(_socketDescriptor, SIO_KEEPALIVE_VALS, &alive, sizeof(alive),
                    NULL, 0, &dwBytesRet, NULL, NULL) != SOCKET_ERROR);

    if (!isKeepAliveSucceeded)
    {
        // Log the error.
        gtString errMsg;
        errMsg.appendFormattedString(L"WSAIotcl(SIO_KEEPALIVE_VALS) failed; %d\n", WSAGetLastError());
        OS_OUTPUT_DEBUG_LOG(errMsg.asCharArray(), OS_DEBUG_LOG_ERROR);
    }

    return isKeepAliveSucceeded;
}

// ---------------------------------------------------------------------------
// Name:        osTCPSocket::isConnectionAlive
// Description: Blocks until the connection is broken
// Notes:       1. This function BLOCKS until it has a result.
//              2. This function resets the TCP Keep Alive settings of this
//                 osTCPSocket object. Therefore, if you use TCP Keep Alive
//                 you should set it again after this function returns.
// Arguments:   bool& isAlive  - a buffer to hold the result.
// Return value:true for success, false otherwise.
// Author:      AMD Developer Tools Team
// Date:        23/7/2014
// ---------------------------------------------------------------------------
bool osTCPSocket::isConnectionAlive(bool& isAlive)
{
    bool ret = false;
    isAlive = true;

    // Set a threshold after which the connection is declared as broken.
    bool rc = setKeepAlive(1000, 1000);

    GT_IF_WITH_ASSERT(rc)
    {
        // A dummy buffer, all we want is to be notified when the connection is broken.
        char* pBuffer = NULL;
        int bufferSize = 0;

        // Block until the connection is broken.
        int nBytesReceived = ::recv(_socketDescriptor, pBuffer, bufferSize, 0);

        // Check if the connection is still alive.
        bool isError = (nBytesReceived == SOCKET_ERROR);
        bool isConnectionError = (WSAGetLastError() == WSAECONNRESET);
        isAlive = (!isError || !isConnectionError);
        ret = true;
    }

    return ret;
}
