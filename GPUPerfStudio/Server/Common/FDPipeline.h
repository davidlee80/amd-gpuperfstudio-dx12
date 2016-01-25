//==============================================================================
// Copyright (c) 2013-2015 Advanced Micro Devices, Inc. All rights reserved.
/// \author AMD Developer Tools Team
/// \file
/// \brief  a base class for a FrameDebugger which has logic that
///         is API-agnostic
//==============================================================================

#ifndef _FD_PIPELINE_H
#define _FD_PIPELINE_H

#include "CommandProcessor.h"
#include "IDrawCall.h"
#include "ShaderDebuggerHostGPS2.h"
#include "ShaderData.h"
#include "FDPipelineStage.h"
#include <string.h>
#include <vector>
#include <map>

using namespace std;

//-----------------------------------------------------------------------------
/// Provides a base class for the FrameDebugger to interact with an API-
/// specific Pipeline.
//-----------------------------------------------------------------------------
class FDPipeline : public CommandProcessor
{
public:

    /// Default constructor
    FDPipeline();

    /// destructor
    virtual ~FDPipeline();

public:

    /// Callback that occurs when the application has reached the drawcall at the breakpoint but nothing has been drawn on the HUD.
    /// Commands should go here that need to happen before the drawcall has actually been executed.
    /// \param rDrawCall that drawcall that is being called by the application
    virtual void OnDrawCallAtBreakPointPreHUD(IDrawCall& rDrawCall);

    /// Callback that occurs when the application has reached the drawcall at the breakpoint but nothing has been drawn on the HUD.
    /// Commands should go here that can happen after the drawcall has actually been executed.
    /// \param rDrawCall that drawcall that is being called by the application
    virtual void OnDrawCallAtBreakpoint(IDrawCall& rDrawCall);

    /// Cleans up any resources created for the pipeline
    void Cleanup();

    /// Returns a list of image resources with basic data.
    IntCommandResponse m_resourceImageList;

protected:

    /// Adds a pipeline stage to the list of processors and stages with the specified identifiers
    /// \param strTag The Tag name to use in the XML
    /// \param strName The name of this pipeline stage
    /// \param strID An ID assigned to this stage for use in URLs
    /// \param eGraphicalStageInclude Value indicating how this stage is represented in the graphical stage view
    /// \param rStage the FDPipelineStage to add
    void AddStage(const char* strTag, const char* strName, const char* strID, GraphicalStageInclude eGraphicalStageInclude, FDPipelineStage& rStage);

    /// An array of FDPipelineStages
    PipelineStageArray m_stages;

private:

    //-----------------------------------------------------------------------------
    // No additional settings
    //-----------------------------------------------------------------------------
    virtual string GetDerivedSettings();

};

#endif //_FD_PIPELINE_H
