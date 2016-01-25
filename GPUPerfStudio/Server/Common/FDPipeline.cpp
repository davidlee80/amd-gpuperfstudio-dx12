//==============================================================================
// Copyright (c) 2008-2015 Advanced Micro Devices, Inc. All rights reserved.
/// \author AMD Developer Tools Team
/// \file
/// \brief  File contains the implementation of the base classes for the Frame Debugger Pipeline Stages
//==============================================================================

#include <typeinfo>
#include "FDPipeline.h"
#include "misc.h"
#include "xml.h"

using namespace std;

//-----------------------------------------------------------------------------
/// Provides a base class for the FrameDebugger to interact with an API-
/// specific Pipeline.
//-----------------------------------------------------------------------------

/// Default constructor
FDPipeline::FDPipeline()
{
    m_resourceImageList.SetEditableContentAutoReply(false);
    AddCommand(CONTENT_XML, "ResourceImageList",   "ResourceImageList",     "ResourceImageList.xml",  NO_DISPLAY, INCLUDE, m_resourceImageList);
}

/// destructor
FDPipeline::~FDPipeline()
{
}

/// Callback that occurs when the application has reached the drawcall at the breakpoint but nothing has been drawn on the HUD.
/// Commands should go here that need to happen before the drawcall has actually been executed.
/// \param rDrawCall that drawcall that is being called by the application
void FDPipeline::OnDrawCallAtBreakPointPreHUD(IDrawCall& rDrawCall)
{
    PipelineStageArray::const_iterator iter;

    for (iter = m_stages.begin(); iter != m_stages.end(); ++iter)
    {
        FDPipelineStage* pStage = *iter;
        pStage->OnDrawCallAtBreakPointPreHUD(rDrawCall);
    }
}

/// Callback that occurs when the application has reached the drawcall at the breakpoint but nothing has been drawn on the HUD.
/// Commands should go here that can happen after the drawcall has actually been executed.
/// \param rDrawCall that drawcall that is being called by the application
void FDPipeline::OnDrawCallAtBreakpoint(IDrawCall& rDrawCall)
{
    PipelineStageArray::const_iterator iter;

    for (iter = m_stages.begin(); iter != m_stages.end(); ++iter)
    {
        FDPipelineStage* pStage = *iter;
        pStage->OnDrawCallAtBreakpoint(rDrawCall);
    }

    if (m_resourceImageList.IsActive())
    {
        ResourceImageType resourceMask = (ResourceImageType)m_resourceImageList.GetValue();

        gtASCIIString out;

        for (iter = m_stages.begin(); iter != m_stages.end(); ++iter)
        {
            FDPipelineStage* pStage = *iter;
            pStage->GetResourceImageList(out, resourceMask);
        }

        m_resourceImageList.Send(out.asCharArray());
    }
}

/// Cleans up any resources created for the pipeline
void FDPipeline::Cleanup()
{
    PipelineStageArray::const_iterator iter;

    for (iter = m_stages.begin(); iter != m_stages.end(); ++iter)
    {
        FDPipelineStage* pStage = *iter;
        pStage->Cleanup();
    }
}

/// Adds a pipeline stage to the list of processors and stages with the specified identifiers
/// \param strTag The Tag name to use in the XML
/// \param strName The name of this pipeline stage
/// \param strID An ID assigned to this stage for use in URLs
/// \param eGraphicalStageInclude Value indicating how this stage is represented in the graphical stage view
/// \param rStage the FDPipelineStage to add
void FDPipeline::AddStage(const char* strTag, const char* strName, const char* strID, GraphicalStageInclude eGraphicalStageInclude, FDPipelineStage& rStage)
{
    // Duplicate the ID as the Title Prefix
    AddProcessor(strTag, strName, strID, strID, DISPLAY, rStage);
    rStage.SetGraphicalStageInclude(eGraphicalStageInclude);
    m_stages.push_back(&rStage);
}

//-----------------------------------------------------------------------------
// No additional settings
//-----------------------------------------------------------------------------
string FDPipeline::GetDerivedSettings()
{
    return "";
}


