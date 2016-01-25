//==============================================================================
// Copyright (c) 2013-2015 Advanced Micro Devices, Inc. All rights reserved.
/// \author AMD Developer Tools Team
/// \file
/// \brief  a base class for a FrameDebugger which has logic that
///         is API-agnostic
//==============================================================================

#ifndef _FD_PIPELINE_SHADER_STAGE_H
#define _FD_PIPELINE_SHADER_STAGE_H

#include "CommandProcessor.h"
#include "IDrawCall.h"
#include "ShaderDebuggerHostGPS2.h"
#include "ShaderData.h"
#include "FDPipelineStage.h"
#include <string.h>
#include <vector>
#include <map>

using namespace std;

/// Command to start the Shader Debugger in a particular mode.
/// Takes Mode=value as a parameter in the URL
class StartShaderDebuggerCommand : public CommandResponse
{
public:

    /// default constructor
    StartShaderDebuggerCommand() {}

    /// default destructor
    virtual ~StartShaderDebuggerCommand() {}

    /// Gets the mode that was supplied when the command became active
    /// \return string containing the mode that the shader debugger should be started in
    const char* GetMode() { return m_mode.c_str(); }

private:
    virtual bool GetParams(CommandObject& rCommObj)
    {
        return GetParam< std::string >(rCommObj, "Mode", m_mode);
    }

    /// Stores the mode supplied in the URL
    std::string m_mode;
};

//-----------------------------------------------------------------------------
/// Provides a base class for all API-specific shader pipeline stages.
//-----------------------------------------------------------------------------
class FDPipelineShaderStage : public FDPipelineStage
{
public:

    /// Constructor
    FDPipelineShaderStage();

    /// Destructor
    virtual ~FDPipelineShaderStage();

public:

    /// Allows API-specific and shader-specific adding of the ShaderDebugger
    /// \param platform That platform (API) which the app is using
    void AddShaderDebuggerCommand(AMD_ShaderDebugger::Platform platform);

    /// Indicates if this shader stage is in the process of debugging a shader
    /// \return true if the shader is being debugged; false otherwise
    bool IsShaderDebugging() const;

    void OnDrawCallAtBreakPointPreHUD(IDrawCall& rDrawCall);

    /// \copydoc FDPipelineStage::OnDrawCallAtBreakpoint
    /// Sends back code, constants, samplers, and activates the
    /// shader debugger if needed
    void OnDrawCallAtBreakpoint(IDrawCall& rDrawCall);

    /// Deletes the shader debugger for this shader stage
    virtual void Cleanup();

private:
    /// Callback to send the shader  to the client
    /// \param cmd The command response to send the data to.
    virtual void CaptureAndSendShader(CommandResponse& cmd, IDrawCall& rDrawCall) = 0;

#ifdef GPS_GSA
    /// Callback to send the shader analysis to the client
    virtual void CaptureAndSendShaderAnalysis() = 0;

    /// Callback to send the shader HW disassembly to the client
    virtual void CaptureAndSendShaderHWDisassembly() = 0;
#endif // GPS_GSA

    /// Callback to send the shader constants to the client
    virtual void CaptureAndSendConstants() = 0;

    /// Callback to send the samplers to the client
    virtual void CaptureAndSendSamplers() = 0;

    /// Callback to respond to additional API-specific commands
    virtual void AtBreakpointCaptureAndSend(IDrawCall& rDrawCall) = 0;

    /// Callback to get the active device / context
    /// \return Pointer to an object that shader debugger resources can be created on
    virtual void* GetActiveDevice() = 0;

    /// Gets shader data for this stage
    /// \param pShaderData reference to a pointer to a shader data object
    /// \return true if the shader is available and if the parameters could be populated; false on error
    virtual bool GetShaderData(ShaderData*& pShaderData) = 0;

    /// Converts the string into the associated Shader Debugger Mode
    /// \param pcszMode string name of the mode that needs to be converted
    /// \return the enumerated value corresponding to the mode string
    virtual AMD_ShaderDebugger::Mode ConvertShaderDebuggerMode(const char* pcszMode) = 0;

protected:
    /// Requests the shader code, info, keywords, etc. to be sent to the shader debugger window
    CommandResponse m_Shader;

    /// Requests the shader code, info, keywords, etc. to be sent to the CodeViewer window
    CommandResponse m_CodeViewer;

    /// Generates XML that can be sent back to the client in response
    /// to a request for the shader
    /// \param pszHash a hash of the supplied shader
    /// \param pszShaderModeXML the output of one or more calls to ShaderModeXML(...)
    /// \return string formatted in XML of the available shader Modes
    gtASCIIString ShaderXML(const char* pszHash,
                            const char* pszShaderModeXML);

    /// Generates XML for an available shader mode. This output should be passed into ShaderXML before
    /// being sent back to the client.
    /// \param pszName the name of this mode to show the user
    /// \param bDefault indicates if this mode should be the default one displayed (there should only be one default)
    /// \param bDebuggable indicates if this mode is debuggable
    /// \param pszDebugError if bDebuggable is false, this error will be sent to explain why it cannot be debugged
    /// \param pszCode the actual shader code
    /// \param pszEntryFunction The name of the shader's entrypoint, may be empty "" or null if unknown
    /// \param uLineNumber The 0-based line number, if no entrypoint is supplied, this value will be ignored.
    /// \param pszKeywords XML describing the keywords (May want to make another function for this)
    /// \param bEditable True if the shader code is editable (currently only for HLSL with correct debug flags).
    /// \param versionMajor Shader target major version
    /// \param versionMinor Shader target minor version
    /// \return string formatted in XML that describes a single shader mode; this should be passed into ShaderXML(...)
    gtASCIIString ShaderModeXML(const char* pszName,
                                bool bDefault,
                                bool bDebuggable,
                                const char* pszDebugError,
                                const char* pszCode,
                                const char* pszEntryFunction,
                                unsigned int uLineNumber,
                                const char* pszKeywords,
                                const char* pszCompilerFlags,
                                bool bEditable,
                                unsigned int versionMajor,
                                unsigned int versionMinor,
                                const char* debugName);

    /// Converts the keywordType into a string
    /// \param keyWordType the keywordType to convert
    /// \return string version of the keyWordType
    const char* GetKeyWordID(ShaderUtils::KeyWordType keyWordType);

    /// Gets attributes that should be associated with a type of keyword
    /// \param keyWordType the type to get XML attributes for
    /// \return string containing the attributes
    const char* GetKeyWordAttrib(ShaderUtils::KeyWordType keyWordType);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Outputs the rShaderLinkage list of class instance names currently being used by the shader in shader data.
    /// \param pShaderData The shaderData being passed to the SD.
    /// \param rShaderLinkage Output list of class instance names.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual void GetClassInstanceNames(void* /*pShaderData*/, AMD_ShaderDebugger::ShaderLinkage& rShaderLinkage)
    {
        rShaderLinkage.clear();
    }

#ifdef GPS_GSA
    /// Requests the shader analysis to be sent to the client
    CommandResponse m_ShaderAnalysis;

    /// Requests the shader HW disassembly to be sent to the client
    CommandResponse m_ShaderHWDisassembly;
#endif // GPS_GSA

    /// Requests the shader constants to be sent to the client
    CommandResponse m_Constants;

    /// Requests the samplers to be sent to the client
    CommandResponse m_Samplers;

    /// starts the shader debugger
    StartShaderDebuggerCommand m_StartShaderDebugger;

    /// stops the shader debugger
    CommandResponse m_StopShaderDebugger;
};

#endif //_FD_PIPELINE_SHADER_STAGE_H
