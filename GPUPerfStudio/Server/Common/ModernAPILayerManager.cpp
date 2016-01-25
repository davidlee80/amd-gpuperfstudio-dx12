//==============================================================================
/// Copyright (c) 2015 Advanced Micro Devices, Inc. All rights reserved.
/// \author AMD Developer Tools Team
/// \file ModernAPILayerManager.cpp
/// \brief A LayerManager implementation for use with Modern rendering APIs.
//==============================================================================

#include "ModernAPILayerManager.h"
#include "xml.h"
#include "SharedGlobal.h"

//--------------------------------------------------------------------------
/// Default constructor for the FrameInfo class.
//--------------------------------------------------------------------------
FrameInfo::FrameInfo()
: mTotalElapsedTime(0.0f)
, mFrameDuration(0.0f)
, mAPICallCount(0)
, mDrawCallCount(0)
{
}

//--------------------------------------------------------------------------
/// Default destructor for the FrameInfo class.
//--------------------------------------------------------------------------
FrameInfo::~FrameInfo()
{
}

//--------------------------------------------------------------------------
/// Read a chunk of metadata XML to populate all members.
/// \param inMetadataXML A string of XML metadata that will be parsed.
/// \returns True if parsing was successful. False if an error occurred.
//--------------------------------------------------------------------------
bool FrameInfo::ReadFromXML(const gtASCIIString& inMetadataXML)
{
    TiXmlDocument xmlDocument;
    xmlDocument.Parse(inMetadataXML.asCharArray());

    // Create a FrameInfoXMLVisitor, which will populate this instance.
    FrameInfoXMLVisitor elementVisitor(this);
    bool bVisitedSuccessfully = xmlDocument.Accept(&elementVisitor);

    return bVisitedSuccessfully;
}

//--------------------------------------------------------------------------
/// Write a chunk of XML containing the basic frame information.
/// \param outMetadataXML A generated chunk of XML based on the contents of this instance.
//--------------------------------------------------------------------------
void FrameInfo::WriteToXML(gtASCIIString& outMetadataXML)
{
    // Append the frame info XML onto the end of the incoming string.
    outMetadataXML += XML("ElapsedTime",        FormatText("%f", mTotalElapsedTime).asCharArray());
    outMetadataXML += XML("CPUFrameDuration",   FormatText("%f", mFrameDuration).asCharArray());
    outMetadataXML += XML("APICallCount",       FormatText("%u", mAPICallCount).asCharArray());
    outMetadataXML += XML("DrawCallCount",      FormatText("%u", mDrawCallCount).asCharArray());
}

//--------------------------------------------------------------------------
/// Default constructor for NextGenLayerManager.
//--------------------------------------------------------------------------
ModernAPILayerManager::ModernAPILayerManager()
{
    // Command that collects a CPU and GPU trace from the same frame.
    AddCommand(CONTENT_XML, "GetCurrentFrameInfo", "GetCurrentFrameInfo", "GetCurrentFrameInfo.xml", DISPLAY, INCLUDE, mCmdGetCurrentFrameInfo);
}

//--------------------------------------------------------------------------
/// Default destructor for NextGenLayerManager.
//--------------------------------------------------------------------------
ModernAPILayerManager::~ModernAPILayerManager()
{

}

//--------------------------------------------------------------------------
/// The layer must create its resources here, it may hook some functions if really needed
/// \param type the creation object type.
/// \param pPtr Pointer to the object that was just created.
/// \return True if success, False if fail.
//--------------------------------------------------------------------------
bool ModernAPILayerManager::OnCreate(CREATION_TYPE type, void* pPtr)
{
    bool bReturn = true;

    for (UINT32 i = 0; i < m_AvailableLayers.size(); i++)
    {
        if (m_AvailableLayers[i]->OnCreate(type, pPtr) == false)
        {
            Log(logERROR, "Layer with index '%u' failed in OnCreate call.\n", i);
            bReturn = false;
        }
    }

    // Reset the timer to initialize the start time.
    mFrameTimer.Reset();

    return bReturn;
}

//--------------------------------------------------------------------------
/// Destroy all layers tracked by this LayerManager.
/// \param type The device type (GR_DEVICE) that's being tracked.
/// \param pPtr A pointer to the device that's being tracked my this server plugin.
/// \return True always.
//--------------------------------------------------------------------------
bool ModernAPILayerManager::OnDestroy(CREATION_TYPE type, void* pPtr)
{
    bool bReturn = false;

    // tell each layer that the context was destroyed; Leave the last layer TimeControl layer destroyed later
    for (UINT32 i = 0; i < m_AvailableLayers.size() - 1; i++)
    {
        ILayer* pL = m_AvailableLayers[m_AvailableLayers.size() - i - 1];
        bool bDestructionSuccess = pL->OnDestroy(type, pPtr);

        if (!bDestructionSuccess)
        {
            // Something went wrong when we tried to destroy the layer- report it here.
            Log(logERROR, "Layer with index '%u' failed in OnDestroy call.\n", i);
        }

        bReturn |= bDestructionSuccess;
    }

    return bReturn;
}

//--------------------------------------------------------------------------
/// Begin the frame.
//--------------------------------------------------------------------------
void ModernAPILayerManager::BeginFrame()
{
    GetPendingRequests();

    // Call into the base class to deal with basic layer management.
    LayerManager::BeginFrame();
}

//--------------------------------------------------------------------------
/// End the frame.
//--------------------------------------------------------------------------
void ModernAPILayerManager::EndFrame()
{
    double currentTime = mFrameTimer.LapDouble();
    static double previousTime = currentTime;

    mFrameDuration = currentTime - previousTime;

    if (mCmdGetCurrentFrameInfo.IsActive())
    {
        FrameInfo frameInfo;
        GetFrameInfo(frameInfo);

        gtASCIIString frameInfoXML;
        frameInfo.WriteToXML(frameInfoXML);

        mCmdGetCurrentFrameInfo.Send(frameInfoXML.asCharArray());
    }

    // Call into the baseclass EndFrame implementation.
    LayerManager::EndFrame();

    previousTime = currentTime;
}

//--------------------------------------------------------------------------
/// A handler invoked when Autocapture mode has been triggered.
//--------------------------------------------------------------------------
void ModernAPILayerManager::AutocaptureTriggered()
{
    // Only enable AutoCapture mode with a valid trace type.
    bool bValidTraceMode = SG_GET_INT(OptionTraceType) != kTraceType_None;

    if (bValidTraceMode)
    {
        m_AutoCapture = true;
    }
}

//--------------------------------------------------------------------------
/// Process a new request received by the server plugin.
/// \param inRequestId The Id associated with the new incoming request.
/// \return True if the request was handled successfully.
//--------------------------------------------------------------------------
bool ModernAPILayerManager::ProcessRequestFromCommId(CommunicationID inRequestId)
{
    CommandObject command(inRequestId, (char*)GetRequestText(inRequestId));
    return Process(command);
}

//--------------------------------------------------------------------------
/// Retrieve the total time that the application has been running in seconds.
/// \returns The total time that the application has been running in seconds.
//--------------------------------------------------------------------------
float ModernAPILayerManager::GetElapsedTime()
{
    return static_cast<float>(mFrameTimer.LapDouble());
}

//--------------------------------------------------------------------------
/// Retrieve the total CPU duration of the last rendered frame in seconds.
/// \returns The total CPU duration of the last rendered frame in seconds.
//--------------------------------------------------------------------------
float ModernAPILayerManager::GetCPUFrameDuration() const
{
    // Return the frame duration computed within EndFrame.
    return static_cast<float>(mFrameDuration);
}