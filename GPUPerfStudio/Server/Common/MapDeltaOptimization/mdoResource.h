//==============================================================================
// Copyright (c) 2015 Advanced Micro Devices, Inc. All rights reserved.
/// \author AMD Developer Tools Team
/// \file
/// \brief  Resource wrapper header file for MDO.
///         This class contains most of the low-level logic used by MDO. Here we
///         page-guard memory, track maps and unmaps, and implement two
///         methods of delta storage and reconstruction.
//==============================================================================

#ifndef __MDO_RESOURCE_H__
#define __MDO_RESOURCE_H__

#include "mdoUtil.h"

class MdoResource
{
public:
    virtual ~MdoResource();

    virtual bool Map(const MdoResMapInfo& mapInfo, void** pMappedPtr) = 0;
    virtual bool Unmap() = 0;

    virtual bool RunMdoCaptureWork() = 0;
    virtual bool RunMdoPlaybackWork() = 0;

    void NewMapEvent(const MdoResMapInfo& mapInfo);
    void NewUnmapEvent();

    void CalcDeltaRegions();
    bool UploadMapDelta();

    UINT32 ResourceByteSize() { return m_size; }

    void DeleteReflectionData();
    void NewReflectionData();
    void UpdateReferenceData();

    void TrackDirtyPage(const DataChunk& deltaData);
    void ResetMapEvents();

    ReflectionData* GetReflectionData() { return &m_reflectionData; }

    bool Guard();
    bool Unguard();

    bool OriginalMapSuccessful();

protected:
    MdoResource();

    void CalcDeltaRegionsPerByteStorage();
    void CalcDeltaRegionsPerMapStorage();

    MdoState              m_state;
    MdoResourceCreateInfo m_createInfo;
    ReflectionData        m_reflectionData;
    MapEvents             m_mapData;
    OrderedAccumDeltas    m_accumDeltas;

    int                   m_captureMapId;
    int                   m_playbackMapId;
    UINT32                m_size;
    int                   m_activeMappedCount;
};

#endif