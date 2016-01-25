//==============================================================================
/// Copyright (c) 2015 Advanced Micro Devices, Inc. All rights reserved.
/// \author AMD Developer Tools Team
/// \file ModernAPILayerManager.h
/// \brief A LayerManager implementation for use with Modern rendering APIs.
//==============================================================================

#ifndef MODERNAPILAYERMANAGER_H
#define MODERNAPILAYERMANAGER_H

#include "LayerManager.h"
#include "CommonTypes.h"
#include "timer.h"
#include <tinyxml.h>

//--------------------------------------------------------------------------
/// A structure that contains basic information about a rendered frame.
//--------------------------------------------------------------------------
class FrameInfo
{
public:
    //--------------------------------------------------------------------------
    /// Default constructor for the FrameInfo class.
    //--------------------------------------------------------------------------
    FrameInfo();

    //--------------------------------------------------------------------------
    /// Default destructor for the FrameInfo class.
    //--------------------------------------------------------------------------
    ~FrameInfo();

    //--------------------------------------------------------------------------
    /// Read a chunk of metadata XML to populate all members.
    /// \param inMetadataXML A string of XML metadata that will be parsed.
    /// \returns True if parsing was successful. False if an error occurred.
    //--------------------------------------------------------------------------
    bool ReadFromXML(const gtASCIIString& inMetadataXML);

    //--------------------------------------------------------------------------
    /// Write a chunk of XML containing the basic frame information.
    /// \param outMetadataXML A generated chunk of XML based on the contents of this instance.
    //--------------------------------------------------------------------------
    void WriteToXML(gtASCIIString& outMetadataXML);

    //--------------------------------------------------------------------------
    /// The total elapsed time that the instrumented application has been running for.
    //--------------------------------------------------------------------------
    float mTotalElapsedTime;

    //--------------------------------------------------------------------------
    /// The total duration (CPU-side timing) of the frame since the last present.
    //--------------------------------------------------------------------------
    float mFrameDuration;

    //--------------------------------------------------------------------------
    /// The total number of API calls that occurred within the traced frame.
    //--------------------------------------------------------------------------
    UINT mAPICallCount;

    //--------------------------------------------------------------------------
    /// The total number of draw calls that occurred within the traced frame.
    //--------------------------------------------------------------------------
    UINT mDrawCallCount;
};

//--------------------------------------------------------------------------
/// The FrameInfoXMLVisitor will visit each XML Element and extra data to
/// populate the given FrameInfo instance.
//--------------------------------------------------------------------------
class FrameInfoXMLVisitor : public TiXmlVisitor
{
public:
    //--------------------------------------------------------------------------
    /// Default constructor for the FrameInfoXMLVisitor class.
    /// \param inFrameInfoInstance The FrameInfo instance that will be populated by this visitor.
    //--------------------------------------------------------------------------
    FrameInfoXMLVisitor(FrameInfo* inFrameInfoInstance)
        : mFrameInfoInstance(inFrameInfoInstance)
    {
    }

    //--------------------------------------------------------------------------
    /// A visitor implementation which visits each element within the visited XML document.
    /// \param inElement The current element being visited.
    /// \param inFirstAttribute The first attribute in the element being visited.
    /// \returns True.
    //--------------------------------------------------------------------------
    virtual bool VisitEnter(const TiXmlElement& inElement, const TiXmlAttribute* /*inFirstAttribute*/)
    {
        const char* elementName = inElement.Value();
        const char* elementText = inElement.GetText();

        // Step through each element, looking for specific names. Extract what we need to fill out our structure.
        if (elementName != NULL && elementText != NULL)
        {
            if (strcmp(elementName, "ElapsedTime") == 0)
            {
                float totalElapsedTime = static_cast<float>(atof(elementText));
                mFrameInfoInstance->mTotalElapsedTime = totalElapsedTime;
            }
            else if (strcmp(elementName, "CPUFrameDuration") == 0)
            {
                float frameDuration = static_cast<float>(atof(elementText));
                mFrameInfoInstance->mFrameDuration = frameDuration;
            }
            else if (strcmp(elementName, "APICallCount") == 0)
            {
                UINT apiCallCount = static_cast<UINT>(atoi(elementText));
                mFrameInfoInstance->mAPICallCount = apiCallCount;
            }
            else if (strcmp(elementName, "DrawCallCount") == 0)
            {
                UINT drawCallCount = static_cast<UINT>(atoi(elementText));
                mFrameInfoInstance->mDrawCallCount = drawCallCount;
            }
        }

        return true;
    }

private:
    //--------------------------------------------------------------------------
    /// The FrameInfo instance that's going to be populated with this visitor.
    //--------------------------------------------------------------------------
    FrameInfo* mFrameInfoInstance;
};

//--------------------------------------------------------------------------
/// The ModernAPILayerManager is intentded to be used with Modern rendering
/// APIs. Shared functionality can be found implemented here.
//--------------------------------------------------------------------------
class ModernAPILayerManager : public LayerManager
{
public:
    //--------------------------------------------------------------------------
    /// Default constructor for ModernAPILayerManager.
    //--------------------------------------------------------------------------
    ModernAPILayerManager();

    //--------------------------------------------------------------------------
    /// Default destructor for ModernAPILayerManager.
    //--------------------------------------------------------------------------
    virtual ~ModernAPILayerManager();

    //--------------------------------------------------------------------------
    /// The layer must create its resources here, it may hook some functions if really needed
    /// \param type the creation object type.
    /// \param pPtr Pointer to the object that was just created.
    /// \return True if success, False if fail.
    //--------------------------------------------------------------------------
    virtual bool OnCreate(CREATION_TYPE type, void* pPtr);

    //--------------------------------------------------------------------------
    /// Destroy all layers tracked by this LayerManager.
    /// \param type The device type (GR_DEVICE) that's being tracked.
    /// \param pPtr A pointer to the device that's being tracked my this server plugin.
    /// \return True always.
    //--------------------------------------------------------------------------
    virtual bool OnDestroy(CREATION_TYPE type, void* pPtr);

    //--------------------------------------------------------------------------
    /// Begin the frame.
    //--------------------------------------------------------------------------
    virtual void BeginFrame();

    //--------------------------------------------------------------------------
    /// End the frame.
    //--------------------------------------------------------------------------
    virtual void EndFrame();

    //--------------------------------------------------------------------------
    /// A handler invoked when Autocapture mode has been triggered.
    //--------------------------------------------------------------------------
    virtual void AutocaptureTriggered();

    //--------------------------------------------------------------------------
    /// Create a new CommandObject to be processed by the LayerManager.
    //--------------------------------------------------------------------------
    bool ProcessRequestFromCommId(CommunicationID inRequestId);

    //--------------------------------------------------------------------------
    /// Retrieve basic timing and API usage information by filling in a FrameInfo
    /// instance.
    /// \param outFrameInfo The FrameInfo structure that will be populated with data.
    //--------------------------------------------------------------------------
    virtual void GetFrameInfo(FrameInfo& outFrameInfo) = 0;

protected:
    //--------------------------------------------------------------------------
    /// Retrieve the total time that the application has been running in seconds.
    /// \returns The total time that the application has been running in seconds.
    //--------------------------------------------------------------------------
    float GetElapsedTime();

    //--------------------------------------------------------------------------
    /// Retrieve the total CPU duration of the last rendered frame in seconds.
    /// \returns The total CPU duration of the last rendered frame in seconds.
    //--------------------------------------------------------------------------
    float GetCPUFrameDuration() const;

private:
    //--------------------------------------------------------------------------
    /// A command responsible for retrieving basic frame information as a chunk
    /// of formatted XML.
    //--------------------------------------------------------------------------
    CommandResponse mCmdGetCurrentFrameInfo;

    //--------------------------------------------------------------------------
    /// A timer instance to measure the total run time, and per-frame CPU time.
    //--------------------------------------------------------------------------
    Timer mFrameTimer;

    //--------------------------------------------------------------------------
    /// The total duration of the last rendered frame in milliseconds;
    //--------------------------------------------------------------------------
    double mFrameDuration;
};

#endif // MODERNAPILAYERMANAGER_H