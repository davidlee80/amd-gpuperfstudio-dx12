//==============================================================================
// Copyright (c) 2015 Advanced Micro Devices, Inc. All rights reserved.
/// \author AMD Developer Tools Team
/// \file
/// \brief  Utilities header file for MDO.
///         Home of the MdoUtil namespace, containing utility functions used
///         throughout MDO. Here we also find reusable macros, includes, etc.
//==============================================================================

#ifndef __MDO_UTIL_H__
#define __MDO_UTIL_H__

#include <windows.h>
#include <process.h>

#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <sstream>
#include <fstream>

#ifdef _DEBUG
    #define MDO_ASSERT(__expr__) if (!(__expr__)) __debugbreak();
    #define MDO_ASSERT_ALWAYS() __debugbreak();
    #define MDO_ASSERT_NOT_IMPLEMENTED() __debugbreak();
#else
    #define MDO_ASSERT(__expr__)
    #define MDO_ASSERT_ALWAYS()
    #define MDO_ASSERT_NOT_IMPLEMENTED()
#endif

#define MDO_SAFE_DELETE(__expr__) if (__expr__) delete __expr__; __expr__ = nullptr;
#define MDO_SAFE_DELETE_ARRAY(__expr__) if (__expr__) delete [] __expr__; __expr__ = nullptr;

enum MdoDeltaStorage
{
    MDO_DISABLED,
    MDO_DELTA_STORAGE_PER_MAP,
    MDO_DELTA_STORAGE_PER_BYTE,
};

enum MdoState
{
    MDO_STATE_CAPTURE_START,
    MDO_STATE_CAPTURE,
    MDO_STATE_PLAYBACK,
};

class MdoResource;

struct MdoConfig
{
    MdoDeltaStorage deltaStorage;
    bool            bypassExceptionFiltering;

    // For debugging
    bool            dbgMirrorAppMaps;
    bool            dbgMdoSpaceUsage;
    bool            dbgMdoTimeUsage;
};

struct MdoResMapInfo
{
    UINT64 resHandle;
    UINT32 subResource;
    UINT32 mapType;
    UINT32 mapFlags;
    void*  pDriverMem;
};

struct SlotHistory
{
    UINT32        mapId;
    unsigned char data;
};

typedef std::vector<SlotHistory> SlotHistories;

struct ReflectionData
{
    unsigned char* pReferenceData;
    unsigned char* pNewData;
    SlotHistories* pHistories;
};

struct MdoResourceCreateInfo
{
    UINT64    resHandle;
    void*     pDevice;
    MdoConfig mdoConfig;
};

struct DataChunk
{
    UINT32 offset;
    UINT32 size;
    void*  pData;
};

typedef std::vector<DataChunk> DataChunks;

struct MapEvent
{
    DataChunks     dirtyPages;
    DataChunks     deltas;
    MdoResMapInfo  mapInfo;

    // For debugging
    unsigned char* pAppMapMirror;
};

typedef std::unordered_map<int, MapEvent> MapEvents;
typedef std::map<int, DataChunks> OrderedAccumDeltas;
typedef std::unordered_map<UINT64, MdoResource*> ResourceMap;

/**
**************************************************************************************************
* @brief Util and debug functions for MDO.
**************************************************************************************************
*/
namespace MdoUtil
{
inline UINT64 Pow2Align(UINT64 val, UINT64 alignment)
{
    return ((val + alignment - 1) & ~(alignment - 1));
}

// For logging/debugging
std::string FormatWithCommas(UINT32 value);

// For logging/debugging
void Print(const char* pStr);

// For logging/debugging
void PrintLn(const char* pStr);

// For logging/debugging
bool DumpMemBufferToDiskArray(
    const std::string& name,
    const std::string& path,
    UINT32             bytesPerRow,
    const char*        pBuf,
    UINT32             byteCount);

// For logging/debugging
void DumpFileToDiskArray(
    const std::string& name,
    const std::string& path,
    const std::string& ext,
    UINT32             bytesPerRow);
}

#define USE_RDTSC 0

/**
**************************************************************************************************
* @brief A CPU timer class.
**************************************************************************************************
*/
class MdoTimer
{
public:
    MdoTimer() : m_freq(0.0), m_startTime(0)
    {
#ifdef _WIN32

#if USE_RDTSC
        static const GR_DOUBLE CalibrationTime = 1.0;
        UINT64 t0 = rdtsc_time();
        Delay(CalibrationTime);
        UINT64 t1 = rdtsc_time();
        m_freq = static_cast<GR_DOUBLE>(t1 - t0) / CalibrationTime;
#else
        LARGE_INTEGER freq = { 0 };
        QueryPerformanceFrequency(&freq);
        m_freq = static_cast<double>(freq.QuadPart);
#endif

#endif
    }

    void Start()
    {
#ifdef _WIN32

#if USE_RDTSC
        m_startTime = rdtsc_time();
#else
        LARGE_INTEGER t = { 0 };
        QueryPerformanceCounter(&t);
        m_startTime = t.QuadPart;
#endif

#endif
    }

    double Stop()
    {
        UINT64 endTime = m_startTime;

#ifdef _WIN32

#if USE_RDTSC
        endTime = rdtsc_time();
#else
        LARGE_INTEGER t = { 0 };
        QueryPerformanceCounter(&t);
        endTime = t.QuadPart;
#endif

#endif

        return static_cast<double>(endTime - m_startTime) / m_freq;
    }

private:
    void Delay(double delay)
    {
#ifdef _WIN32

        LARGE_INTEGER st = { 0 };
        LARGE_INTEGER et = { 0 };
        QueryPerformanceCounter(&st);

        bool calibrating = false;

        while (calibrating)
        {
            QueryPerformanceCounter(&et);

            double t = static_cast<double>(et.QuadPart - st.QuadPart) / m_freq;

            if (t >= delay)
            {
                calibrating = false;
            }
        }

#endif
    }

    double m_freq;
    UINT64 m_startTime;
};

/**
**************************************************************************************************
* @brief A wrapper class to simplify using the OS's CRITICAL_SECTION
**************************************************************************************************
*/
class MdoMutex
{
public:
    /// Constructor
    ///
    /// Initializes the critical section so that it can be used
    MdoMutex()
    {
        InitializeCriticalSection(&m_cs);
    }

    /// Destructor
    ///
    /// Deletes the critical section
    ~MdoMutex()
    {
        DeleteCriticalSection(&m_cs);
    }

    /// Locks the critical section
    void Lock()
    {
        EnterCriticalSection(&m_cs);
    }

    /// Unlocks the critical section
    void Unlock()
    {
        LeaveCriticalSection(&m_cs);
    }

private:
    /// The underlying critical section object
    CRITICAL_SECTION m_cs;
};

/// Helper for Mutex class which Enters a critical section in the constructor and Leaves that section in the destructor
class ScopedLock
{
public:
    /// enters a critical section
    /// \param m pointer to the mutex to use
    ScopedLock(MdoMutex* m)
    {
        m_pMtx = m;
        m_pMtx->Lock();
    }

    /// enters a critical section
    /// \param m reference to the mutex to use
    ScopedLock(MdoMutex& m)
    {
        m_pMtx = &m;
        m_pMtx->Lock();
    }

    /// Leaves the critical section
    ~ScopedLock()
    {
        m_pMtx->Unlock();
    }

private:
    /// Underlying mutex class
    MdoMutex* m_pMtx;
};


#endif