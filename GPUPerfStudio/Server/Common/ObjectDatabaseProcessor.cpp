//==============================================================================
// Copyright (c) 2014-2015 Advanced Micro Devices, Inc. All rights reserved.
/// \author AMD Developer Tools Team
/// \file
/// \brief  The ObjectDatabaseProcessor is responsible for queries sent to the
/// object database. Through this processor, we can collect data related to
/// API objects that have been wrapped and stored within the database.
//==============================================================================

#include "ObjectDatabaseProcessor.h"
#include "WrappedObjectDatabase.h"
#include "IInstanceBase.h"
#include "xml.h"

#define OBJECT_UNDEFINED -1

//--------------------------------------------------------------------------
/// Default constructor for the ObjectDatabaseProcessor class.
//--------------------------------------------------------------------------
ObjectDatabaseProcessor::ObjectDatabaseProcessor()
    : mSelectedObject(NULL)
{
    // Add each command as a child of "this" CommandProcessor, and set each to not autoreply.
    // We need to do this so that we can detect which Command is active, read the client arguments,
    // and generate a suitable response string to reply with.
    AddCommand(CONTENT_XML, "ObjType", "ObjType", "ObjType.xml", NO_DISPLAY, NO_INCLUDE, mObjectTypeResponse);
    mObjectTypeResponse.SetEditableContentAutoReply(false);

    AddCommand(CONTENT_XML, "ObjTag", "ObjTag", "ObjTag.xml", NO_DISPLAY, NO_INCLUDE, mObjectTagResponse);
    mObjectTagResponse.SetEditableContentAutoReply(false);

    AddCommand(CONTENT_XML, "ObjCreateInfo", "ObjCreateInfo", "ObjCreateInfo.xml", DISPLAY, INCLUDE, mObjectCreateInfoResponse);
    mObjectCreateInfoResponse.SetEditableContentAutoReply(false);

    AddCommand(CONTENT_XML, "ObjectTree", "ObjectTree", "ObjectTree.xml", NO_DISPLAY, NO_INCLUDE, mObjectTreeResponse);
    mObjectTreeResponse.SetEditableContentAutoReply(false);

    AddCommand(CONTENT_TEXT, "SelectedObject", "Change DB's the selected object", "SelectedObject", NO_DISPLAY, NO_INCLUDE, mSelectedObjectResponse);
    mSelectedObjectResponse.SetEditableContentAutoReply(false);

    AddCommand(CONTENT_XML, "AllCreateInfo", "AllCreateInfo", "AllCreateInfo.xml", DISPLAY, INCLUDE, mAllCreateInfo);
    mAllCreateInfo.SetEditableContentAutoReply(false);

    SetLayerName("ObjectDatabaseProcessor");
}

//--------------------------------------------------------------------------
/// Default destructor for the ObjectDatabaseProcessor class.
//--------------------------------------------------------------------------
ObjectDatabaseProcessor::~ObjectDatabaseProcessor()
{
}

//--------------------------------------------------------------------------
/// Invoked when the ObjectDatabaseProcessor is created.
/// \param type The type of object that this processor is being created specifically for.
/// \param pPtr A pointer to the object instance that was created for this layer.
/// \returns True if layer creation succeeded.
//--------------------------------------------------------------------------
bool ObjectDatabaseProcessor::OnCreate(CREATION_TYPE type, void* pPtr)
{
    PS_UNREFERENCED_PARAMETER(type);
    PS_UNREFERENCED_PARAMETER(pPtr);

    return true;
}

//--------------------------------------------------------------------------
/// Invoked when the ObjectDatabaseProcessor is destroyed.
/// \param type The type of object that's probably going to die.
/// \param pPtr A pointer to the object instance that's going to die.
/// \returns True if layer destruction succeeded.
//--------------------------------------------------------------------------
bool ObjectDatabaseProcessor::OnDestroy(CREATION_TYPE type, void* pPtr)
{
    PS_UNREFERENCED_PARAMETER(type);
    PS_UNREFERENCED_PARAMETER(pPtr);

    return true;
}

//--------------------------------------------------------------------------
/// Invoked when rendering the frame begins. Used to clean up state before a new frame's trace begins.
//--------------------------------------------------------------------------
void ObjectDatabaseProcessor::BeginFrame()
{
    // Before starting frame tracing, purge all the dead objects from the object database.
    // We only want to track existing objects and instances that are created during the new frame's tracing.
    WrappedObjectDatabase* objectDatabase = GetObjectDatabase();
    objectDatabase->TrackObjectLifetime(true);
}

//--------------------------------------------------------------------------
/// Invoked when the frame finishes rendering. At this point, we can check
/// if there are any requests to retrieve data from the object database.
//--------------------------------------------------------------------------
void ObjectDatabaseProcessor::EndFrame()
{
    // The frame is over. Don't track any new object instance creation again until we've started a new frame.
    WrappedObjectDatabase* objectDatabase = GetObjectDatabase();
    objectDatabase->TrackObjectLifetime(false);

    // Check if the selection has been changed. This should be checked first in case future responses need it.
    UpdateSelectedObject();

    if (mObjectTypeResponse.IsActive())
    {
        if (mSelectedObject != NULL)
        {
            gtASCIIString typeString;
            mSelectedObject->AppendTypeXML(typeString);
            mObjectTypeResponse.Send(typeString.asCharArray());
        }
    }

    if (mObjectTreeResponse.IsActive())
    {
        // Create a response string by querying a bunch of objects and serializing an object hierarchy.
        gtASCIIString objectTreeResponseString;
        BuildObjectTreeResponse(objectTreeResponseString);

        if (objectTreeResponseString.length() > 0)
        {
            const char* responseString = objectTreeResponseString.asCharArray();
            mObjectTreeResponse.Send(responseString);
        }
    }

    if (mObjectTagResponse.IsActive())
    {
        // Write the tag data for the selected wrapper object (if there is any) to a string.
        if (mSelectedObject != NULL)
        {
            gtASCIIString tagDataString;
            mSelectedObject->AppendTagDataXML(tagDataString);
            mObjectTagResponse.Send(tagDataString.asCharArray());
        }
    }

    // Check to see if the "Get CreateInfo" request is active. If so, look for the selected object in the wrapper database.
    // When found, write it's CreateInfo struct into the response.
    if (mObjectCreateInfoResponse.IsActive())
    {
        gtASCIIString createInfoXml;
        mSelectedObject->AppendCreateInfoXML(createInfoXml);

        mObjectCreateInfoResponse.Send(createInfoXml.asCharArray());
    }

    // Retrieve a large dump of CreateInfo XML for all objects of a given type.
    if (mAllCreateInfo.IsActive())
    {
        WrappedObjectDatabase* objDatabase = GetObjectDatabase();
        gtASCIIString allCreateInfoResponse;

        // First find out which object type we're interested in dumping.
        gtASCIIString argString(mAllCreateInfo.GetValue());

        // Is this a request to dump the info for a single object with a given handle?
        if (argString.startsWith("0x"))
        {
            // We received the handle as a hex string. Convert it to an address and look up the object in the database.
            std::vector<void*> addressHandle;
            bool bHandleConverted = ParseAddressesString(argString, addressHandle);

            if (bHandleConverted && addressHandle.size() == 1)
            {
                gtASCIIString instanceCreateInfo;

                IInstanceBase* instancePointer = objDatabase->GetWrappedInstance(addressHandle[0]);
                instancePointer->AppendCreateInfoXML(instanceCreateInfo);

                // Now include the handle as an attribute of the CreateInfo element within the response.
                gtASCIIString applicationHandleString;
                instancePointer->PrintFormattedApplicationHandle(applicationHandleString);
                gtASCIIString tagWithHandle;
                tagWithHandle.appendFormattedString("<CreateInfo handle=\"%s\"", applicationHandleString.asCharArray());
                instanceCreateInfo.replace("<CreateInfo", tagWithHandle, false);

                allCreateInfoResponse.append(instanceCreateInfo);
            }
            else
            {
                allCreateInfoResponse.appendFormattedString("Error: Failed to parse object handle '%s'.\n", argString.asCharArray());
            }
        }
        else
        {
            // This is a request to dump info for all objects with a given type.
            int objType = GetObjectTypeFromString(argString);

            int firstType = GetFirstObjectType();
            int lastType = GetLastObjectType();

            if (objType != -1 && (objType >= firstType && objType <= lastType))
            {
                // Dump each type of object in order.
                eObjectType objectTypeEnum = static_cast<eObjectType>(objType);

                WrappedInstanceVector objectVector;
                objDatabase->GetObjectsByType(objectTypeEnum, objectVector);

                if (!objectVector.empty())
                {
                    for (size_t objectIndex = 0; objectIndex < objectVector.size(); ++objectIndex)
                    {
                        gtASCIIString thisCreateInfo;
                        IInstanceBase* instancePointer = objectVector[objectIndex];
                        instancePointer->AppendCreateInfoXML(thisCreateInfo);

                        gtASCIIString applicationHandleString;
                        instancePointer->PrintFormattedApplicationHandle(applicationHandleString);

                        gtASCIIString tagWithHandle;
                        tagWithHandle.appendFormattedString("<CreateInfo handle=\"%s\"", applicationHandleString.asCharArray());
                        thisCreateInfo.replace("<CreateInfo", tagWithHandle, false);

                        allCreateInfoResponse.append(thisCreateInfo);
                    }
                }
            }
            else
            {
                allCreateInfoResponse.appendFormattedString("Error: Please choose an object type between '%d' and '%d'\n", firstType, lastType);
            }
        }

        mAllCreateInfo.Send(allCreateInfoResponse.asCharArray());
    }
}

//--------------------------------------------------------------------------
/// This CommandProcessor has responses that operate on vectors of memory addresses.
/// This utility function is responsible for splitting the input string by commas, and into a vector of addresses.
//--------------------------------------------------------------------------
bool ObjectDatabaseProcessor::ParseAddressesString(const gtASCIIString& inAddressesString, std::vector<void*>& outAddressStrings)
{
    bool bHaveAddresses = false;

    gtASCIIString addressStringCopy(inAddressesString);

    if (addressStringCopy.length() > 0)
    {
        std::list<gtASCIIString> addressList;
        addressStringCopy.Split(",", false, addressList);
        bHaveAddresses = !(addressList.empty());

        if (bHaveAddresses)
        {
            std::list<gtASCIIString>::iterator addressIter;
            addressIter = addressList.begin();

            // Convert from string to memory address.
            gtASCIIString& address = (*addressIter);
            unsigned long long memoryAddress = 0;
            bool bConversionSuccessful = address.toUnsignedLongLongNumber(memoryAddress);

            if (bConversionSuccessful)
            {
                outAddressStrings.push_back(reinterpret_cast<void*>(memoryAddress));
            }

            ++addressIter;
        }
    }

    return bHaveAddresses;
}

//--------------------------------------------------------------------------
/// Check if the selected object has been changed by the client. If so, update the selected object pointer.
//--------------------------------------------------------------------------
void ObjectDatabaseProcessor::UpdateSelectedObject()
{
    if (mSelectedObjectResponse.IsActive())
    {
        gtASCIIString appHandleASCIIString(mSelectedObjectResponse.GetValue());

        vector<void*> applicationHandles;

        if (!appHandleASCIIString.startsWith("NULL") && ParseAddressesString(appHandleASCIIString, applicationHandles))
        {
            void* selectedObject = applicationHandles[0];
            IInstanceBase* selectedWrappedInstance = GetObjectDatabase()->GetWrappedInstance(selectedObject);

            if (selectedWrappedInstance != NULL)
            {
                mSelectedObject = selectedWrappedInstance;
            }
        }
        else
        {
            // Just select the first device.
            WrappedInstanceVector devices;
            int deviceType = GetDeviceType();
            GetObjectDatabase()->GetObjectsByType((eObjectType)deviceType, devices);

            if (!devices.empty())
            {
                mSelectedObject = devices[0];
            }
        }

        // Respond to let the client know that selection worked fine.
        mSelectedObjectResponse.Send("OK");
    }
}

//--------------------------------------------------------------------------
/// A helper function that does the dirty work of building object XML.
/// \param outObjectTreeXml An out-param that contains the object tree XML response.
/// \returns Nothing. The result is returned through an out-param string.
//--------------------------------------------------------------------------
void ObjectDatabaseProcessor::BuildObjectTreeResponse(gtASCIIString& outObjectTreeXml)
{
    // Separate all of the objects into categories based on the object type.
    WrappedObjectDatabase* objectDatabase = GetObjectDatabase();

    // Find the device wrappers first- they are the roots of our treeview.
    int deviceType = GetDeviceType();
    WrappedInstanceVector deviceWrappers;
    objectDatabase->GetObjectsByType((eObjectType)deviceType, deviceWrappers);

    // Find the object instances created under each device to create a tree.
    for (size_t deviceIndex = 0; deviceIndex < deviceWrappers.size(); ++deviceIndex)
    {
        // Get the device's application handle from the wrapper. Look for objects with a handle to the parent device.
        IInstanceBase* deviceInstance = deviceWrappers[deviceIndex];

        // Don't bother building an object tree for a device that's been destroyed.
        if (!deviceInstance->IsDestroyed())
        {
            gtASCIIString applicationHandleString;
            deviceInstance->PrintFormattedApplicationHandle(applicationHandleString);

            // The object XML for each active device will be appended into a long string here.
            gtASCIIString deviceObjectXml = "";

            // Look for objects of all types besides "Device". We'll handle those last.
            int firstObjectType = GetFirstObjectType();
            int lastObjectType = GetLastObjectType();

            for (int objectType = firstObjectType; objectType < lastObjectType; objectType++)
            {
                // Step through each type of object and store the results in vectors.
                eObjectType currentType = (eObjectType)objectType;

                if (currentType == GetDeviceType())
                {
                    // Skip devices. They're the highest level node in the hierarchy. We're only looking for device children.
                    continue;
                }

                // We need to empty out the list of objects with the current type so that objects from the 0th device don't get added in.
                WrappedInstanceVector objectsOfType;
                objectDatabase->GetObjectsByType(currentType, objectsOfType);

                // There's at least a single instance of this type within the object database. Serialize the object to the tree.
                if (!objectsOfType.empty())
                {
                    // Objects of the same type will be sorted into similar groups, and then divided based on parent device.
                    gtASCIIString instancesString = "";
                    size_t numInstances = objectsOfType.size();
                    const char* objectTypeAsString = objectsOfType[0]->GetTypeAsString();

                    for (size_t instanceIndex = 0; instanceIndex < numInstances; ++instanceIndex)
                    {
                        IInstanceBase* objectInstance = objectsOfType[instanceIndex];

                        // Check to see if the object's parent device matches the one we're building the tree for.
                        if (objectInstance->GetParentDeviceHandle() == deviceInstance->GetApplicationHandle())
                        {
                            gtASCIIString objectHandleString;
                            objectInstance->PrintFormattedApplicationHandle(objectHandleString);
                            instancesString.append(objectHandleString);

                            if (objectInstance->IsDestroyed())
                            {
                                // Append a "|d" to indicate that this object instance was deleted during the frame.
                                instancesString.append("|d");
                            }

                            if ((instanceIndex + 1) < numInstances)
                            {
                                instancesString.append(",");
                            }
                        }
                    }

                    deviceObjectXml += XML(objectTypeAsString, instancesString.asCharArray());
                }
            }

            // Surround all of the subobjects in an object element.
            gtASCIIString deviceAddressAttributeString;
            deviceAddressAttributeString.appendFormattedString("handle='%s'", applicationHandleString.asCharArray());
            outObjectTreeXml += XMLAttrib("Device", deviceAddressAttributeString.asCharArray(), deviceObjectXml.asCharArray());
        }
    }

    outObjectTreeXml = XML("Objects", outObjectTreeXml.asCharArray());
}