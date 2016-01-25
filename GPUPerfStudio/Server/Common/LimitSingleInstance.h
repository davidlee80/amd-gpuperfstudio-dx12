//==============================================================================
// Copyright (c) 2013-2015 Advanced Micro Devices, Inc. All rights reserved.
/// \author AMD Developer Tools Team
/// \file
/// \brief  Class that helps limit apps instaces to one
//==============================================================================

#ifndef LimitSingleInstance_H
#define LimitSingleInstance_H

#ifdef WIN32
#include <windows.h>

/// This code is from Q243953 in case you lose the article and wonder
/// where this code came from.
class CLimitSingleInstance
{
protected:
    /// Caches the last error that was received
    DWORD  m_dwLastError;

    /// Handle to a mutex which ensure that this is the only instance
    HANDLE m_hMutex;

public:

    /// Constructor which sets the name of the mutex
    /// \param strMutexName name of the mutex to use
    CLimitSingleInstance(TCHAR* strMutexName)
    {
        //Make sure that you use a name that is unique for this application otherwise
        //two apps may think they are the same if they are using same name for
        //3rd parm to CreateMutex
        m_hMutex = CreateMutex(NULL, FALSE, strMutexName);   //do early
        m_dwLastError = GetLastError(); //save for use later...
    }

    /// Destructor
    ~CLimitSingleInstance()
    {
        if (m_hMutex)    //Do not forget to close handles.
        {
            CloseHandle(m_hMutex);   //Do as late as possible.
            m_hMutex = NULL; //Good habit to be in.
        }
    }

    /// Checks to see if another instance is running
    /// \return true if another instance is running; false otherwise
    BOOL IsAnotherInstanceRunning()
    {
        return (ERROR_ALREADY_EXISTS == m_dwLastError);
    }
};

#else

#include "NamedMutex.h"

/// This code is from Q243953 in case you lose the article and wonder
/// where this code came from.
class CLimitSingleInstance
{
protected:
    /// Caches the last error that was received
    bool   m_anotherInstanceRunning;

    /// Handle to a mutex which ensure that this is the only instance
    NamedMutex m_mutex;

public:

    /// Constructor which sets the name of the mutex
    /// \param strMutexName name of the mutex to use
    CLimitSingleInstance(char* strMutexName)
    {
        m_anotherInstanceRunning = true;

        //Make sure that you use a name that is unique for this application otherwise
        //two apps may think they are the same if they are using same name for
        //1st param to Open(). Also make sure 32 and 64 bit version share the same name
        if (m_mutex.Open(strMutexName, false, true) == false)
        {
            // doesn't exist, create one
            m_mutex.OpenOrCreate(strMutexName, false, true);
            m_anotherInstanceRunning = false;
        }
    }

    /// Destructor
    ~CLimitSingleInstance()
    {
        m_mutex.Close();
    }

    /// Checks to see if another instance is running
    /// \return true if another instance is running; false otherwise
    bool IsAnotherInstanceRunning()
    {
        return m_anotherInstanceRunning;
    }
};

#endif // WIN32

#endif // LimitSingleInstance_H
