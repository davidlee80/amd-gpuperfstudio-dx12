//==============================================================================
// Copyright (c) 2013-2015 Advanced Micro Devices, Inc. All rights reserved.
/// \author AMD Developer Tools Team
/// \file
/// \brief  a base class for a FrameDebugger which has logic that
///         is API-agnostic
//==============================================================================

#ifndef _FD_PIPELINE_STAGE_H
#define _FD_PIPELINE_STAGE_H

#include "CommandProcessor.h"
#include "IDrawCall.h"
#include "ShaderDebuggerHostGPS2.h"
#include "ShaderData.h"

#include <string.h>
#include <vector>
#include <map>

using namespace std;

/// Indicates whether or not the stage is included in the stage view, and if so, whether or not it is "connected" to the previous or subsequent stage
enum GraphicalStageInclude { STAGE_CONNECTED, STAGE_NOT_CONNECTED, STAGE_NO_INCLUDE };

/// Indicates that the value is not an index. This is used on the client so that non-array images (e.g. DepthBuffer) dont use an index in their URLs.
/// Note: this must match the client defined value of 999
enum ArrayIndex { NO_ARRAY_INDEX = 999 };

//-----------------------------------------------------------------------------
/// Provides a base class for all API-specific pipeline stages.
//-----------------------------------------------------------------------------
class FDPipelineStage : public CommandProcessor
{
public:

    /// Constructor
    FDPipelineStage();

    /// Destructor
    virtual ~FDPipelineStage();

    /// Returns XML description of each HUD element.
    /// \return XML string containing the positional information of each HUD Element.
    virtual gtASCIIString GetHUDLayouts() = 0;

    /// Cleans up any resources created for the pipeline stage
    virtual void Cleanup();

public:

    /// Callback when the frame debugger has reached the breakpoint and has not yet setup HUD state
    /// \param rDrawCall the drawcall that is at the breakpoint
    virtual void OnDrawCallAtBreakPointPreHUD(IDrawCall& rDrawCall) = 0;

    /// Callback when the frame debugger has reached the breakpoint
    /// \param rDrawCall the drawcall that is at the breakpoint
    virtual void OnDrawCallAtBreakpoint(IDrawCall& rDrawCall) = 0;

    /// Accessor to retrieve a value indicating if and how this stage will appear in the graphical stage view
    /// \return a value indicating if and how this stage will appear in the graphical stage view
    GraphicalStageInclude GetGraphicalStageInclude() const;

    /// Accessor to set a value indicating if and how this stage will appear in the graphical stage view
    /// \param eGraphicalStageInclude the value indicating if and how this stage will appear in the graphical stage view
    void SetGraphicalStageInclude(GraphicalStageInclude eGraphicalStageInclude);

    /// Gets a string representation of the stage's GraphicalStageInclude state
    /// \return a string representation of the stage's GraphicalStageInclude state
    string GetGraphicalStageIncludeString() const;

    /// All API's need to implement this function. It gets an XML description of each image resource in the
    /// pipeline (Textures, RTs, DB, & SC)
    /// \param strOut The output string
    /// \param mask The resource image type.
    virtual void GetResourceImageList(gtASCIIString& strOut, ResourceImageType mask) = 0;

protected:
    // Determine if shader debugging is allowed
    bool IsShaderDebuggingSupported();

    /// Creates a shader debugger host
    /// \param platform The API on which the shader is being debugged
    /// \param pDevice Pointer to an object on which resources can be created for debugger
    bool CreateShaderDebugger(AMD_ShaderDebugger::Platform platform, void* pDevice);

    /// Deletes the shader debugger host if it exists
    void DeleteShaderDebugger();

    /// The shader debugger host, accessible by non-shader stages
#ifdef _WIN32
    CShaderDebuggerHostGPS2* m_pSDHost;
#endif

    /// The platform (API) for the shader debugger
    AMD_ShaderDebugger::Platform m_sdPlatform;

private:
    // Track the current pipeline stage
    virtual PIPELINE_STAGE GetPipelineStage() { return PIPELINE_STAGE_NONE; }

    /// indicates if and how this stage appears in the graphical pipeline stage view
    GraphicalStageInclude m_eGraphicalStageInclude;

    // No additional settings
    virtual string GetDerivedSettings();

    /// Gets a string representing the state of the m_eGraphicalStageInclude member
    /// to be included as part of the XML attribute for the pipeline stage
    /// \return string representing the state of the m_eGraphicalStageInclude member
    virtual string GetDerivedAttributes();

    /// Indicates that the Shader Debugger has finished initializing
    bool m_bDoneSDInit;
};

/// A vector of FDPipelineStage pointers
typedef vector< FDPipelineStage* > PipelineStageArray;

#endif //_FD_PIPELINE_STAGE_H
