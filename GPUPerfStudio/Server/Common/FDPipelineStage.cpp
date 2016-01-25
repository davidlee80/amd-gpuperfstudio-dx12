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
/// Provides a base class for all API-specific pipeline stages.
//-----------------------------------------------------------------------------
/// Constructor
FDPipelineStage::FDPipelineStage()
    : m_bDoneSDInit(false)
#ifdef _WIN32
    , m_pSDHost(NULL)
#endif
{};

/// Destructor
FDPipelineStage::~FDPipelineStage() {};

// No additional settings
string FDPipelineStage::GetDerivedSettings() { return ""; }

/// Gets a string representing the state of the m_eGraphicalStageInclude member
/// to be included as part of the XML attribute for the pipeline stage
/// \return string representing the state of the m_eGraphicalStageInclude member
string FDPipelineStage::GetDerivedAttributes()
{
    stringstream strOut;

    strOut << "stageinclude='" << GetGraphicalStageIncludeString() << "'";

    return strOut.str();
}

// Cleanup
void FDPipelineStage::Cleanup() {}

// Determine if shader debugging is allowed
bool FDPipelineStage::IsShaderDebuggingSupported()
{
    const PIPELINE_STAGE stage = GetPipelineStage();

    return (stage == PIPELINE_STAGE_CS) || (stage == PIPELINE_STAGE_VS) || (stage == PIPELINE_STAGE_PS);
}

#ifdef _WIN32
/// Creates a shader debugger host
/// \param platform The API on which the shader is being debugged
/// \param pDevice Pointer to an object on which resources can be created for debugger
bool FDPipelineStage::CreateShaderDebugger(AMD_ShaderDebugger::Platform platform, void* pDevice)
{
    if (m_bDoneSDInit == false)
    {
        Log(logMESSAGE, "CreateShaderDebugger %s\n", typeid(*this).name());

        PsAssert(pDevice != NULL);
        m_pSDHost = CShaderDebuggerHostGPS2::CreateShaderDebuggerHostGPS2(platform, pDevice);

        if (m_pSDHost != NULL)
        {
            // need to add the host as a processor so that it can receive commands
            AddProcessor("ShaderDebugger", "Debugger", "SD", "", NO_DISPLAY, *m_pSDHost);

            m_bDoneSDInit = true;
        }
    }

    return m_bDoneSDInit;
}

/// Deletes the shader debugger host if it exists
void FDPipelineStage::DeleteShaderDebugger()
{
    if (m_pSDHost != NULL)
    {
        Log(logMESSAGE, "Deleting Shader Debugger\n");

        // remove the host as a processor so that it cannot receive any more comands
        RemoveProcessor(*m_pSDHost);
        SAFE_DELETE(m_pSDHost);

        m_bDoneSDInit = false;
    }
}
#endif // _WIN32

/// Accessor to retrieve a value indicating if and how this stage will appear in the graphical stage view
/// \return a value indicating if and how this stage will appear in the graphical stage view
GraphicalStageInclude FDPipelineStage::GetGraphicalStageInclude() const
{
    return m_eGraphicalStageInclude;
}

/// Accessor to set a value indicating if and how this stage will appear in the graphical stage view
/// \param eGraphicalStageInclude the value indicating if and how this stage will appear in the graphical stage view
void FDPipelineStage::SetGraphicalStageInclude(GraphicalStageInclude eGraphicalStageInclude)
{
    m_eGraphicalStageInclude = eGraphicalStageInclude;
}

/// Gets a string representation of the stage's GraphicalStageInclude
/// \return a string representation of the stage's GraphicalStageInclude
string FDPipelineStage::GetGraphicalStageIncludeString() const
{
    string strRet = "";

    switch (m_eGraphicalStageInclude)
    {

        case STAGE_CONNECTED:
        {
            strRet = "STAGE_CONNECTED";
            break;
        }

        case STAGE_NOT_CONNECTED:
        {
            strRet = "STAGE_NOT_CONNECTED";
            break;
        }

        case STAGE_NO_INCLUDE:
        {
            strRet = "STAGE_NO_INCLUDE";
            break;
        }

        default:
            Log(logERROR, "Unknown GraphicalStageInclude\n");
            strRet = "STAGE_NO_INCLUDE";
            break;
    }

    return strRet;
}

