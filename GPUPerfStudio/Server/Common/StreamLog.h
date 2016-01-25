//==============================================================================
// Copyright (c) 2015 Advanced Micro Devices, Inc. All rights reserved.
/// \author AMD Developer Tools Team
/// \file
/// \brief  Simple stream-based logger. Use for directed debugging only.
///         Don't use in any release code.
//==============================================================================

#ifndef STREAM_LOG_H
#define STREAM_LOG_H

#include <fstream>
#include <iostream>
using namespace std;

// Uncomment if you want top use stream logging
//#define USE_STREAMLOG

#ifdef USE_STREAMLOG

///////////////////////////////////////////////////////////////////////////////////////////
/// Simple stream-based logger. Use for directed debugging only.
/// Don't use in any release code.
///////////////////////////////////////////////////////////////////////////////////////////
class StreamLog
{
private:

    /// Internal pointer to the file
    fstream* m_pLogfile;

    static StreamLog* pInstance;

public:

    ///////////////////////////////////////////////////////////////////////////////////////////
    /// Constructor
    ///////////////////////////////////////////////////////////////////////////////////////////
    StreamLog()
    {
        // Currently uses a default file name
        m_pLogfile = new fstream("C:\\StreamLog.txt", fstream::out);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////
    /// Destructor
    ///////////////////////////////////////////////////////////////////////////////////////////
    ~StreamLog()
    {
        /// Close the file
        m_pLogfile->close();
        /// Delete our object (this does not remove the output file)
        delete m_pLogfile;
    }

public:

    ///////////////////////////////////////////////////////////////////////////////////////////
    /// Templatized overloading of the << operator
    ///////////////////////////////////////////////////////////////////////////////////////////
    template <class T>
    StreamLog& operator<<(T msg)
    {
        *m_pLogfile << " " << msg;
        m_pLogfile->flush();
        return *this;
    }


    static StreamLog& Get()
    {
        if (!pInstance)
        {
            pInstance = new StreamLog;
        }

        return *pInstance;
    }

    static StreamLog& Ref()
    {
        if (!pInstance)
        {
            pInstance = new StreamLog;
        }

        return *pInstance;
    }
};

#endif //USE_STREAMLOG

#endif //STREAM_LOG_H