//==============================================================================
// Copyright (c) 2008-2015 Advanced Micro Devices, Inc. All rights reserved.
/// \author AMD Developer Tools Team
/// \file
/// \brief  TODO add documentation
//==============================================================================

#if defined (_LINUX)
    #include "WinDefs.h"
    #include "SaveImage.h"  // for BITMAPFILEHEADER / BITMAPINFOHEADER
#endif
#include "ShaderDebugger.h"
#include "ShaderDebuggerHostGPS2.h"
#include "ErrorImage.h"
#include "Logger.h"
#include "xml.h"
#include "misc.h"
#include "math.h"
#include <algorithm>
#include "AMDT-DDS.h"
#include "GDT_MemUtils.h"

#ifdef _WIN32
CShaderDebuggerHostGPS2::CShaderDebuggerHostGPS2(AMD_ShaderDebugger::CShaderDebuggerDevice* pDevice)
    : AMD_ShaderDebugger::CShaderDebuggerHost(pDevice),
      m_SDCurBufferType(ShaderUtils::BT_Unknown),
      m_SDCurBufferWidth(0),
      m_SDCurBufferHeight(0),
      m_SDCurBufferDepth(0),
      m_bInconsistentDrawCalls(false)
{
    AddCommand(CONTENT_XML, "SDRegister",      "Registers",      "registers.xml",      NO_DISPLAY, INCLUDE, m_PSRegisters);
    AddCommand(CONTENT_XML, "SDState",         "State",          "state.xml",          NO_DISPLAY, INCLUDE, m_PSDebuggerState);
    AddCommand(CONTENT_BMP, "SDDrawMask",      "Draw Mask",      "drawmask.bmp",       NO_DISPLAY, INCLUDE, m_PSDrawMaskBuffer);
    AddCommand(CONTENT_XML, "SDStep",          "Step",           "step.xml",           NO_DISPLAY, INCLUDE, m_step);
    AddCommand(CONTENT_XML, "SDBack",          "Back",           "back.xml",           NO_DISPLAY, INCLUDE, m_back);
    AddCommand(CONTENT_XML, "SDRun",           "Run",            "run.xml",            NO_DISPLAY, INCLUDE, m_run);
    AddCommand(CONTENT_XML, "SDRunBack",       "RunBack",        "runback.xml",        NO_DISPLAY, INCLUDE, m_runback);

    m_PSTempRegisterBuffer.SetEditableContentAutoReply(false);
    AddCommand(CONTENT_BMP, "SDTempRegBuffer", "Temp Reg Buffer", "tempregister.bmp" , NO_DISPLAY, INCLUDE, m_PSTempRegisterBuffer);

    m_PSTempRegisterRawBuffer.SetEditableContentAutoReply(false);
    AddCommand(CONTENT_DDS, "SDTempRegRawBuffer", "Temp Reg Raw Buffer", "tempregister.dds" , NO_DISPLAY, INCLUDE, m_PSTempRegisterRawBuffer);

    // we need to update the shader debugger when a breakpoint is toggled, so set AutoReply to false
    m_toggleBreakpoint.SetEditableContentAutoReply(false);
    AddCommand(CONTENT_XML, "SDBreakpoint",    "Toggle Breakpoint", "breakpoint.xml" , NO_DISPLAY, INCLUDE, m_toggleBreakpoint);
}

CShaderDebuggerHostGPS2::~CShaderDebuggerHostGPS2(void)
{

}

void CShaderDebuggerHostGPS2::EndDebugging(bool bUpdateOriginalBuffers)
{
    m_SDCurBufferType = ShaderUtils::BT_Unknown;
    AMD_ShaderDebugger::CShaderDebuggerHost::EndDebugging(bUpdateOriginalBuffers);

    m_bInconsistentDrawCalls = false;
}

void CShaderDebuggerHostGPS2::LogMessage(AMD_ShaderDebugger::LogType type, const char* pMessage)
{
    LogType gpsType = NA_LOG_TYPE;

    switch (type)
    {
        case AMD_ShaderDebugger::logRAW:     gpsType = logRAW; break;

        case AMD_ShaderDebugger::logASSERT:  gpsType = logASSERT; break;

        case AMD_ShaderDebugger::logERROR:   gpsType = logERROR; break;

        case AMD_ShaderDebugger::logWARNING: gpsType = logWARNING; break;

        case AMD_ShaderDebugger::logMESSAGE: gpsType = logMESSAGE; break;

        // the following three fall through to be logTRACE
        case AMD_ShaderDebugger::logVERBOSE:
        case AMD_ShaderDebugger::logMAX:
        case AMD_ShaderDebugger::logTRACE:   gpsType = logTRACE; break;
    }

    Log(gpsType, "SD: %s", pMessage);
}

bool CShaderDebuggerHostGPS2::SendRequestedData(IDrawCall& rDrawCall)
{
    // first check for drawcall consistency
    if (GetDrawCall().GetXML() != rDrawCall.GetXML())
    {
        Log(logERROR, "ShaderDebugger detected inconsistent drawcalls. Please make sure your application has a determinant set of drawcalls.\n");
        m_bInconsistentDrawCalls = true;
    }

    // string to indicate that there were inconsistent drawcalls
    gtASCIIString strIDCError = "";

    if (m_bInconsistentDrawCalls == true)
    {
        strIDCError = " Inconsistent drawcalls detected.";
    }

    if (m_step.IsActive())
    {
        if (!m_bInconsistentDrawCalls && Step())
        {
            m_step.Send("OK");
        }
        else
        {
            gtASCIIString errorString = "Could not step.";
            errorString += strIDCError;
            m_step.SendError(errorString.asCharArray());
        }
    }

    if (m_back.IsActive())
    {
        if (!m_bInconsistentDrawCalls && Back())
        {
            m_back.Send("OK");
        }
        else
        {
            gtASCIIString errorString = "Could not step backwards.";
            errorString += strIDCError;
            m_back.SendError(errorString.asCharArray());
        }
    }

    if (m_run.IsActive())
    {
        if (!m_bInconsistentDrawCalls && Run())
        {
            m_run.Send("OK");
        }
        else
        {
            gtASCIIString errorString = "Could not run.";
            errorString += strIDCError;
            m_run.SendError(errorString.asCharArray());
        }
    }

    if (m_runback.IsActive())
    {
        if (!m_bInconsistentDrawCalls && RunBack())
        {
            m_runback.Send("OK");
        }
        else
        {
            gtASCIIString errorString = "Could not run back.";
            errorString += strIDCError;
            m_runback.SendError(errorString.asCharArray());
        }
    }

    if (m_toggleBreakpoint.IsActive())
    {
        if (!m_bInconsistentDrawCalls && ToggleBreakPointAtLineNumber((AMD_ShaderDebugger::LineNumber) m_toggleBreakpoint, true))
        {
            m_toggleBreakpoint.Send("OK");
        }
        else
        {
            m_toggleBreakpoint.SendError("Failed to toggle breakpoint at line %lu. %s", m_toggleBreakpoint.GetValue(), strIDCError.asCharArray());
        }
    }

    if (m_PSRegisters.IsActive())
    {
        if (!m_bInconsistentDrawCalls)
        {
            SendPixelShaderDebuggerRegisters(&m_PSRegisters);
        }
        else
        {
            m_PSRegisters.SendError(strIDCError.asCharArray());
        }
    }

    if (m_PSDebuggerState.IsActive())
    {
        if (!m_bInconsistentDrawCalls)
        {
            SendPixelShaderDebuggerState(&m_PSDebuggerState);
        }
        else
        {
            m_PSDebuggerState.SendError(strIDCError.asCharArray());
        }
    }

    if (m_PSDrawMaskBuffer.IsActive())
    {
        if (!m_bInconsistentDrawCalls)
        {
            SendPixelShaderDebuggerDrawMaskBuffer(&m_PSDrawMaskBuffer);
        }
        else
        {
            ErrorImage::Instance()->Send(&m_PSDrawMaskBuffer);
        }
    }

    if (m_PSTempRegisterBuffer.IsActive())
    {
        if (!m_bInconsistentDrawCalls)
        {
            SendPixelShaderDebuggerTempRegBuffer(&m_PSTempRegisterBuffer);
        }
        else
        {
            ErrorImage::Instance()->Send(&m_PSTempRegisterBuffer);
        }
    }

    if (m_PSTempRegisterRawBuffer.IsActive())
    {
        if (!m_bInconsistentDrawCalls)
        {
            SendPixelShaderDebuggerTempRegRawBuffer(&m_PSTempRegisterRawBuffer);
        }
        else
        {
            ErrorImage::Instance()->Send(&m_PSTempRegisterRawBuffer);
        }
    }

    return (m_bInconsistentDrawCalls == false);
}

/// Get a text string describing the data type.
/// \param[in] mode The type to describe.
/// \ret A pointer to text describing the type.
const char* GetTypeText(ShaderUtils::DataType dataType)
{
    switch (dataType)
    {
        case ShaderUtils::DT_Float32:  return "Float32";

        case ShaderUtils::DT_Int32:    // Fall-through
        case ShaderUtils::DT_SInt32:   return "SInt32";

        case ShaderUtils::DT_UInt32:   return "UInt32";

        case ShaderUtils::DT_Boolean:  return "Boolean";

        case ShaderUtils::DT_Unknown:
        default:                              return "Unknown";
    }
}

/// Get a text string describing the channel.
/// \param[in] nChannel The channel index to describe.
/// \ret A pointer to text describing the channel.
const char* GetChannelText(UINT nChannel)
{
    switch (nChannel)
    {
        case 0:  return "x";

        case 1:  return "y";

        case 2:  return "z";

        case 3:  return "w";

        default: return " ";
    }
}

/// Generate XML describing a single channel of a CShaderDebuggerVariable variable.
/// \param[in] var The variable object.
/// \param[in] nChannel The index of the channel to describe.
/// \ret A string describing the channel as XML.
gtASCIIString XMLChannel(const AMD_ShaderDebugger::CShaderDebuggerVariable& var, UINT nChannel)
{
    PsAssert(nChannel < var.m_nChannels);

    if (nChannel >= var.m_nChannels)
    {
        return "";
    }

    gtASCIIString strData;

    switch (var.m_nDataType[nChannel])
    {
        case ShaderUtils::DT_Float32:  strData = XML("value", var.m_data[nChannel].fValue); break;

        case ShaderUtils::DT_Int32:    // Fall-through
        case ShaderUtils::DT_SInt32:   strData = XML("value", var.m_data[nChannel].nValue); break;

        case ShaderUtils::DT_UInt32:   strData = XML("value", var.m_data[nChannel].uiValue); break;

        case ShaderUtils::DT_Boolean:  strData = XML("value", var.m_data[nChannel].bValue); break;

        case ShaderUtils::DT_Unknown:
        default:                              strData = XML("value", ""); break;
    }

    gtASCIIString xmlString = XML("type", GetTypeText(var.m_nDataType[nChannel]));
    xmlString += strData;
    return XML(GetChannelText(nChannel), xmlString.asCharArray());
}

void CShaderDebuggerHostGPS2::SendPixelShaderDebuggerRegisters(RegistersCommandResponse* pR)
{
    if (m_bInconsistentDrawCalls)
    {
        pR->SendError("Inconsistent drawcalls detected.");
        return;
    }

    AMD_ShaderDebugger::IShaderDebugger* pDebugger = GetDebugger();

    if (pDebugger == NULL)
    {
        pR->SendError("No debugger is available");
        return;
    }

    if (pDebugger->IsDebugging() == false)
    {
        pR->SendError("The debugger is not active");
        return;
    }
    else
    {
        gtASCIIString strDebugInfo;

        if (m_SDCurBufferType == ShaderUtils::BT_Unknown)
        {
            m_SDCurBufferWidth = pDebugger->GetBufferWidth();
            m_SDCurBufferHeight = pDebugger->GetBufferHeight();
            m_SDCurBufferDepth = pDebugger->GetBufferDepth();
            m_SDCurBufferType = pDebugger->GetBufferType();
        }

        // Set which thread (pixel) we want register values for
        ShaderUtils::ThreadID threadID;

        int x = pR->X();
        int y = pR->Y();

        if (m_SDCurBufferType == ShaderUtils::BT_Texture_3D ||
            (m_SDCurBufferType == ShaderUtils::BT_Buffer && m_SDCurBufferHeight > 1 && m_SDCurBufferDepth > 1))
        {
            threadID.type = ShaderUtils::TID_3D;

            if (x >= 0 && x < (int)(m_SDCurBufferWidth * m_SDCurBufferDepth) &&
                y >= 0 && y < (int) m_SDCurBufferHeight)
            {
                int nArrayElement = (int) floor((float) x / (float)(m_SDCurBufferWidth * m_SDCurBufferDepth));

                threadID.x = x % m_SDCurBufferWidth;
                threadID.y = y;
                threadID.z = (int) floor((float) x / (float) m_SDCurBufferWidth) % m_SDCurBufferDepth;
                threadID.nSubResource = nArrayElement;
            }
            else
            {
                threadID.x = -1;
                threadID.y = -1;
                threadID.z = -1;
                threadID.nSubResource = -1;
            }
        }
        else if (m_SDCurBufferType == ShaderUtils::BT_Texture_2D  ||
                 (m_SDCurBufferType == ShaderUtils::BT_Buffer && m_SDCurBufferHeight > 1))
        {
            threadID.type = ShaderUtils::TID_2D;

            if (x >= 0 && x < (int) m_SDCurBufferWidth &&
                y >= 0 && y < (int) m_SDCurBufferHeight)
            {
                threadID.x = x % m_SDCurBufferWidth;
                threadID.y = y;
                threadID.nSubResource = (int) floor((float)x / (float)m_SDCurBufferWidth);
            }
            else
            {
                threadID.x = -1;
                threadID.y = -1;
                threadID.nSubResource = -1;
            }
        }
        else if (m_SDCurBufferType == ShaderUtils::BT_Texture_1D  ||
                 m_SDCurBufferType == ShaderUtils::BT_Buffer)
        {
            threadID.type = ShaderUtils::TID_1D;

            if (x >= 0 && x < (int) m_SDCurBufferWidth)
            {
                threadID.x = x % m_SDCurBufferWidth;
                threadID.nSubResource = (int) floor((float)x / (float)m_SDCurBufferWidth);
            }
            else
            {
                threadID.x = -1;
                threadID.y = -1;
                threadID.nSubResource = -1;
            }
        }
        else if (m_SDCurBufferType == ShaderUtils::BT_Texture_CubeMap)
        {
            // the number in each face corresponds to the subResource index
            //     0   1   2   3
            //       |---|
            // 0     | 2 |
            //   |---|   |-------|
            // 1 | 1   4   0   5 |
            //   |---|   |-------|
            // 2     | 3 |
            //       |---|
            threadID.type = ShaderUtils::TID_2D;

            // top row
            if (y < (int) m_SDCurBufferHeight &&
                x >= (int) m_SDCurBufferWidth && x < (int) m_SDCurBufferWidth * 2)
            {
                threadID.nSubResource = 2;
                threadID.x = x - m_SDCurBufferWidth;
                threadID.y = y;
            }
            // middle row
            else if (y >= (int) m_SDCurBufferHeight && y < (int) m_SDCurBufferHeight * 2)
            {
                threadID.y = y - m_SDCurBufferHeight;

                if (x < (int) m_SDCurBufferWidth)
                {
                    threadID.nSubResource = 1;
                    threadID.x = x;
                }
                else if (x < (int) m_SDCurBufferWidth * 2)
                {
                    threadID.nSubResource = 4;
                    threadID.x = x - m_SDCurBufferWidth;
                }
                else if (x < (int) m_SDCurBufferWidth * 3)
                {
                    threadID.nSubResource = 0;
                    threadID.x = x - m_SDCurBufferWidth * 2;
                }
                else
                {
                    threadID.nSubResource = 5;
                    threadID.x = x - m_SDCurBufferWidth * 3;
                }
            }
            // bottom row
            else if (y >= (int) m_SDCurBufferHeight * 2 &&
                     x >= (int) m_SDCurBufferWidth && x < (int) m_SDCurBufferWidth * 2)
            {
                threadID.nSubResource = 3;
                threadID.x = x - m_SDCurBufferWidth;
                threadID.y = y - m_SDCurBufferHeight * 2;
            }
            else
            {
                threadID.nSubResource = 0;
                threadID.x = -1;
                threadID.y = -1;
            }
        }
        else
        {
            threadID.x = -1;
            threadID.y = -1;
            threadID.z = -1;
        }

        AMD_ShaderDebugger::CShaderDebuggerVariables* pVariables = pDebugger->GetVariables(threadID);

        if (pVariables != NULL)
        {
            gtASCIIString strVariables;
            gtASCIIString strSlot;
            std::deque<AMD_ShaderDebugger::CShaderDebuggerVariable> variables = pVariables->GetVariables();
            std::deque<AMD_ShaderDebugger::CShaderDebuggerVariable>::iterator iVar = variables.begin();

            while (iVar != variables.end())
            {
                const AMD_ShaderDebugger::CShaderDebuggerVariable& var = *iVar++;

                strSlot = XML("name", var.m_strName.c_str());
                strSlot += XML("changedMask", var.m_ChangedMask);
                strSlot += XML("channels", var.m_nChannels);

                for (UINT i = 0 ; i < var.m_nChannels; i++)
                {
                    strSlot += XMLChannel(var, i);
                }

                strVariables += XML("slot", strSlot.asCharArray());
            }

            std::string strSize;
            std::string strPos;

            switch (m_SDCurBufferType)
            {
                case ShaderUtils::BT_Buffer:
                    strSize = XML("x", pVariables->GetWidth()).asCharArray();
                    strPos = XML("x", pVariables->GetThreadID().x).asCharArray();

                    if (pVariables->GetHeight() > 1 || pVariables->GetDepth() > 1)
                    {
                        strSize += XML("y", pVariables->GetHeight()).asCharArray();
                        strPos += XML("y", pVariables->GetThreadID().y).asCharArray();

                        if (pVariables->GetDepth() > 1)
                        {
                            strSize += XML("z", pVariables->GetDepth()).asCharArray();
                            strPos += XML("z", pVariables->GetThreadID().z).asCharArray();
                        }
                    }

                    break;

                case ShaderUtils::BT_Texture_1D:
                    strSize = XML("x", pVariables->GetWidth()).asCharArray();
                    strPos = XML("x", pVariables->GetThreadID().x).asCharArray();

                    if (pVariables->GetArraySize() > 1)
                    {
                        strSize += XML("y", pVariables->GetArraySize()).asCharArray();
                        strPos += XML("y", pVariables->GetThreadID().nSubResource).asCharArray();
                    }

                    break;

                case ShaderUtils::BT_Texture_2D:
                    strSize = XML("x", pVariables->GetWidth()).asCharArray();
                    strSize += XML("y", pVariables->GetHeight()).asCharArray();
                    strPos = XML("x", pVariables->GetThreadID().x).asCharArray();
                    strPos += XML("y", pVariables->GetThreadID().y).asCharArray();

                    if (pVariables->GetArraySize() > 1)
                    {
                        strSize += XML("z", pVariables->GetArraySize()).asCharArray();
                        strPos += XML("z", pVariables->GetThreadID().nSubResource).asCharArray();
                    }

                    break;

                case ShaderUtils::BT_Texture_3D:
                    strSize = XML("x", pVariables->GetWidth()).asCharArray();
                    strSize += XML("y", pVariables->GetHeight()).asCharArray();
                    strSize += XML("z", pVariables->GetDepth()).asCharArray();
                    strPos = XML("x", pVariables->GetThreadID().x).asCharArray();
                    strPos += XML("y", pVariables->GetThreadID().y).asCharArray();
                    strPos += XML("z", pVariables->GetThreadID().z).asCharArray();

                    if (pVariables->GetArraySize() > 1)
                    {
                        strSize += XML("w", pVariables->GetArraySize()).asCharArray();
                        strPos += XML("w", pVariables->GetThreadID().nSubResource).asCharArray();
                    }

                    break;

                case ShaderUtils::BT_Texture_CubeMap:
                    strSize = XML("x", pVariables->GetWidth()).asCharArray();
                    strSize += XML("y", pVariables->GetHeight()).asCharArray();
                    strSize += XML("z", pVariables->GetArraySize()).asCharArray();
                    strPos = XML("x", pVariables->GetThreadID().x).asCharArray();
                    strPos += XML("y", pVariables->GetThreadID().y).asCharArray();
                    strPos += XML("z", pVariables->GetThreadID().nSubResource).asCharArray();
                    break;
            }

            gtASCIIString tmpString = XML("size", strSize.c_str());
            tmpString += XML("position", strPos.c_str());
            tmpString += XML("temps",  XML("float", strVariables.asCharArray()).asCharArray());
            strDebugInfo += XML("thread", tmpString.asCharArray());

            SAFE_DELETE(pVariables);
        }

        pR->Send(strDebugInfo.asCharArray());
    }
}

/// Get a text string describing the Mode
/// This will need to be moved to an API specific file once we support more than DX apis in the Shader
/// Debugger but it will do here for now.
/// \param[in] mode The mode to describe.
/// \return A pointer to text describing the current mode.
const char* GetModeText(AMD_ShaderDebugger::Mode mode)
{
    switch (mode)
    {
        case AMD_ShaderDebugger::DM_Asm:      return "Assembly";

        case AMD_ShaderDebugger::DM_Mixed:    return "Mixed";

        case AMD_ShaderDebugger::DM_Source:   return "HLSL";

        default:                              return "";
    }
}

void CShaderDebuggerHostGPS2::SendPixelShaderDebuggerState(CommandResponse* pR)
{
    if (m_bInconsistentDrawCalls)
    {
        pR->SendError("Inconsistent drawcalls detected.");
        return;
    }

    AMD_ShaderDebugger::IShaderDebugger* pDebugger = GetDebugger();
    PsAssert(pDebugger != NULL);

    if (pDebugger == NULL)
    {
        pR->SendError("No debugger is available");
        return;
    }

    gtASCIIString strDebugInfo = XML("line", pDebugger->GetNextLine());
    strDebugInfo += XML("mode", GetModeText(pDebugger->GetMode()));
    strDebugInfo += XML("forward",  true);
    strDebugInfo += XML("backward", true);

    std::set<AMD_ShaderDebugger::LineNumber> breakpoints;

    if (GetBreakPointsAsLineNumbers(&breakpoints) > 0)
    {
        std::set<AMD_ShaderDebugger::LineNumber>::iterator breakpoint = breakpoints.begin();

        while (breakpoint != breakpoints.end())
        {
            AMD_ShaderDebugger::LineNumber lineNumber = *breakpoint++;
            strDebugInfo += XML("breakpoint", lineNumber);
        }
    }

    pR->Send(strDebugInfo.asCharArray());
}

#define BYTE_MAX_FLOAT 255.0f
char ConvertFloatToByte(float f)
{
    f = (((f) * BYTE_MAX_FLOAT) + 0.5f);
    return static_cast<char>(std::min<float>((std::max<float>(f, 0.0)), 255.0));
}

void CShaderDebuggerHostGPS2::SendPixelShaderDebuggerDrawMaskBuffer(CommandResponse* pR)
{
    if (m_bInconsistentDrawCalls)
    {
        ErrorImage::Instance()->Send(pR);
        return;
    }

    AMD_ShaderDebugger::IShaderDebugger* pDebugger = GetDebugger();
    PsAssert(pDebugger != NULL);

    if (pDebugger == NULL)
    {
        ErrorImage::Instance()->Send(pR);
        return;
    }

    SendPixelShaderDebuggerBuffer(pR, pDebugger->GetExecutionMaskBuffer());
}

void CShaderDebuggerHostGPS2::SendPixelShaderDebuggerTempRegBuffer(TextCommandResponse* pR)
{
    if (m_bInconsistentDrawCalls)
    {
        ErrorImage::Instance()->Send(pR);
        return;
    }

    AMD_ShaderDebugger::IShaderDebugger* pDebugger = GetDebugger();
    PsAssert(pDebugger != NULL);

    if (pDebugger == NULL)
    {
        ErrorImage::Instance()->Send(pR);
        return;
    }

    AMD_ShaderDebugger::VariableID varID;
    varID.type = AMD_ShaderDebugger::VID_ByName;
    varID.strName = *pR;
    SendPixelShaderDebuggerBuffer(pR, pDebugger->GetRegisterResultsBuffer(varID));
}

void CShaderDebuggerHostGPS2::SendPixelShaderDebuggerTempRegRawBuffer(TextCommandResponse* pR)
{
    if (m_bInconsistentDrawCalls)
    {
        ErrorImage::Instance()->Send(pR);
        return;
    }

    AMD_ShaderDebugger::IShaderDebugger* pDebugger = GetDebugger();
    PsAssert(pDebugger != NULL);

    if (pDebugger == NULL)
    {
        ErrorImage::Instance()->Send(pR);
        return;
    }

    AMD_ShaderDebugger::VariableID varID;
    varID.type = AMD_ShaderDebugger::VID_ByName;
    varID.strName = *pR;
    SendPixelShaderDebuggerRawBuffer(pR, pDebugger->GetRegisterResultsBuffer(varID));
}

void CShaderDebuggerHostGPS2::SendPixelShaderDebuggerBuffer(CommandResponse* pR, AMD_ShaderDebugger::CShaderDebuggerBuffer* pBuffer)
{
    PsAssert(pR != NULL);

    if (m_bInconsistentDrawCalls)
    {
        ErrorImage::Instance()->Send(pR);
        return;
    }

    if (pBuffer == NULL || pBuffer->GetData() == NULL)
    {
        ErrorImage::Instance()->Send(pR);
        return;
    }

    m_SDCurBufferType = pBuffer->GetType();
    m_SDCurBufferWidth = pBuffer->GetWidth();
    m_SDCurBufferHeight = pBuffer->GetHeight();
    m_SDCurBufferDepth = pBuffer->GetDepth();
    unsigned long ulSDCurBufferArraySize = pBuffer->GetArraySize();

    PsAssert(ulSDCurBufferArraySize > 0);
    PsAssert(m_SDCurBufferWidth > 0);
    PsAssert(m_SDCurBufferHeight > 0);
    PsAssert(m_SDCurBufferDepth > 0);

    DWORD dwBytesPerPixel = (pBuffer->GetFormat() == ShaderUtils::BF_R_8) ? 1 : 4;

    DWORD dwDataSize = dwBytesPerPixel;

    DWORD dwBMPWidth = pBuffer->GetWidth();
    DWORD dwBMPHeight = pBuffer->GetHeight();

    switch (m_SDCurBufferType)
    {
        case ShaderUtils::BT_Buffer:
        case ShaderUtils::BT_Texture_1D:
        case ShaderUtils::BT_Texture_2D:
            dwDataSize *= pBuffer->GetHeight() * pBuffer->GetWidth() * ulSDCurBufferArraySize;
            dwBMPWidth *= ulSDCurBufferArraySize;
            break;

        case ShaderUtils::BT_Texture_CubeMap:
            // make the buffer big enough for a cube cross
            dwDataSize *= pBuffer->GetHeight() * pBuffer->GetWidth() * 4 * 3;
            dwBMPWidth *= 4;
            dwBMPHeight *= 3;
            break;

        case ShaderUtils::BT_Texture_3D:
            dwDataSize *= pBuffer->GetHeight() * pBuffer->GetWidth() * m_SDCurBufferDepth;
            dwBMPWidth *= m_SDCurBufferDepth;
            break;

        default:
            Log(logERROR, "Unrecognized Shader Debugger buffer type: %u\n", m_SDCurBufferType);
            ErrorImage::Instance()->Send(pR);
            return;
    }

    // Do we need a palette?
    if (dwBytesPerPixel == 1)
    {
        dwDataSize += (256 * 4);
    }

    DWORD dwSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + dwDataSize;

    char* pDataBuffer = (char*) malloc(dwSize);

    if (pDataBuffer == NULL)
    {
        Log(logERROR, "Failed to allocate memory in SendPixelShaderDebuggerBuffer\n");
        ErrorImage::Instance()->Send(pR);
        return;
    }

    // The following information is used for rendering CubeMaps

    // offset is the number of bytes from the beginning of the dest, to the location
    // where the first src byte should be written. This will change for each of the 6 faces
    // need to use row and column IDs to calculate the byte position for each face
    // the number in each face corresponds to the subResource index
    //     0   1   2   3
    //       |---|
    // 0     | 2 |
    //   |---|   |-------|
    // 1 | 1   4   0   5 |
    //   |---|   |-------|
    // 2     | 3 |
    //       |---|

    unsigned long ulSingleFaceSize = pBuffer->GetWidth() * dwBytesPerPixel;

    // this stride will move from the end of one line of the src texture to the beginning
    // of the next line. It is 3 times the width because the aspect ratio of cubecrosses are
    // 4 wide (but 1 of those is the texture currently being written, so the stride steps
    // over the other 3)
    unsigned long ulDestStrideCubeMap = ulSingleFaceSize * 3;

    // all textures will be treated as 3D arrays; those that aren't will just have an array size and depth of 1
    unsigned long ulDestStride = ulSingleFaceSize * (m_SDCurBufferDepth * ulSDCurBufferArraySize - 1);

    // now setup the bitmap header

    BITMAPFILEHEADER* pBMFH = (BITMAPFILEHEADER*) pDataBuffer;
    pBMFH->bfType = ((USHORT)(BYTE)('B') | ((USHORT)(BYTE)('M') << 8));;
    pBMFH->bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + dwDataSize;
    pBMFH->bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
    pBMFH->bfReserved1 = 0;
    pBMFH->bfReserved2 = 0;

    BITMAPINFOHEADER* pBMINFO = (BITMAPINFOHEADER*)(pDataBuffer + sizeof(BITMAPFILEHEADER));
    pBMINFO->biSize = sizeof(BITMAPINFOHEADER);
    pBMINFO->biWidth = dwBMPWidth;
    pBMINFO->biHeight = -((int) dwBMPHeight);
    pBMINFO->biCompression = 0;
    pBMINFO->biSizeImage = dwDataSize;
    pBMINFO->biXPelsPerMeter = 0;
    pBMINFO->biYPelsPerMeter = 0;
    pBMINFO->biClrUsed = 0;
    pBMINFO->biClrImportant = 0;

    // specify the number of planes and bit count
    if (pBuffer->GetFormat() == ShaderUtils::BF_RGBA_32F ||
        pBuffer->GetFormat() == ShaderUtils::BF_XYZW_32F ||
        pBuffer->GetFormat() == ShaderUtils::BF_RGBA_8)
    {
        pBMINFO->biPlanes = 1;
        pBMINFO->biBitCount = 32;
    }
    else
    {
        pBMINFO->biPlanes = 1;
        pBMINFO->biBitCount = 8;
    }

    // fill buffer data
    char* pDestData = pDataBuffer + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

    // Do we need a palette?
    if (dwBytesPerPixel == 1)
    {
        for (UINT i = 0; i < 256; i++)
        {
            *pDestData++ = *pDestData++ = *pDestData++ = *pDestData++ = (char) i;
        }
    }

    if (m_SDCurBufferType == ShaderUtils::BT_Texture_CubeMap)
    {
        unsigned long ulDestOffset[] =
        {
            // pitch * col       + ( image size            * row * # images in row )
            ulSingleFaceSize * 2 + (ulSingleFaceSize * pBuffer->GetHeight() * 1 * 4),
            ulSingleFaceSize * 0 + (ulSingleFaceSize * pBuffer->GetHeight() * 1 * 4),
            ulSingleFaceSize * 1 + (ulSingleFaceSize * pBuffer->GetHeight() * 0 * 4),
            ulSingleFaceSize * 1 + (ulSingleFaceSize * pBuffer->GetHeight() * 2 * 4),
            ulSingleFaceSize * 1 + (ulSingleFaceSize * pBuffer->GetHeight() * 1 * 4),
            ulSingleFaceSize * 3 + (ulSingleFaceSize * pBuffer->GetHeight() * 1 * 4)
        };

        // clear the buffer to a color that is similar to that control's background color (at least it is on my machine)
        memset(pDestData, 160, dwDataSize);

        // iterate through all the faces and render their output
        for (unsigned long ulFace = 0; ulFace < 6; ulFace++)
        {
            unsigned char* pSrcData = pBuffer->GetSubResource(ulFace);

            if (pSrcData == NULL)
            {
                Log(logWARNING, "SD Buffer cube map face %ul was null\n", ulFace);
                continue;
            }

            // get pointer to the beginning of the location for this face
            char* pDestFace = pDestData + ulDestOffset[ulFace];

            for (unsigned long ulRow = 0; ulRow < pBuffer->GetHeight(); ulRow++)
            {
                for (unsigned long ulCol = 0; ulCol < pBuffer->GetWidth(); ulCol++)
                {
                    if (pBuffer->GetFormat() == ShaderUtils::BF_RGBA_32F || pBuffer->GetFormat() == ShaderUtils::BF_XYZW_32F)
                    {
                        char cRed = ConvertFloatToByte(*((float*)pSrcData));   pSrcData += 4;
                        char cGreen = ConvertFloatToByte(*((float*)pSrcData)); pSrcData += 4;
                        char cBlue = ConvertFloatToByte(*((float*)pSrcData));  pSrcData += 4;
                        char cAlpha = ConvertFloatToByte(*((float*)pSrcData)); pSrcData += 4;

                        *pDestFace++ = cBlue;
                        *pDestFace++ = cGreen;
                        *pDestFace++ = cRed;
                        *pDestFace++ = cAlpha;
                    }
                    else if (pBuffer->GetFormat() == ShaderUtils::BF_RGBA_8)
                    {
                        char cRed = *pSrcData++;
                        char cGreen = *pSrcData++;
                        char cBlue = *pSrcData++;
                        char cAlpha = *pSrcData++;

                        *pDestFace++ = cBlue;
                        *pDestFace++ = cGreen;
                        *pDestFace++ = cRed;
                        *pDestFace++ = cAlpha;
                    }
                    else if (pBuffer->GetFormat() == ShaderUtils::BF_R_8)
                    {
                        char cRed = *pSrcData++;

                        *pDestFace++ = cRed;
                    }
                }

                // at the end of each row, skip the dest pointer ahead by stride
                pDestFace += ulDestStrideCubeMap;
            }
        }
    }
    else
    {
        // handles Buffer, 1D, 2D, 3D
        for (unsigned int uArrayElement = 0; uArrayElement < ulSDCurBufferArraySize; uArrayElement++)
        {
            for (unsigned long ulSlice = 0; ulSlice < m_SDCurBufferDepth; ulSlice++)
            {
                // get pointer to the beginning of the location for this element and slice
                char* pDestFace = pDestData + (ulSingleFaceSize * uArrayElement) + (ulSingleFaceSize * ulSlice);

                unsigned char* pSrcData = pBuffer->GetSubResource((uArrayElement * m_SDCurBufferDepth) + ulSlice);

                if (pSrcData == NULL)
                {
                    Log(logWARNING, "SD Buffer array element %u was null\n", uArrayElement);
                    continue;
                }

                for (unsigned long ulRow = 0; ulRow < pBuffer->GetHeight(); ulRow++)
                {
                    for (unsigned long ulCol = 0; ulCol < pBuffer->GetWidth(); ulCol++)
                    {
                        if (pBuffer->GetFormat() == ShaderUtils::BF_RGBA_32F || pBuffer->GetFormat() == ShaderUtils::BF_XYZW_32F)
                        {
                            char cRed = ConvertFloatToByte(*((float*)pSrcData));   pSrcData += 4;
                            char cGreen = ConvertFloatToByte(*((float*)pSrcData)); pSrcData += 4;
                            char cBlue = ConvertFloatToByte(*((float*)pSrcData));  pSrcData += 4;
                            char cAlpha = ConvertFloatToByte(*((float*)pSrcData)); pSrcData += 4;

                            *pDestFace++ = cBlue;
                            *pDestFace++ = cGreen;
                            *pDestFace++ = cRed;
                            *pDestFace++ = cAlpha;
                        }
                        else if (pBuffer->GetFormat() == ShaderUtils::BF_RGBA_8)
                        {
                            char cRed = *pSrcData++;
                            char cGreen = *pSrcData++;
                            char cBlue = *pSrcData++;
                            char cAlpha = *pSrcData++;

                            *pDestFace++ = cBlue;
                            *pDestFace++ = cGreen;
                            *pDestFace++ = cRed;
                            *pDestFace++ = cAlpha;
                        }
                        else if (pBuffer->GetFormat() == ShaderUtils::BF_R_8)
                        {
                            char cRed = *pSrcData++;

                            *pDestFace++ = cRed;
                        }
                        else
                        {
                            Log(logERROR, "Unrecognized SD Buffer format: %u\n", pBuffer->GetFormat());
                            ErrorImage::Instance()->Send(pR);
                            return;
                        }
                    }

                    // at the end of each row, skip the dest pointer ahead by stride
                    pDestFace += ulDestStride;
                } // end for each row
            } // end for each slice
        }
    }

    pR->Send(pDataBuffer, dwSize);

    free(pDataBuffer);
}

void CShaderDebuggerHostGPS2::SendPixelShaderDebuggerRawBuffer(CommandResponse* pR, AMD_ShaderDebugger::CShaderDebuggerBuffer* pBuffer)
{
    PsAssert(pR != NULL);

    if (m_bInconsistentDrawCalls)
    {
        ErrorImage::Instance()->Send(pR);
        return;
    }

    if (pBuffer == NULL || pBuffer->GetData() == NULL)
    {
        ErrorImage::Instance()->SendDDS(pR);
        return;
    }

    void* pDDSData = NULL;
    size_t nDDSSize = 0;

    DDS::DDS_Type eType = DDS::UnknownType;

    switch (pBuffer->GetType())
    {
        case ShaderUtils::BT_Buffer:            // Fall-through
        case ShaderUtils::BT_Texture_1D:        eType = DDS::Texture1D; break;

        case ShaderUtils::BT_Texture_2D:        eType = DDS::Texture2D; break;

        case ShaderUtils::BT_Texture_CubeMap:   eType = DDS::TextureCubeMap; break;

        case ShaderUtils::BT_Texture_3D:        eType = DDS::Texture3D; break;
    }


    if (eType == DDS::UnknownType)
    {
        Log(logERROR, "Shader debugger: DDS::UnknownType Error\n");
        ErrorImage::Instance()->SendDDS(pR);
        return;
    }

    DDS::DDS_Format eFormat = DDS::UnknownFormat;

    switch (pBuffer->GetFormat())
    {
        case ShaderUtils::BF_RGBA_32F:  eFormat = DDS::RGBA_32F; break;

        case ShaderUtils::BF_RGBA_8:    eFormat = DDS::RGBA_8; break;

        case ShaderUtils::BF_XYZW_32F:  eFormat = DDS::RGBA_32F; break;

        case ShaderUtils::BF_R_8:       eFormat = DDS::R_8; break;
    }


    if (eFormat == DDS::UnknownFormat)
    {
        Log(logERROR, "Shader debugger: DDS::UnknownFormat Error\n");
        ErrorImage::Instance()->SendDDS(pR);
        return;
    }

    // We have to use a special case for output by the ComputeShader.
    // In this case a 1DTexture will actually be computed in an NxN grid and the pBuffer's width and height will both be N.
    // This really messes things up as the draw mask image gets returned as a 2D grid but the register buffer data gets sent as 1D. We want them
    // to have the same dimensions to make the presentation in the client work properly.
    if (eType == DDS::Texture1D && pBuffer->GetHeight() > 1)
    {
        eType = DDS::Texture2D;
    }

    DDS::DDS_Error err = DDS::SaveDDSToMemory(pBuffer->GetWidth(),
                                              pBuffer->GetHeight(),
                                              pBuffer->GetDepth(),
                                              pBuffer->GetSubResourceCount(),
                                              1,
                                              eType,
                                              eFormat,
                                              pBuffer->GetData(),
                                              pBuffer->GetSize(),
                                              &pDDSData,
                                              &nDDSSize);

    if (err == DDS::Err_Ok)
    {
        pR->Send((const char*) pDDSData, (unsigned int) nDDSSize);
    }
    else
    {
        Log(logERROR, "Shader debugger: SaveDDSToMemory Error: %ld\n", err);
        ErrorImage::Instance()->SendDDS(pR);
    }

    GDT_SAFE_FREE(pDDSData);
}

#endif // _WIN32