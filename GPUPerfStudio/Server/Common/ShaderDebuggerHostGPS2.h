//==============================================================================
// Copyright (c) 2008-2015 Advanced Micro Devices, Inc. All rights reserved.
/// \author AMD Developer Tools Team
/// \file
/// \brief  TODO add documentation
//==============================================================================

#pragma once
#ifndef SHADERDEBUGGERHOSTGPS2_H
#define SHADERDEBUGGERHOSTGPS2_H

#include "ShaderDebuggerHost.h"
#include "CommandProcessor.h"
#include "ShaderDebuggerCommon.h"
#include <AMDTBaseTools/Include/gtASCIIString.h>
#include "IDrawCall.h"

/// Helper CommandResponse which can parse for X and Y values for the registers request
class RegistersCommandResponse : public CommandResponse
{
public:

    /// Default constructor
    RegistersCommandResponse():
        m_nPixelX(-1),
        m_nPixelY(-1)
    {
    }

    /// Default Destructor
    virtual ~RegistersCommandResponse()
    {
    }

    /// Accessor the X value
    int X()
    {
        return m_nPixelX;
    }

    /// Accessor the Y value
    int Y()
    {
        return m_nPixelY;
    }

private:

    /// Parse the X and Y coordinates of the pixel to retrieve register values for
    virtual bool GetParams(CommandObject& rCommObj)
    {
        int nTmpX = -1;
        int nTmpY = -1;

        if (GetParam<int>(rCommObj, "X", nTmpX) &&
            GetParam<int>(rCommObj, "Y", nTmpY))
        {
            m_nPixelX = nTmpX;
            m_nPixelY = nTmpY;
            return true;
        }

        return false;
    }

    int m_nPixelX; ///< the column of the pixel to return register values for
    int m_nPixelY; ///< the row of the pixel to return register values for
};

/// CShaderDebuggerHostGPS2 is the common implementation of CShaderDebuggerHost for GPU PerfStudio2.
/// Its main responsibility to respond to callbacks from the IShaderDebugger in order to provide a stage for shader debugging in
/// GPS. Currently it defers to the default implementation which calls the CShaderDebuggerDevice* m_pDevice member to provide
/// default functionality. It also exports a set of methods that respond to GPS2 commands requesting results.
#ifdef _WIN32
class CShaderDebuggerHostGPS2 : public AMD_ShaderDebugger::CShaderDebuggerHost, public CommandProcessor
{
protected:
    /// CShaderDebuggerHostGPS2 constructor.
    /// Declared as protected so that its instances can only be created from the CreateShaderDebuggerHostGPS2 factory method.
    /// \param[in] pDevice  A pointer to the CShaderDebuggerDevice to use for default functionality.
    CShaderDebuggerHostGPS2(AMD_ShaderDebugger::CShaderDebuggerDevice* pDevice);

public:
    /// Factory method for creating a CShaderDebuggerHostGPS2.
    /// \param[in] platform The API (DX9, DX10, etc) to debug shaders on.
    /// \param[in] pDevice  A pointer to the actual hardware device (i.e. a IDirect3DDevice9* or ID3D10Device*).
    /// \return A pointer to a new CShaderDebuggerHostGPS2 if successful otherwise NULL.
    static CShaderDebuggerHostGPS2* CreateShaderDebuggerHostGPS2(AMD_ShaderDebugger::Platform platform, void* pDevice);

    /// CShaderDebuggerHostGPS2 destructor
    /// Declared as virtual since the class has virtual members.
    virtual ~CShaderDebuggerHostGPS2(void);

    /// Override the EndDebugging function so that we can reset some variables. NOTE: ignored for DX11.
    /// \param bUpdateOriginalBuffers False will restore the UAVs to their original values, True will keep the values as produced while debugging.
    virtual void EndDebugging(bool bUpdateOriginalBuffers);

    /// Sets the drawcall that the Debugger should use for re-rendering the draw
    /// \param rDrawCall the drawcall that is being debugged
    virtual void SetDrawCall(IDrawCall& rDrawCall) = 0;

    /// Accessor to the drawcall being used for debugging
    /// \return the shader debugger's current drawcall
    virtual IDrawCall& GetDrawCall() = 0;

    /// Indicates to the ShaderDebugger to process any received commands and send back requested data
    /// \param rDrawCall the drawcall currently being executed by the application. This should be compared against the SD's cached drawcall to ensure consistency.
    bool SendRequestedData(IDrawCall& rDrawCall);

    /// No derived settings
    std::string GetDerivedSettings() { return ""; }

    /// Provides a means for the ShaderDebugger library to pass log messages to PerfStudio
    /// \param type the type of message
    /// \param pMessage the message to log
    virtual void LogMessage(AMD_ShaderDebugger::LogType type, const char* pMessage);

protected:
    /// Return an image describing the buffer as a HTTP response.
    /// \param[in,out] pR A pointer to the HTTPRespond interface.
    /// \param[in] pBuffer The buffer to output.
    virtual void SendPixelShaderDebuggerBuffer(CommandResponse* pR, AMD_ShaderDebugger::CShaderDebuggerBuffer* pBuffer);

    /// Return an image describing the raw buffer as a HTTP response.
    /// \param[in,out] pR A pointer to the HTTPRespond interface.
    /// \param[in] pBuffer The buffer to output.
    virtual void SendPixelShaderDebuggerRawBuffer(CommandResponse* pR, AMD_ShaderDebugger::CShaderDebuggerBuffer* pBuffer);

    /// Return XML describing the current debugger registers as a HTTP response.
    /// \param[in,out] pR A pointer to the HTTPRespond interface.
    virtual void SendPixelShaderDebuggerRegisters(RegistersCommandResponse* pR);

    /// Return XML describing the current debugger state as a HTTP response.
    /// \param[in,out] pR A pointer to the HTTPRespond interface.
    virtual void SendPixelShaderDebuggerState(CommandResponse* pR);

    /// Return an image describing the current debugger draw mask buffer as a HTTP response.
    /// \param[in,out] pR A pointer to the HTTPRespond interface.
    virtual void SendPixelShaderDebuggerDrawMaskBuffer(CommandResponse* pR);

    /// Return an image describing the current value of a temp register results buffer as a HTTP response.
    /// \param[in,out] pR A pointer to the HTTPRespond interface.
    virtual void SendPixelShaderDebuggerTempRegBuffer(TextCommandResponse* pR);

    /// Return an image describing the current value of a temp register results raw buffer as a HTTP response.
    /// \param[in,out] pR A pointer to the HTTPRespond interface.
    virtual void SendPixelShaderDebuggerTempRegRawBuffer(TextCommandResponse* pR);

    RegistersCommandResponse m_PSRegisters;      ///< Activated when the client requests registers
    CommandResponse m_PSDebuggerState;  ///< Activated when the client requests state
    CommandResponse m_PSDrawMaskBuffer; ///< Activated when the client requests the draw mask buffer
    CommandResponse m_step;             ///< Advances the shader debugger one line
    CommandResponse m_back;             ///< Steps back one line in the shader
    CommandResponse m_run;              ///< Indicates to the debugger to run to the next breakpoint or run to the end
    CommandResponse m_runback;          ///< Indicates to the debugger to run to the previous breakpoint or run to the start
    ULongCommandResponse m_toggleBreakpoint; ///< Turns on and off breakpoints at a specified line
    TextCommandResponse m_PSTempRegisterBuffer; ///< Activated when the client requests a temp register results buffer
    TextCommandResponse m_PSTempRegisterRawBuffer; ///< Activated when the client requests a temp register results raw buffer

    ShaderUtils::BufferType m_SDCurBufferType; ///< The type of buffer that was last sent to the client
    unsigned long m_SDCurBufferWidth;                 ///< The width of the buffer that was last sent to the client
    unsigned long m_SDCurBufferHeight;                ///< The height of the buffer that was last sent to the client
    unsigned long m_SDCurBufferDepth;                 ///< The depth of the buffer that was last sent to the client

    bool m_bInconsistentDrawCalls;      ///< flag to indicate that inconsistent drawcalls have been recognized
};
#endif // _WIN32

#endif // SHADERDEBUGGERHOSTGPS2_H
