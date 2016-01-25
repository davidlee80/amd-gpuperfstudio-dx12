//=====================================================================
// Copyright 2016 (c), Advanced Micro Devices, Inc. All rights reserved.
//
/// \author AMD Developer Tools Team
/// \file osFile.h 
/// 
//=====================================================================

//------------------------------ osFile.h ------------------------------

#ifndef __OSFILE
#define __OSFILE

// Pre-declarations:
class osFileImpl;

// Infra:
#include <AMDTBaseTools/Include/gtASCIIString.h>

// Local:
#include <AMDTOSWrappers/Include/osStream.h>
#include <AMDTOSWrappers/Include/osFilePath.h>

//Invalid characters for a file name
#if GR_BUILD_TARGET == GR_WINDOWS_OS
    #define OS_INVALID_CHARS_FOR_FILE_STR L"*?/:<>|\\"
    //[.*?/:<>%|\], the reg exp uses escape codes also
    #define OS_INVALID_CHARS_FOR_FILE_QREGEXP "[^*\\?/:<>|]*" // YuriR: ^ used to exclude characters, * means zero or more characters
#elif GR_BUILD_TARGET == GR_LINUX_OS
    #define OS_INVALID_CHARS_FOR_FILE_STR L"/"
    #define OS_INVALID_CHARS_FOR_FILE_QREGEXP "[^/]*"
#endif


// Copy a file:
OS_API bool osCopyFile(const osFilePath& sourcePath, const osFilePath& destinationPath, bool allowOverwrite);

// ----------------------------------------------------------------------------------
// Class Name:           OS_API osFile
// General Description:
//   Represents a file on a storage device (disk / other).
// Author:      AMD Developer Tools Team
// Creation Date:        15/5/2004
// ----------------------------------------------------------------------------------
class OS_API osFile : public osStream
{
public:
    // Describes the file open mode.
    enum osOpenMode
    {
        OS_OPEN_TO_READ,
        OS_OPEN_TO_WRITE,
        OS_OPEN_TO_APPEND,
    };

    osFile();
    osFile(const osFilePath& path);
    virtual ~osFile();

    bool open(const osFilePath& path, osChannelType fileType = OS_UNICODE_TEXT_CHANNEL, osOpenMode openMode = OS_OPEN_TO_READ);
    bool open(osChannelType fileType = OS_UNICODE_TEXT_CHANNEL, osOpenMode openMode = OS_OPEN_TO_READ);
    void close();
    bool deleteFile();
    void flush();
    bool isOK() const;

    bool setPath(const osFilePath& path);
    const osFilePath& path() const { return _filePath; };

    bool exists() const;
    bool getSize(unsigned long& fileSize) const;

    // Input stream methods:
    bool readLine(gtString& line);
    bool readLine(gtASCIIString& line);
    bool readIntoString(gtString& string);
    bool readIntoString(gtASCIIString& string);

    template <typename Ty>
    bool read(Ty& data) { return read((gtByte*)&data, sizeof(Ty)); }

    template <typename Ty>
    bool write(const Ty& data) { return write((const gtByte*)&data, sizeof(Ty)); }

    // Overrides osChannel:
    virtual osChannelType channelType() const;
    virtual bool write(const gtByte* pDataBuffer, gtSize_t dataSize);
    virtual bool read(gtByte*, gtSize_t);
    virtual bool writeString(const gtString& str);
    virtual bool readString(gtString&);
    virtual bool readAvailableData(gtByte* pDataBuffer, gtSize_t bufferSize, gtSize_t& amountOfDataRead);


    // Overrides osStream:
    virtual bool isOpened() const;
    virtual bool seekCurrentPosition(osStreamPosition seekStartPosition, gtSize_t offset);
    virtual bool currentPosition(osStreamPosition positionReference, gtSize_t& offset) const;

private:
    // The file implementation:
    osFileImpl* _pFileImpl;

    // My file (channel) type:
    osChannelType _fileType;

    // The file path:
    osFilePath _filePath;
};


#endif  // __OSFILE
