//==============================================================================
// Copyright (c) 2014-2015 Advanced Micro Devices, Inc. All rights reserved.
/// \author AMD Developer Tools Team
/// \file
/// \brief  The ObjectDatabaseProcessor is responsible for queries sent to the
/// object database. Through this processor, we can collect data related to
/// API objects that have been wrapped and stored within the database.
//==============================================================================

#ifndef OBJECTDATABASEPROCESSOR_H
#define OBJECTDATABASEPROCESSOR_H

class IInstanceBase;
class WrappedObjectDatabase;

#include "ILayer.h"
#include "CommandProcessor.h"

typedef std::map<gtASCIIString, int> ObjectTypeNameToValueMap;

//--------------------------------------------------------------------------
/// The ObjectDatabaseProcessor is a processor that is not only responsible
/// for maintaining handles to the stored content, and can be used to query
/// the stored object instances to retrieve info.
//--------------------------------------------------------------------------
class ObjectDatabaseProcessor : public ILayer, public CommandProcessor
{
    // TSingleton
    friend TSingleton<ObjectDatabaseProcessor>;

public:
    ObjectDatabaseProcessor();
    virtual ~ObjectDatabaseProcessor();

    // ILayer
    virtual bool OnCreate(CREATION_TYPE type, void* pPtr);
    virtual bool OnDestroy(CREATION_TYPE type, void* pPtr);
    virtual void BeginFrame();
    virtual void EndFrame();

    // CommandProcessor
    virtual string GetDerivedSettings() { return ""; }

    //--------------------------------------------------------------------------
    /// Use the DatabaseProcessor to retrieve a pointer to the object database instance.
    /// Must be implemented by subclass ObjectDatabaseProcessor types.
    //--------------------------------------------------------------------------
    virtual WrappedObjectDatabase* GetObjectDatabase() = 0;

protected:
    //--------------------------------------------------------------------------
    /// Retrieve the object type enumeration value from a type string.
    /// \param inObjectTypeString A string containing the type of object to get the value for.
    /// \returns The enumeration value for the incoming object type string.
    //--------------------------------------------------------------------------
    virtual int GetObjectTypeFromString(const gtASCIIString& inObjectTypeString) const = 0;

    //--------------------------------------------------------------------------
    /// A helper function that does the dirty work of building object XML.
    //--------------------------------------------------------------------------
    void BuildObjectTreeResponse(gtASCIIString& outObjectTreeXml);

    //--------------------------------------------------------------------------
    /// Parse a comma-separated string of addresses into a list of pointers.
    /// \param inAddressesString A string containing comma-separated hex addresses.
    /// \param outAddressStrings A vector of address pointers.
    //--------------------------------------------------------------------------
    bool ParseAddressesString(const gtASCIIString& inAddressesString, std::vector<void*>& outAddressStrings);

    //--------------------------------------------------------------------------
    /// Return the value of the first object in the API's typelist.
    /// \returns the value of the first object in the API's type list.
    //--------------------------------------------------------------------------
    virtual int GetFirstObjectType() const = 0;

    //--------------------------------------------------------------------------
    /// Return the value of the last object in the API's typelist.
    /// \returns the value of the last object in the API's type list.
    //--------------------------------------------------------------------------
    virtual int GetLastObjectType() const = 0;

    //--------------------------------------------------------------------------
    /// Return the value of the Device type in the API's type list.
    /// \returns the value of the Device type in the API's type list.
    //--------------------------------------------------------------------------
    virtual int GetDeviceType() const = 0;

protected:
    //--------------------------------------------------------------------------
    /// A member that points to the currently-selected object within the object database.
    /// The selection is changed by responding to messages from the client.
    //--------------------------------------------------------------------------
    IInstanceBase* mSelectedObject;

private:
    //--------------------------------------------------------------------------
    /// Respond to object selection change commands.
    //--------------------------------------------------------------------------
    void UpdateSelectedObject();

    //--------------------------------------------------------------------------
    /// A command that responds with an object's type.
    //--------------------------------------------------------------------------
    CommandResponse mObjectTypeResponse;

    //--------------------------------------------------------------------------
    /// A command that responds with an object's tag data.
    //--------------------------------------------------------------------------
    CommandResponse mObjectTagResponse;

    //--------------------------------------------------------------------------
    /// A command that responds with an object's CreateInfo structure.
    //--------------------------------------------------------------------------
    CommandResponse mObjectCreateInfoResponse;

    //--------------------------------------------------------------------------
    /// A command that responds with a large tree of XML representing active devices and objects.
    //--------------------------------------------------------------------------
    CommandResponse mObjectTreeResponse;

    //--------------------------------------------------------------------------
    /// This command responds with a large dump of CreateInfo XML for objects with
    /// a given type. The argument is an instance of eObjectType.
    //--------------------------------------------------------------------------
    TextCommandResponse mAllCreateInfo;

    //--------------------------------------------------------------------------
    /// A CommandResponse used by the client to change the currently selected object. When there's a request for
    /// tag data or create info responses, this is updated first, and then the response is generated using
    /// the selected wrapper object.
    //--------------------------------------------------------------------------
    TextCommandResponse mSelectedObjectResponse;
};

#endif // OBJECTDATABASEPROCESSOR_H
