//==============================================================================
// Copyright (c) 2008-2015 Advanced Micro Devices, Inc. All rights reserved.
/// \author AMD Developer Tools Team
/// \file
/// \brief  File contains the implementation of the base classes for the Frame Debugger Pipeline Stages
//==============================================================================

#include <typeinfo>
#include "FDPipelineShaderStage.h"
#include "misc.h"
#include "xml.h"

using namespace std;

/// Constructor
FDPipelineShaderStage::FDPipelineShaderStage() :
    FDPipelineStage()
{
    AddCommand(CONTENT_XML,  "Shader",                "Shader Debugger", "shader.xml",           NO_DISPLAY, INCLUDE, m_Shader);

    // Gets the shader code for display in the clients shader code window.
    AddCommand(CONTENT_XML,  "CodeViewer",            "Code Viewer",     "codeviewer.xml",       DISPLAY,    INCLUDE, m_CodeViewer);

#ifdef GPS_GSA
    AddCommand(CONTENT_XML,  "ShaderAnalysis",        "Analysis",       "shaderanalysis.xml",   DISPLAY,    INCLUDE, m_ShaderAnalysis);
    AddCommand(CONTENT_XML,  "ShaderHWDisassembly",   "HWDisassembly",  "hwshaders.xml",        NO_DISPLAY, INCLUDE, m_ShaderHWDisassembly);
#endif // GPS_GSA
    AddCommand(CONTENT_XML,  "Constants",             "Constants",      "constants.xml",        NO_DISPLAY, INCLUDE, m_Constants);
    AddCommand(CONTENT_XML,  "Samplers",              "Samplers",       "samplers.xml",         DISPLAY,    INCLUDE, m_Samplers);

#ifdef _WIN32
    m_pSDHost = NULL;
#endif
}

/// Destructor
FDPipelineShaderStage::~FDPipelineShaderStage()
{
};

/// Allows API-specific and shader-specific adding of the ShaderDebugger
/// \param platform That platform (API) which the app is using
void FDPipelineShaderStage::AddShaderDebuggerCommand(AMD_ShaderDebugger::Platform platform)
{
    AddCommand(CONTENT_XML, "startshaderdebugger", "Start Debugging", "startdebugging.xml", NO_DISPLAY, INCLUDE, m_StartShaderDebugger);
    AddCommand(CONTENT_XML, "stopshaderdebugger",  "Stop Debugging",  "stopdebugging.xml",  NO_DISPLAY, INCLUDE, m_StopShaderDebugger);
    m_sdPlatform = platform;
}

/// Indicates if this shader stage is in the process of debugging a shader
/// \return true if the shader is being debugged; false otherwise
bool FDPipelineShaderStage::IsShaderDebugging() const
{
#ifdef _WIN32
    return (m_pSDHost != NULL &&  m_pSDHost->IsDebugging());
#else
    return false;
#endif // _WIN32
};

void FDPipelineShaderStage::OnDrawCallAtBreakPointPreHUD(IDrawCall& rDrawCall)
{
#ifdef _WIN32
    CreateShaderDebugger(m_sdPlatform, GetActiveDevice());
#endif // _WIN32

    // Send the shader code to the shader debugger window
    if (m_Shader.IsActive())
    {
        CaptureAndSendShader(m_Shader, rDrawCall);
    }

    // Send the shader code to the CodeViewer window
    if (m_CodeViewer.IsActive())
    {
        CaptureAndSendShader(m_CodeViewer, rDrawCall);
    }

#ifdef GPS_GSA

    if (m_ShaderAnalysis.IsActive())
    {
        CaptureAndSendShaderAnalysis();
    }

    if (m_ShaderHWDisassembly.IsActive())
    {
        CaptureAndSendShaderHWDisassembly();
    }

#endif // GPS_GSA

    if (m_Constants.IsActive())
    {
        CaptureAndSendConstants();
    }

    if (m_Samplers.IsActive())
    {
        CaptureAndSendSamplers();
    }
}

/// \copydoc FDPipelineShaderStage::OnDrawCallAtBreakpoint
/// Sends back code, constants, samplers, and activates the
/// shader debugger if needed
void FDPipelineShaderStage::OnDrawCallAtBreakpoint(IDrawCall& rDrawCall)
{
    AtBreakpointCaptureAndSend(rDrawCall);
}

/// Deletes the shader debugger for this shader stage
void FDPipelineShaderStage::Cleanup()
{
#ifdef _WIN32
    DeleteShaderDebugger();
#endif // _WIN32
}

/// Generates XML that can be sent back to the client in response
/// to a request for the shader
/// \param pszHash a hash of the supplied shader
/// \param pszShaderModeXML the output of one or more calls to ShaderModeXML(...)
/// \return string formatted in XML of the available shader Modes
gtASCIIString FDPipelineShaderStage::ShaderXML(const char* pszHash,
                                               const char* pszShaderModeXML)
{
    gtASCIIString xml = XML("Info", XML("Hash", XMLEscape(pszHash).asCharArray()).asCharArray());
    xml += XML("Modes", pszShaderModeXML);
    return xml;
}

gtASCIIString FDPipelineShaderStage::ShaderModeXML(const char* pszName,
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
                                                   const char* debugName)
{
    PsAssert(pszName);
    PsAssert(pszDebugError);
    PsAssert(pszKeywords);

    gtASCIIString strContent = XML("Code", XMLEscape(pszCode).asCharArray());
    strContent += XML("KeyWords", pszKeywords);

    if (bDebuggable == false)
    {
        strContent += XML("DebugError", XMLEscape(pszDebugError).asCharArray());
    }

    if (pszEntryFunction != NULL && strlen(pszEntryFunction) > 0)
    {
        strContent += XML("FunctionName", pszEntryFunction);
        strContent += XML("FunctionLine", uLineNumber);
    }

    strContent += XML("VersionMajor", versionMajor);
    strContent += XML("VersionMinor", versionMinor);

    strContent += XML("DebugName", debugName);

    strContent += pszCompilerFlags;

    return XMLAttrib("Mode",
                     FormatText("name='%s' default='%s' debuggable='%s' editable='%s'",
                                pszName,
                                bDefault ? "TRUE" : "FALSE",
                                bDebuggable ? "TRUE" : "FALSE",
                                bEditable ? "TRUE" : "FALSE").asCharArray(),
                     strContent.asCharArray()
                    );
}

/// Converts the keywordType into a string
/// \param keyWordType the keywordType to convert
/// \return string version of the keyWordType
const char* FDPipelineShaderStage::GetKeyWordID(ShaderUtils::KeyWordType keyWordType)
{
    switch (keyWordType)
    {
        case ShaderUtils::KW_Op:              return "OpKeyWords";

        case ShaderUtils::KW_IntOp:           return "IntOpKeyWords";

        case ShaderUtils::KW_DoubleOp:        return "DoubleOpKeyWords";

        case ShaderUtils::KW_TextureOp:       return "TextureOpKeyWords";

        case ShaderUtils::KW_UnsupportedOp:   return "UnsupportedOpKeyWords";

        case ShaderUtils::KW_Function:        return "FunctionKeyWords";

        case ShaderUtils::KW_Reg:             return "RegKeyWords";

        case ShaderUtils::KW_Type:            return "TypeKeyWords";

        case ShaderUtils::KW_Semantics:       return "SemanticsKeyWords";

        case ShaderUtils::KW_Variable:        return "VariableKeyWords";

        case ShaderUtils::KW_Reserved:        return "ReservedKeyWords";

        default:                              return "UnknownKeyWords";
    }
}

/// Gets attributes that should be associated with a type of keyword
/// \param keyWordType the type to get XML attributes for
/// \return string containing the attributes
const char* FDPipelineShaderStage::GetKeyWordAttrib(ShaderUtils::KeyWordType keyWordType)
{
    switch (keyWordType)
    {
        case ShaderUtils::KW_Op:              return "index='0' name='Op Key-Words'";

        case ShaderUtils::KW_IntOp:           return "index='0' name='Integer Op Key-Words'";

        case ShaderUtils::KW_DoubleOp:        return "index='0' name='Double Op Key-Words'";

        case ShaderUtils::KW_TextureOp:       return "index='1' name='Texture Op Key-Words'";

        case ShaderUtils::KW_UnsupportedOp:   return "index='1' name='Unsupported Op Key-Words'";

        case ShaderUtils::KW_Function:        return "index='0' name='Function Key-Words'";

        case ShaderUtils::KW_Reg:             return "index='3' name='Reg Op Key-Words'";

        case ShaderUtils::KW_Type:            return "index='1' name='Type Key-Words'";

        case ShaderUtils::KW_Semantics:       return "index='3' name='Semantics Key-Words'";

        case ShaderUtils::KW_Variable:        return "index='3' name='Variable Op Key-Words'";

        case ShaderUtils::KW_Reserved:        return "index='0' name='Reserved Op Key-Words'";

        default:                              return "index='6' name='Unknown Op Key-Words'";
    }
}
