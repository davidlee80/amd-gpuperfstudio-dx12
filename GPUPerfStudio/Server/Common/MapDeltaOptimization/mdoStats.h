//==============================================================================
// Copyright (c) 2015 Advanced Micro Devices, Inc. All rights reserved.
/// \author AMD Developer Tools Team
/// \file
/// \brief  Stats header file for MDO.
///         A simple class used to measure MDO performance and mem consumption.
//==============================================================================

#ifndef __MDO_STATS_H__
#define __MDO_STATS_H__

#include "mdoUtil.h"

class MdoStats
{
public:
    static MdoStats* Create(const MdoConfig& mdoConfig);

    static void TrackHeapAlloc(UINT32 numBytes, void* pMem);
    static void TrackHeapFree(void* pMem);
    static void StartCpuTimer();
    static void StopCpuTimer(const std::string& desc);

    void TrackAlloc(UINT32 numBytes, void* pMem);
    void TrackFree(void* pMem);
    void StartTimer();
    double StopTimer(const std::string& desc);

    ~MdoStats() {}

private:
    MdoStats(const MdoConfig& mdoConfig);

    MdoConfig                         m_mdoConfig;
    std::unordered_map<void*, UINT32> m_allocSizes;
    UINT32                            m_totalConsumption;
    MdoTimer                          m_timer;
};

#endif