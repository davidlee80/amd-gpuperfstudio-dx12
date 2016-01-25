//==============================================================================
// Copyright (c) 2015 Advanced Micro Devices, Inc. All rights reserved.
/// \author AMD Developer Tools Team
/// \file
/// \brief  The baseclass for Mantle and DX12 TraceAnalyzerLayers.
//==============================================================================

#include "MultithreadedTraceAnalyzerLayer.h"
#include "misc.h"
#include "SharedGlobal.h"
#include "ModernAPILayerManager.h"
#include "../Common/OSwrappers.h"
#include <AMDTOSWrappers/Include/osProcess.h>
#include <AMDTOSWrappers/Include/osFile.h>
#include <AMDTOSWrappers/Include/osTime.h>
#include <AMDTOSWrappers/Include/osDirectory.h>
#include "../Common/xml.h"
#include <tinyxml.h>

//--------------------------------------------------------------------------
/// The MetadataXMLVisitor will visit each XML Element and extract data
/// used to populate the given TraceMetadata instance.
//--------------------------------------------------------------------------
class MetadataXMLVisitor : public TiXmlVisitor
{
public:
    //--------------------------------------------------------------------------
    /// Constructor used to indicate which TraceMetadata instance is going to be populated.
    /// \param inMetadataInstance The TraceMetadata instance that is going to be populated.
    //--------------------------------------------------------------------------
    MetadataXMLVisitor(TraceMetadata* inMetadataInstance)
    : mMetadataInstance(inMetadataInstance)
    , mFrameInfoVisitor(&inMetadataInstance->mFrameInfo)
    {
    }

    //--------------------------------------------------------------------------
    /// A visitor implementation which visits each element within the visited XML document.
    /// \param inElement The current element being visited.
    /// \param inFirstAttribute The first attribute in the element being visited.
    /// \returns True.
    //--------------------------------------------------------------------------
    virtual bool VisitEnter(const TiXmlElement& inElement, const TiXmlAttribute* inFirstAttribute)
    {
        const char* elementName = inElement.Value();
        const char* elementText = inElement.GetText();

        // Step through each element, looking for specific names. Extract what we need to fill out our structure.
        if (elementName != NULL && elementText != NULL)
        {
            if (strcmp(elementName, "Location") == 0)
            {
                mMetadataInstance->mMetadataFilepath.assign(elementText);
            }
            else if (strcmp(elementName, "FrameNumber") == 0)
            {
                int frameIndex = atoi(elementText);
                mMetadataInstance->mFrameIndex = frameIndex;
            }
            else if (strcmp(elementName, "Architecture") == 0)
            {
                mMetadataInstance->mArchitecture = static_cast<osModuleArchitecture>(atoi(elementText));
            }
            else if (strcmp(elementName, "LinkedTrace") == 0)
            {
                mMetadataInstance->mPathToTraceFile.assign(elementText);
            }
            else if (strcmp(elementName, "FrameBufferImage") == 0)
            {
                mMetadataInstance->mPathToFrameBufferImage.assign(elementText);
            }
            else
            {
                mFrameInfoVisitor.VisitEnter(inElement, inFirstAttribute);
            }
        }

        return true;
    }

private:
    //--------------------------------------------------------------------------
    /// The TraceMetadata instance that's going to be populated with this visitor.
    //--------------------------------------------------------------------------
    TraceMetadata* mMetadataInstance;

    //--------------------------------------------------------------------------
    /// An XML element visitor responsible for extracting FrameInfo elements.
    //--------------------------------------------------------------------------
    FrameInfoXMLVisitor mFrameInfoVisitor;
};

//--------------------------------------------------------------------------
/// Default constructor for the TraceMetadata class.
//--------------------------------------------------------------------------
TraceMetadata::TraceMetadata()
: mFrameIndex(0)
, mArchitecture(OS_UNKNOWN_ARCHITECTURE)
{
    mFrameInfo = {};
    mPathToTraceFile.clear();
    mPathToFrameBufferImage.clear();
}

//--------------------------------------------------------------------------
/// Default destructor for the TraceMetadata class.
//--------------------------------------------------------------------------
TraceMetadata::~TraceMetadata()
{
}

//--------------------------------------------------------------------------
/// Read a chunk of metadata XML to populate all members.
/// \param inMetadataXML A string of XML metadata that will be parsed.
/// \returns True if parsing was successful. False if an error occurred.
//--------------------------------------------------------------------------
bool TraceMetadata::ReadFromXML(const gtASCIIString& inMetadataXML)
{
    TiXmlDocument xmlDocument;
    xmlDocument.Parse(inMetadataXML.asCharArray());

    // Create a visitor, which will populate "this" TraceMetadata instance.
    MetadataXMLVisitor elementVisitor(this);
    bool bVisistedSuccessfully = xmlDocument.Accept(&elementVisitor);

    return bVisistedSuccessfully;
}

//--------------------------------------------------------------------------
/// Write a chunk of XML that will ultimately be written to disk as a metadata file.
/// \param outMetadataXML A generated chunk of XML based on the contents of this instance.
//--------------------------------------------------------------------------
void TraceMetadata::WriteToXML(gtASCIIString& outMetadataXML)
{
    gtASCIIString metadataXML;

    metadataXML += XML("Location",          FormatText("%s", mMetadataFilepath.c_str()).asCharArray());
    metadataXML += XML("FrameNumber",       FormatText("%u", mFrameIndex).asCharArray());
    metadataXML += XML("Architecture",      FormatText("%u", mArchitecture).asCharArray());

    gtASCIIString contentsXML;

    contentsXML += XML("LinkedTrace",       FormatText("%s", mPathToTraceFile.c_str()).asCharArray());
    contentsXML += XML("FrameBufferImage",  FormatText("%s", mPathToFrameBufferImage.c_str()).asCharArray());

    // Pull this section of the contents out of the internal FrameInfo structure.
    mFrameInfo.WriteToXML(contentsXML);

    // Surround the generated contents string with a parent element
    metadataXML += XML("Contents", contentsXML.asCharArray());

    // Now surround the entire chunk with an "XML" block so there's a single root node.
    outMetadataXML = XML("XML", metadataXML.asCharArray());
}

//--------------------------------------------------------------------------
/// MultithreadedTraceAnalyzerLayer's default constructor, which doesn't really do much.
//--------------------------------------------------------------------------
MultithreadedTraceAnalyzerLayer::MultithreadedTraceAnalyzerLayer()
    : TraceAnalyzer()
    , mbGPUTraceAlreadyCollected(false)
    , mbCollectingApiTrace(false)
    , mbCollectingGPUTrace(false)
    , mbWaitingForAutocaptureClient(false)
{

    // Command that collects a CPU and GPU trace from the same frame.
    AddCommand(CONTENT_TEXT, "LinkedTrace", "LinkedTrace", "LinkedTrace.txt", DISPLAY, INCLUDE, mCmdLinkedTrace);

    // Command that collects a CPU and GPU trace from the same frame.
    AddCommand(CONTENT_TEXT, "LinkedTraceWithSave", "LinkedTraceWithSave", "LinkedTraceWithSave.txt", DISPLAY, INCLUDE, mCmdLinkedTraceWithSave);

    // Command that collects a Linked Trace, but immediately caches it to disk. The response is the server-relative path to a metadata file describing the trace.
    AddCommand(CONTENT_TEXT, "GetCachedTraceResponse", "GetCachedTraceResponse", "GetCachedTraceResponse.txt", NO_DISPLAY, INCLUDE, mRetrieveCachedTraceResponse);
    mRetrieveCachedTraceResponse.SetEditableContentAutoReply(false);

    // Command used to automatically trace a target frame in an instrumented application.
    AddCommand(CONTENT_TEXT, "AutoTrace", "AutoTrace", "AutoTrace.txt", DISPLAY, INCLUDE, mCmdAutoCaptureCachedTrace);
}

//--------------------------------------------------------------------------
/// MultithreadedTraceAnalyzerLayer's destructor, in which all trace buffers are destroyed.
//--------------------------------------------------------------------------
MultithreadedTraceAnalyzerLayer::~MultithreadedTraceAnalyzerLayer()
{
    // Destroy all of the buffered trace data.
    ClearCPUThreadTraceData();
}

//--------------------------------------------------------------------------
/// BeginFrame is called when a new frame is started, and should setup the
/// collection process to log each Mantle call issued during the frame.
/// \return Nothing.
//--------------------------------------------------------------------------
void MultithreadedTraceAnalyzerLayer::BeginFrame()
{
    // Check if automatic tracing is enabled for a specific frame. Determine which trace type by examining the result.
    int autotraceFlags = GetTraceTypeFlags();

    // Pick up new requests that are active and flip the switch to begin tracing calls.
    bool bLinkedTraceRequested = (mCmdLinkedTrace.IsActive() || mCmdLinkedTraceWithSave.IsActive()) || (autotraceFlags == kTraceType_Linked);

    // If a linked trace is required, turn on both trace switches.
    bool bAPITraceNeeded = m_apiTraceTXT.IsActive() || bLinkedTraceRequested || (autotraceFlags & kTraceType_API);
    bool bGPUTraceNeeded = m_cmdGPUTrace.IsActive() || bLinkedTraceRequested || (autotraceFlags & kTraceType_GPU);

    if (bAPITraceNeeded || bGPUTraceNeeded)
    {
        // Clear out the previous trace data before tracing the new frame.
        Clear();

        // We need to enable tracing no matter what so that we go into the PreCall/PostCall.
        InterceptorBase* interceptor = GetInterceptor();
        interceptor->SetCollectTrace(true);

        if (bAPITraceNeeded)
        {
            // Enable global trace collection so API and GPU trace can happen.
            BeforeAPITrace();
            mbCollectingApiTrace = true;
        }

        if (bGPUTraceNeeded)
        {
            BeforeGPUTrace();

            // Enable GPU time collection
            interceptor->SetProfilingEnabled(true);
            mbCollectingGPUTrace = true;
        }

        mFramestartTime = mFramestartTimer.GetRaw();
    }
}

//--------------------------------------------------------------------------
/// EndFrame is used to signal the end of a rendered frame and to stop/send
/// the captured results that were logged during the frame render.
/// \return Nothing.
//--------------------------------------------------------------------------
void MultithreadedTraceAnalyzerLayer::EndFrame()
{
    // Check again which trace type is active at the end of the frame. Need to match how it was started.
    int autotraceFlags = GetTraceTypeFlags();

    // If the linked trace was requested, return all of the results through a single response.
    bool bLinkedTraceRequested = (mCmdLinkedTrace.IsActive() || mCmdLinkedTraceWithSave.IsActive()) || (autotraceFlags == kTraceType_Linked);

    // Will we be required to dump the trace response to a file on disk?
    bool bSaveResponseToFile = mCmdLinkedTraceWithSave.IsActive();

    // If we want a linked trace, we'll need responses from all trace types.
    bool bAPITraceResponseNeeded = m_apiTraceTXT.IsActive() || bLinkedTraceRequested || (autotraceFlags & kTraceType_API);
    bool bGPUTraceResponseNeeded = m_cmdGPUTrace.IsActive() || bLinkedTraceRequested || (autotraceFlags & kTraceType_GPU);

    if (bAPITraceResponseNeeded || bGPUTraceResponseNeeded)
    {
        // We're done collecting, so turn off the interception and profiler switch.
        InterceptorBase* interceptor = GetInterceptor();
        interceptor->SetCollectTrace(false);
        interceptor->SetProfilingEnabled(false);

        AfterAPITrace();
        AfterGPUTrace();

        std::string apiTraceResponseString, gpuTraceResponseString;

        if (bAPITraceResponseNeeded)
        {
            mbCollectingApiTrace = false;
            apiTraceResponseString.assign(GetAPITraceTXT().c_str());
        }

        if (bGPUTraceResponseNeeded)
        {
            mbGPUTraceAlreadyCollected = false;
            gpuTraceResponseString.assign(GetGPUTraceTXT().c_str());
        }

        std::stringstream fullResponseString;

        if (bAPITraceResponseNeeded)
        {
#if !defined(CODEXL_GRAPHICS)

            // In Linked trace mode, insert a separator indicating the split between response types.
            if (bLinkedTraceRequested)
            {
                fullResponseString << "//Type:API" << std::endl;
            }

#endif
            fullResponseString << apiTraceResponseString.c_str() << std::endl;
        }

        if (bGPUTraceResponseNeeded)
        {
#if !defined(CODEXL_GRAPHICS)

            // In Linked trace mode, insert a separator indicating the split between response types.
            if (bLinkedTraceRequested)
            {
                fullResponseString << "//Type:GPU" << std::endl;
            }

#endif
            fullResponseString << gpuTraceResponseString.c_str() << std::endl;
        }

        // If the autotrace flags are anything besides "None," we'll just store the trace log internally so the client can pick it up later.
        bool bShouldCacheForAutotrace = (autotraceFlags != kTraceType_None);

        if (bShouldCacheForAutotrace)
        {
            // Don't send the response back through a command yet.
            // The client will know to pick it up through a special AutoCapture command.
            mCachedTraceResponse.assign(fullResponseString.str());
            mbWaitingForAutocaptureClient = true;
        }
        else
        {
            // Send the response string back to the server through a specific request command.
            if (bLinkedTraceRequested)
            {
                HandleLinkedTraceResponse(fullResponseString, bSaveResponseToFile);
            }
            else
            {
                if (bAPITraceResponseNeeded)
                {
                    m_apiTraceTXT.Send(apiTraceResponseString.c_str());
                }
                else if (bGPUTraceResponseNeeded)
                {
                    m_cmdGPUTrace.Send(gpuTraceResponseString.c_str());
                }
            }
        }

    }

    // When AutoCapture is enabled, we need to delay rendering so the user has time to retrieve the cached response.
    if (mbWaitingForAutocaptureClient)
    {
        // For AutoCapture the client will send the separate request below to collect the cached trace response.
        if (mCmdAutoCaptureCachedTrace.IsActive())
        {
            // We're done waiting to send the cached response to the client. Move on with playback.
            mbWaitingForAutocaptureClient = false;

            mCmdAutoCaptureCachedTrace.Send(mCachedTraceResponse.c_str());
            mCachedTraceResponse.clear();
        }
        else
        {
            // Sleep to give the user a chance to connect during playback.
            osSleep(500);
        }
    }

    if (mRetrieveCachedTraceResponse.IsActive())
    {
        const char* pathToTraceMetadata = mRetrieveCachedTraceResponse.GetValue();
        if (strlen(pathToTraceMetadata) > 0)
        {
            // Read the metadata file and store the contents in a structure.
            TraceMetadata traceMetadata;
            bool bReadMetadataFileSuccessfully = ReadTraceMetadataFile(pathToTraceMetadata, traceMetadata);
            if (bReadMetadataFileSuccessfully)
            {
                gtASCIIString traceContents;
                bool bReadTraceSuccessfully = LoadTraceFile(traceMetadata.mPathToTraceFile, traceContents);
                if (bReadTraceSuccessfully)
                {
                    // At this point the full trace response text should be loaded into our string and ready to be sent back to the client.
                    mRetrieveCachedTraceResponse.Send(traceContents.asCharArray());
                }
                else
                {
                    Log(logERROR, "Failed to read trace file at '%s'.", traceMetadata.mPathToTraceFile.c_str());
                }
            }
            else
            {
                Log(logERROR, "Failed to read metadata file at '%s'.", pathToTraceMetadata);
            }
        }
        else
        {
            Log(logERROR, "Failed to locate valid path to trace metadata file.");
        }
    }
}

//--------------------------------------------------------------------------
/// Read a trace's metadata file and store the parsed info into the output argument.
/// \param inPathToMetadataFile The full path to the metadata file on disk.
/// \param outTraceMetadata A structure containing all parsed info from the metadata file.
/// \returns True if reading the metadata file was successful.
//--------------------------------------------------------------------------
bool MultithreadedTraceAnalyzerLayer::ReadTraceMetadataFile(const std::string& inPathToMetadataFile, TraceMetadata& outTraceMetadata)
{
    bool bReadSuccessfully = true;

    // We have a path to a cached trace metadata file. Load the metadata file, and return the trace response text.
    std::wstring widePathToMetadataFile;
    widePathToMetadataFile.assign(inPathToMetadataFile.begin(), inPathToMetadataFile.end());
    gtString metadataFilepathGTString;
    metadataFilepathGTString.appendFormattedString(L"%s", widePathToMetadataFile.c_str());

    osFilePath metadataFilepath(metadataFilepathGTString);
    osFile metadataFile;
    metadataFile.setPath(metadataFilepath);

    // Open the metadata file, and dump the XML into a string.
    bool bMetadataFileOpened = metadataFile.open(osChannel::OS_ASCII_TEXT_CHANNEL);
    if (bMetadataFileOpened)
    {
        gtASCIIString metadataXML;
        bool bReadSuccessful = metadataFile.readIntoString(metadataXML);
        if (bReadSuccessful)
        {
            bReadSuccessfully = outTraceMetadata.ReadFromXML(metadataXML);
        }
        else
        {
            Log(logERROR, "Failed to read metadata XML from file.\n");
        }
    }
    else
    {
        Log(logERROR, "Failed to open trace metadata XML file at '%s'.\n", inPathToMetadataFile.c_str());
    }

    return bReadSuccessfully;
}

//--------------------------------------------------------------------------
/// Write a trace's metadata file and return the contenst through the out-param.
/// \param inFullResponseString The full response string for a collected linked trace request.
/// \param outMetadataXML The XML metadata string to return to the client.
/// \returns True if writing the metadata file was successful.
//--------------------------------------------------------------------------
bool MultithreadedTraceAnalyzerLayer::WriteTraceAndMetadataFiles(const std::stringstream& inFullResponseString, std::string& outMetadataXML)
{
    bool bWrittenSuccessfully = false;

    // Empty out the incoming path to the metadata file. We'll know the exact path later.
    outMetadataXML.assign("");

    osModuleArchitecture moduleArchitecture;
    osRuntimePlatform currentPlatform;
    gtString executablePath;
    gtString commandLine;
    gtString workingDirectory;

    // Retrieve the name of the instrumented application. Construct a metadata filename which references it.
    if (osGetProcessLaunchInfo(osGetCurrentProcessId(), moduleArchitecture, currentPlatform, executablePath, commandLine, workingDirectory) == true)
    {
        osFilePath executableFilepath;
        executableFilepath.setFullPathFromString(executablePath);

        gtString appName;
        if (executableFilepath.getFileName(appName) == true)
        {
            osTime currentTime;
            currentTime.setFromCurrentTime();
            tm timeStruct;
            currentTime.timeAsTmStruct(timeStruct, osTime::LOCAL);

            // Need to add 1900, since tm contains "years since 1900".
            int year = timeStruct.tm_year + 1900;

            // Need to add 1, since tm contains "months since January".
            int month = timeStruct.tm_mon + 1;
            int day = timeStruct.tm_mday;

            int hour = timeStruct.tm_hour;
            int minute = timeStruct.tm_min;
            int second = timeStruct.tm_sec;

            gtASCIIString metadataFilename;

            // ExecutableFilename-YEAR-MM-DD-HOUR-MINUTE-SECOND
            metadataFilename.appendFormattedString("description-%s-%d-%d-%d-%d-%d-%d.xml",
                appName.asASCIICharArray(),
                year, month, day,
                hour, minute, second);

            // Build a path to the GPS folder within the Temp directory.
            osFilePath systemTempDirectory;
            systemTempDirectory.setPath(osFilePath::OS_TEMP_DIRECTORY);


            gtString toolDirectory;
#ifdef CODEXL_GRAPHICS
            toolDirectory.fromASCIIString("CodeXL");
#else
            toolDirectory.fromASCIIString(GetPerfStudioDirName());
#endif
            // @TODO: Construct a path from the temp + tool directory.
            systemTempDirectory.appendSubDirectory(toolDirectory);

            gtList<osFilePath> toolDirectoryPaths;
            osDirectory d;
            d.setDirectoryPath(systemTempDirectory);

            // @TODO: Find a better way to create the "Session" directory. We shouldn't need to search through existing session directories.
            int maxSessionIndex = 0;
            bool bGotSubDirectories = d.getSubDirectoriesPaths(osDirectory::SORT_BY_NAME_DESCENDING, toolDirectoryPaths);
            if (bGotSubDirectories)
            {
                // Step through each directory, and look for "Session" folders.
                gtList<osFilePath>::iterator subdirectoryIter;
                for (subdirectoryIter = toolDirectoryPaths.begin(); subdirectoryIter != toolDirectoryPaths.end(); ++subdirectoryIter)
                {
                    gtString subdirString;
                    osFilePath subdir = *subdirectoryIter;
                    subdir.getFileName(subdirString);

                    if (subdir.isDirectory() && subdirString.startsWith(L"Session"))
                    {
                        // Remove the "Session" part of the string. We're only interested in the number at the end.
                        subdirString.replace(L"Session", L"", true);

                        const char* sessionIndexAsString = subdirString.asASCIICharArray();
                        int thisSessionId = atoi(sessionIndexAsString);

                        if (thisSessionId > maxSessionIndex)
                        {
                            maxSessionIndex = thisSessionId;
                        }
                    }

                }
            }


            gtASCIIString pathToDataDirectory = systemTempDirectory.asString().asASCIICharArray();

            // Metadata files will be saved to the temp directory with the following filename scheme:
            // "%TEMP%/ToolDirectory/Session[Index]/ApplicationBinaryName/Frame[Index]/description.xml"

            int frameIndex = GetInterceptor()->GetParentLayerManager()->GetFrameCount();

            // Generate a "Session" folder with a number at the end. Compute the correct number by looking at the
            // Session folders that already exist
            gtASCIIString sessionString;
            sessionString.appendFormattedString("Session%d", maxSessionIndex + 1);

            pathToDataDirectory.appendFormattedString("\\%s\\%s\\Frame%d\\", sessionString.asCharArray(), appName.asASCIICharArray(), frameIndex);

            // Create the data directory if it doesn't already exist.
            gtString fullPathToDataDirectoryAsGTString;
            fullPathToDataDirectoryAsGTString.fromASCIIString(pathToDataDirectory.asCharArray());

            osDirectory dir;
            dir.setDirectoryFullPathFromString(fullPathToDataDirectoryAsGTString);
            if (!dir.exists())
            {
                bool bDirectoryCreated = dir.create();
                if (!bDirectoryCreated)
                {
                    Log(logERROR, "Failed to create data directory for traced frame: '%s'.\n", fullPathToDataDirectoryAsGTString.asASCIICharArray());
                }
            }

            gtASCIIString pathToMetadataFile = pathToDataDirectory;
            pathToMetadataFile.appendFormattedString("%s", metadataFilename.asCharArray());

            gtString fullMetadataFilepathAsGTString;
            fullMetadataFilepathAsGTString.fromASCIIString(pathToMetadataFile.asCharArray());

            osFile metadataFile(fullMetadataFilepathAsGTString);
            bool bMetadataFileOpened = metadataFile.open(osChannel::OS_ASCII_TEXT_CHANNEL, osFile::OS_OPEN_TO_WRITE);

            // If we've successfully opened the metadata file, we'll also attempt to write the trace file.
            if (bMetadataFileOpened)
            {
                osFilePath traceFileDirectory;
                traceFileDirectory.setPath(osFilePath::OS_TEMP_DIRECTORY);

                traceFileDirectory.appendSubDirectory(toolDirectory);

                // Construct a filename for the cached trace response.
                gtASCIIString fullTraceFilename;
                fullTraceFilename.appendFormattedString("LinkedTrace-%s-%d-%d-%d-%d-%d-%d.ltr", appName.asASCIICharArray(), year, month, day, hour, minute, second);

                gtASCIIString fullTraceFilePath = pathToDataDirectory;
                fullTraceFilePath.appendFormattedString("%s", fullTraceFilename.asCharArray());

                gtString fullTraceResponseFilepathGTString;
                fullTraceResponseFilepathGTString.fromASCIIString(fullTraceFilePath.asCharArray());

                // Write the contents of the trace response file.
                osFile traceResponseFile(fullTraceResponseFilepathGTString);
                bool bTraceResponseFileOpened = traceResponseFile.open(osChannel::OS_ASCII_TEXT_CHANNEL, osFile::OS_OPEN_TO_WRITE);
                if (bTraceResponseFileOpened)
                {
                    // Dump the response into an ASCII string.
                    std::string responseAsString = inFullResponseString.str();

                    // Now put the ASCII string into a gtString so we can write it to the open file.
                    gtString gtStringResponse;
                    std::wstring wideString;
                    wideString.assign(responseAsString.begin(), responseAsString.end());
                    gtStringResponse.appendFormattedString(L"%s", wideString.c_str());
                    traceResponseFile.writeString(gtStringResponse);
                    traceResponseFile.close();
                }
                else
                {
                    Log(logERROR, "Failed to write trace response to file: '%s'\n", fullTraceResponseFilepathGTString.asASCIICharArray());
                }

                // Write the filename for the associated trace response that was just collected.
                std::string traceFilepathAsString;
                traceFilepathAsString.assign(fullTraceResponseFilepathGTString.asASCIICharArray());

                TraceMetadata metadata;

                // Insert the location of the metadata file being written out.
                metadata.mMetadataFilepath = pathToMetadataFile.asCharArray();

                // Insert the path to the cached trace file.
                metadata.mPathToTraceFile = fullTraceFilePath.asCharArray();

                // @TODO: When we have a framebuffer image system working, assign the path-to-image here.
                metadata.mPathToFrameBufferImage = "UNKNOWNPATH";

                ModernAPILayerManager* layerManager = GetInterceptor()->GetParentLayerManager();

                FrameInfo frameInfo;

                layerManager->GetFrameInfo(frameInfo);

                // Populate the metadata structure with the values stored in the LayerManager.
                metadata.mFrameInfo = frameInfo;
                metadata.mFrameIndex = frameIndex;
                metadata.mArchitecture = moduleArchitecture;

                // Write the metadata xml into the output file.
                gtASCIIString metadataXMLString;
                metadata.WriteToXML(metadataXMLString);
                gtString metadataXMLAsGTString;
                metadataXMLAsGTString.fromASCIIString(metadataXMLString.asCharArray());

                // Write the metadata XML into the file, and close.
                metadataFile.writeString(metadataXMLAsGTString);
                metadataFile.close();

                // The client will receive the full metadata XML string to parse.
                outMetadataXML.assign(metadataXMLString.asCharArray());

                bWrittenSuccessfully = true;
            }
            else
            {
                Log(logERROR, "Failed to open trace metadata file for writing: '%s'\n", metadataFilename);
            }
        }
        else
        {
            Log(logERROR, "Failed to retrieve the instrumented process's application filename.\n");
        }
    }
    else
    {
        Log(logERROR, "Failed to retrieve process launch info for target application.\n");
    }

    return bWrittenSuccessfully;
}

//--------------------------------------------------------------------------
/// Load a trace file from disk when given a valid path.
/// \param inTraceFilepath The full filepath to a trace file.
/// \param outTraceFileContents The full contents of the loaded trace file.
/// \returns True if the trace file was loaded correctly.
//--------------------------------------------------------------------------
bool MultithreadedTraceAnalyzerLayer::LoadTraceFile(const std::string& inTraceFilepath, gtASCIIString& outTraceFileContents)
{
    bool bReadSuccessful = false;
    gtString traceFilepath;
    traceFilepath.fromASCIIString(inTraceFilepath.c_str());
    osFile traceFile(traceFilepath);
    bool bTraceFileOpened = traceFile.open(osChannel::OS_ASCII_TEXT_CHANNEL);
    if (bTraceFileOpened)
    {
        // Read the entire file and return the contents through the output string.
        if (traceFile.readIntoString(outTraceFileContents))
        {
            bReadSuccessful = true;
        }
        else
        {
            Log(logERROR, "Failed to read trace file at path '%s'.", inTraceFilepath.c_str());
        }
    }

    return bReadSuccessful;
}

//--------------------------------------------------------------------------
/// Clear and destroy all of the traced thread data.
//--------------------------------------------------------------------------
void MultithreadedTraceAnalyzerLayer::ClearCPUThreadTraceData()
{
    if (!mThreadTraces.empty())
    {
        ScopeLock threadTraceLock(&mTraceMutex);

        // Kill all thread trace buffers so we can start over on the next frame, or just to shut the layer down.
        std::map<DWORD, ThreadTraceData*>::iterator threadIter;

        for (threadIter = mThreadTraces.begin(); threadIter != mThreadTraces.end(); ++threadIter)
        {
            ThreadTraceData* traceData = threadIter->second;
            SAFE_DELETE(traceData);
        }

        mThreadTraces.clear();
    }
}

//--------------------------------------------------------------------------
/// Handle what happens when a Linked Trace is requested. We can either:
/// 1. Return the trace response as normal.
/// 2. Cache the response to disk, and generate a "trace metadata" file used to retrieve the trace later.
/// \param inFullResponseString The response string built by tracing the application.
/// \param inbSaveResponseToFile A switch used to determine which response method to use.
//--------------------------------------------------------------------------
void MultithreadedTraceAnalyzerLayer::HandleLinkedTraceResponse(std::stringstream& inFullResponseString, bool inbSaveResponseToFile)
{
    // If we're building for use with CodeXL, insert extra metadata into the response before returning.
#if defined(CODEXL_GRAPHICS)
    // The response should include a header when connected to CodeXL Graphics.
    gtASCIIString headerBuilder;
    headerBuilder.appendFormattedString("//AMD CodeXL Frame Trace\n");

    osModuleArchitecture moduleArchitecture;
    osRuntimePlatform currentPlatform;
    gtString executablePath;
    gtString commandLine;
    gtString workingDirectory;

    if (osGetProcessLaunchInfo(osGetCurrentProcessId(), moduleArchitecture, currentPlatform, executablePath, commandLine, workingDirectory) == true)
    {
        headerBuilder.appendFormattedString("//ProcessExe=%s\n", executablePath.asASCIICharArray());

        // Build a timestamp.
        osTime currentTime;
        currentTime.setFromCurrentTime();
        tm timeStruct;
        currentTime.timeAsTmStruct(timeStruct, osTime::LOCAL);

        // Need to add 1900, since tm contains "years since 1900".
        int year = timeStruct.tm_year + 1900;

        // Need to add 1, since tm contains "months since January".
        int month = timeStruct.tm_mon + 1;

        int day = timeStruct.tm_mday;
        int hour = timeStruct.tm_hour;
        int minute = timeStruct.tm_min;
        int second = timeStruct.tm_sec;

        gtASCIIString timestampBuilder;
        timestampBuilder.appendFormattedString("%d/%d/%d %d:%d:%d", month, day, year, hour, minute, second);
        headerBuilder.appendFormattedString("//TraceDateTime=%s\n", timestampBuilder.asCharArray());

        headerBuilder.appendFormattedString("//TraceFileVersion=%d\n", 1);
        headerBuilder.appendFormattedString("//ApplicationArgs=%s\n", commandLine.asASCIICharArray());
        headerBuilder.appendFormattedString("//WorkingDirectory=%s\n", workingDirectory.asASCIICharArray());
    }
    else
    {
        Log(logERROR, "Failed to retrieve process info when building response header.\n");
    }

    // Build a system information header string.
    std::string systemInfo;
    OSWrappers::WriteSystemInfoString(systemInfo);
    headerBuilder.appendFormattedString("\n%s\n", systemInfo.c_str());

    // Store the response temporarily.
    std::string headerString = headerBuilder.asCharArray();
    std::string responseString = inFullResponseString.str();

    // Clear the response string stream.
    inFullResponseString.str(std::string());
    inFullResponseString.clear();

    // Write the header and response chunks into the final response.
    inFullResponseString << headerString;
    inFullResponseString << responseString;
#endif

    // Check if we want to cache the response to disk, or return it as-is.
    if (inbSaveResponseToFile)
    {
        std::string metadataXMLString;
        bool bWriteMetadataSuccessful = WriteTraceAndMetadataFiles(inFullResponseString, metadataXMLString);
        if (bWriteMetadataSuccessful)
        {
            // Send a response back to the client indicating which trace metadata file was written to disk.
            mCmdLinkedTraceWithSave.Send(metadataXMLString.c_str());
        }
        else
        {
            Log(logERROR, "Failed to write trace metadata XML.\n");
            mCmdLinkedTraceWithSave.Send("Failed");
        }
    }
    else
    {
        // Return the normal trace response string.
        // Send a response containing the API and GPU trace text.
        mCmdLinkedTrace.Send(std::string(inFullResponseString.str()).c_str());
    }
}

//--------------------------------------------------------------------------
/// Return a string with all of the logged API call data in line-delimited
/// text format. This is used within the Timeline view in the client.
/// \return A string of all of the logged API calls captured during frame render.
//--------------------------------------------------------------------------
std::string MultithreadedTraceAnalyzerLayer::GetAPITraceTXT()
{
    // A switch to determine at the last moment whether or not we should send our generated response back to the client.
    bool bWriteResponseString = false;

    // Concatenate all of the logged call lines into a single string that we can send to the client.
    std::stringstream traceString;
    std::map<DWORD, ThreadTraceData*>::iterator traceIter;

    for (traceIter = mThreadTraces.begin(); traceIter != mThreadTraces.end(); ++traceIter)
    {
        ThreadTraceData* currentTrace = traceIter->second;
        const TimingLog& currentTimer = currentTrace->mAPICallTimer;
        GPS_TIMESTAMP timeFrequency = currentTimer.GetTimeFrequency();
        const GPS_TIMESTAMP frameStartTime = mFramestartTime;
        size_t numEntries = currentTrace->mLoggedCallVector.size();

        // When using the updated trace format, include a preamble section for each traced thread.
#if defined(CODEXL_GRAPHICS)
        // Write the trace type, API, ThreadID, and count of APIs traced.
        traceString << "//==API Trace==" << std::endl;
        traceString << "//API=" << GetAPIString() << std::endl;
        traceString << "//ThreadID=" << traceIter->first << std::endl;
        traceString << "//ThreadAPICount=" << numEntries << std::endl;
#endif

        for (size_t entryIndex = 0; entryIndex < numEntries; ++entryIndex)
        {
            // Get each logged call by index.
            const CallsTiming& callTiming = currentTimer.GetTimingByIndex(entryIndex);

            double deltaStartTime, deltaEndTime;
            bool conversionResults = currentTimer.ConvertTimestampToDoubles(callTiming.m_startTime,
                                                                            callTiming.m_endTime,
                                                                            deltaStartTime,
                                                                            deltaEndTime,
                                                                            frameStartTime,
                                                                            &timeFrequency);

            // We should always be able to convert from GPS_TIMESTAMPs to doubles.
            PsAssert(conversionResults == true);
            (void)conversionResults;

            const APIEntry* callEntry = currentTrace->mLoggedCallVector[entryIndex];

            // This exists as a sanity check. If a duration stretches past this point, we can be pretty sure something is messed up.
            // This signal value is basically random, with the goal of it being large enough to catch any obvious duration errors.
            if (deltaEndTime > 8000000000.0f)
            {
                const char* functionName = callEntry->GetAPIName();
                Log(logWARNING, "The duration for APIEntry '%s' with index '%d' is suspicious. Tracing the application may have hung, producing inflated results.\n", functionName, entryIndex);
            }

            callEntry->AppendAPITraceLine(traceString, deltaStartTime, deltaEndTime);
        }

        bWriteResponseString = true;
    }

    // If for some reason we failed to write a valid response string, reply with a known failure signal so the client handles it properly.
    if (!bWriteResponseString)
    {
        traceString << "NODATA";
    }

    return traceString.str();
}

//-----------------------------------------------------------------------------
/// Return GPU-time in text format, to be parsed by the Client and displayed as its own timeline.
/// \return A line-delimited, ASCII-encoded, version of the GPU Trace data.
//-----------------------------------------------------------------------------
std::string MultithreadedTraceAnalyzerLayer::GetGPUTraceTXT()
{
    return "NODATA";
}

//--------------------------------------------------------------------------
/// Check if the newly-started frame should be automatically traced at a specific frame.
/// \returns An eTraceType value, corresponding to the value set in the server config.
//--------------------------------------------------------------------------
int MultithreadedTraceAnalyzerLayer::GetTraceTypeFlags()
{
    int traceTypeFlags = kTraceType_None;

    // It's time to capture a frame. Check the trace type that was requested.
    InterceptorBase* interceptor = GetInterceptor();
    LayerManager* thisLayerManager = interceptor->GetParentLayerManager();

    if (thisLayerManager->IsAutocaptureFrame())
    {
        traceTypeFlags = SG_GET_INT(OptionTraceType);
    }

    return traceTypeFlags;
}

//--------------------------------------------------------------------------
/// Retrieve the number of API calls invoked within a traced frame.
/// \returns The number of API calls invoked within a traced frame.
//--------------------------------------------------------------------------
uint32 MultithreadedTraceAnalyzerLayer::GetNumTracedAPICalls()
{
    uint32 totalAPICalls = 0;

    // Step through each ThreadTraceData and add up the total number of API calls.
    ThreadIdToTraceData::iterator threadDataIter;
    for (threadDataIter = mThreadTraces.begin(); threadDataIter != mThreadTraces.end(); ++threadDataIter)
    {
        ThreadTraceData* traceData = threadDataIter->second;
        totalAPICalls += static_cast<uint32>(traceData->mLoggedCallVector.size());
    }

    return totalAPICalls;
}

//--------------------------------------------------------------------------
/// Retrieve the number of Draw calls that occurred within a traced frame.
/// \returns The number of Draw calls that occurred within a traced frame.
//--------------------------------------------------------------------------
uint32 MultithreadedTraceAnalyzerLayer::GetNumTracedDrawCalls()
{
    uint32 totalDrawCalls = 0;

    // Step through each ThreadTraceData and add up the total number of API calls.
    ThreadIdToTraceData::iterator threadDataIter;
    for (threadDataIter = mThreadTraces.begin(); threadDataIter != mThreadTraces.end(); ++threadDataIter)
    {
        ThreadTraceData* traceData = threadDataIter->second;

        size_t numCalls = traceData->mLoggedCallVector.size();
        for (size_t callIndex = 0; callIndex < numCalls; ++callIndex)
        {
            APIEntry* currentEntry = traceData->mLoggedCallVector[callIndex];
            if (currentEntry->IsDrawCall())
            {
                totalDrawCalls++;
            }
        }
    }

    return totalDrawCalls;
}

//--------------------------------------------------------------------------
/// This is called before the target function call, and will setup a new ThreadTraceData
/// structure if necessary, but will also track the start time for a function call.
/// \return Nothing.
//--------------------------------------------------------------------------
void MultithreadedTraceAnalyzerLayer::BeforeAPICall()
{
    // Find the correct ThreadTraceData instance and inject the precall time.
    // A single thread will only ever deal with tracing one function at a time, so we can
    // leave "this" traced function's start time in the per-thread data.
    DWORD threadId = osGetCurrentThreadId();
    ThreadTraceData* currentThreadData = FindOrCreateThreadData(threadId);
    currentThreadData->m_startTime = currentThreadData->mAPICallTimer.GetRaw();
}

//--------------------------------------------------------------------------
/// Invoked when the MantleTraceAnalyzerLayer is created.
/// \param inType The incoming type of interface being created.
/// \param pInPtr An interface pointer being created.
/// \returns True if creation was successful.
//--------------------------------------------------------------------------
bool MultithreadedTraceAnalyzerLayer::OnCreate(CREATION_TYPE inType, void* pInPtr)
{
    PS_UNREFERENCED_PARAMETER(inType);
    PS_UNREFERENCED_PARAMETER(pInPtr);

    return true;
}

//--------------------------------------------------------------------------
/// Invoked when the MantleTraceAnalyzerLayer is finished and about to be destroyed.
/// \param inType The incoming type of instance being destroyed.
/// \param pInPtr An instance pointer being destroyed.
/// \returns True if destruction was successful.
//--------------------------------------------------------------------------
bool MultithreadedTraceAnalyzerLayer::OnDestroy(CREATION_TYPE inType, void* pInPtr)
{
    PS_UNREFERENCED_PARAMETER(inType);
    PS_UNREFERENCED_PARAMETER(pInPtr);

    Clear();

    return true;
}

//--------------------------------------------------------------------------
/// Clear all logged data. Overridden because this TraceAnalyzer collects multiple
/// instances of the same data, so it all has to be deallocated.
//--------------------------------------------------------------------------
void MultithreadedTraceAnalyzerLayer::Clear()
{
    ClearCPUThreadTraceData();

    // We've just killed all the APIEntries above, so our profiling results list is invalid. Clear it as well.
    ClearProfilingResults();
}

//--------------------------------------------------------------------------
/// Find an existing ThreadTraceData instance to drop things into, or create a new one and insert into the map.
/// Each thread will receive its own ThreadTraceData instance to log to, allowing multithreaded collection.
/// \param inThreadId The Id of a thread invoking API calls.
/// \returns A polymorphic ThreadTraceData instance for the thread to log to without any locking.
//--------------------------------------------------------------------------
ThreadTraceData* MultithreadedTraceAnalyzerLayer::FindOrCreateThreadData(DWORD inThreadId)
{
    // We don't need to lock yet. map::find is threadsafe.
    ThreadTraceData* resultTraceData = NULL;
    ThreadIdToTraceData::iterator traceIter = mThreadTraces.find(inThreadId);

    if (traceIter != mThreadTraces.end())
    {
        resultTraceData = traceIter->second;
    }
    else
    {
        // We need to lock here- we're going to insert a new instance into our thread trace map.
        ScopeLock mapInsertionLock(&mTraceMutex);

        // Insert the new ThreadData struct into the map to hold them all.
        resultTraceData = CreateThreadTraceDataInstance();
        mThreadTraces[inThreadId] = resultTraceData;
    }

    return resultTraceData;
}