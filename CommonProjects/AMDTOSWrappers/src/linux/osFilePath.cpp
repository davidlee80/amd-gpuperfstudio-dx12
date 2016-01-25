//=====================================================================
// Copyright 2016 (c), Advanced Micro Devices, Inc. All rights reserved.
//
/// \author AMD Developer Tools Team
/// \file osFilePath.cpp 
/// 
//=====================================================================

//------------------------------ osFilePath.cpp ------------------------------

// Standard C:
#include <stdlib.h>
#include <string.h>

// POSIX:
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pwd.h>

// Infra:
#include <AMDTBaseTools/Include/gtAssert.h>
#include <AMDTBaseTools/Include/gtDefinitions.h>

// Local:
#include <AMDTOSWrappers/Include/osStringConstants.h>
#include <AMDTOSWrappers/Include/osOSDefinitions.h>
#include <AMDTOSWrappers/Include/osDirectory.h>
#include <AMDTOSWrappers/Include/osFilePath.h>
#include <AMDTOSWrappers/Include/osGeneralFunctions.h>

// Static members initializations:
const wchar_t osFilePath::osPathSeparator = '/';
const wchar_t osFilePath::osExtensionSeparator = '.';
const wchar_t osFilePath::osEnvironmentVariablePathsSeparator = ':';

// Constants:
#define OS_PATH_STRING_BUFFER_SIZE FILENAME_MAX
#define OS_LINUX_TEMP_DIR_PATH L"/.amd/AMD/CodeXL/"
#define OS_LINUX_COMMON_APPLICATION_DATA_DIR L"/etc"
#define OS_LINUX32_SYSTEM_DIR_PATH L"/usr/lib"
#define OS_LINUX64_SYSTEM_DIR_PATH L"/usr/lib64"
#define OS_LINUX32_UBUNTU_SYSTEM_DIR_PATH L"/usr/lib32"
#define OS_LINUX64_UBUNTU_SYSTEM_DIR_PATH L"/usr/lib"
#define OS_CURR_USER_ENV_VARIABLE L"USER"
#define OS_HOME_DIR_ENV_VARIABLE "HOME"
#define OS_ROOT_USER_HOME_DIR L"/root"
#define OS_AMD_DOT_SUBDIR_NAME L".amd"
#define OS_LINUX_DOWNLOAD_DIR L"Downloads"


// ---------------------------------------------------------------------------
// Name:        osFilePath::operator==
// Author:      AMD Developer Tools Team
// Date:        7/1/2006
// ---------------------------------------------------------------------------
bool osFilePath::operator==(const osFilePath& other) const
{
    bool retVal = false;

    if ((_fileDirectory == other._fileDirectory) &&
        (_fileName == other._fileName) &&
        (_fileExtension == other._fileExtension))
    {
        retVal = true;
    }

    return retVal;
}

// ---------------------------------------------------------------------------
// Name:        osFilePath::setPath
// Description: Sets the file path from a pre-defined file path
// Arguments:   predefinedfilePath - The pre-defined file path
//              (See osFilePath::osPreDefinedFilePaths)
// Return Val:  bool - Success / failure.
// Author:      AMD Developer Tools Team
// Date:        30/5/2004
// ---------------------------------------------------------------------------
bool osFilePath::setPath(osPreDefinedFilePaths predefinedfilePath, bool applyRedirection)
{
    bool retVal = false;
    bool copybuffer = true;

    wchar_t stringBuffer[OS_PATH_STRING_BUFFER_SIZE] = {0};

    // Get the directory path:
    switch (predefinedfilePath)
    {
        case OS_SYSTEM_DIRECTORY:
        case OS_SYSTEM_X86_DIRECTORY:
        {
            // On Linux we have few "system" directories (/bin, /usr/bin, /usr/sbin,
            // usr/lib, etc). We will output the "most used" linux system directory:
            // - /usr/lib - For 32 bit Linux.
            // - /usr/lib64 - For 64 bit Linux.
            gtString linuxVariant;
            osGetLinuxVariantName(linuxVariant);
#if GR_ADDRESS_SPACE_TYPE == GR_32_BIT_ADDRESS_SPACE

            if (linuxVariant == OS_STR_linuxVariantUbuntu)
            {
                wcscpy(stringBuffer, OS_LINUX32_UBUNTU_SYSTEM_DIR_PATH);
            }
            else
            {
                wcscpy(stringBuffer, OS_LINUX32_SYSTEM_DIR_PATH);
            }

#elif GR_ADDRESS_SPACE_TYPE == GR_64_BIT_ADDRESS_SPACE

            if (linuxVariant == OS_STR_linuxVariantUbuntu)
            {
                wcscpy(stringBuffer, OS_LINUX64_UBUNTU_SYSTEM_DIR_PATH);
            }
            else
            {
                wcscpy(stringBuffer, OS_LINUX64_SYSTEM_DIR_PATH);
            }

#else
#error Unknown address space type!
#endif

            retVal = true;
        }
        break;

        case OS_TEMP_DIRECTORY:
        {
            // Output the Linux temp directory:
            // Get the current user home directory from the system
            struct passwd *pw = getpwuid(getuid());
            GT_IF_WITH_ASSERT(pw != nullptr)
            {
                const char *homedir = pw->pw_dir;

                // convert it:
                gtString realPathStr;
                realPathStr.fromASCIIString(homedir);

                // append the place we want the temp dir:
                realPathStr.append(OS_LINUX_TEMP_DIR_PATH);
                
                // Copy to the output string
                wcscpy(stringBuffer, realPathStr.asCharArray());

                retVal = true;
            }
        }
        break;

        case OS_USER_APPLICATION_DATA:
        {
            InitializeUnicodeCharactersUserFilePath(applyRedirection);
            wcscpy(stringBuffer, ms_userAppDataFilePath.asString().asCharArray());
            retVal = true;
        }
        break;

        case OS_ROOT_USER_APPLICATION_DATA:
        {
            // We define the root user application data directory to be /root/.amd

            // Build /root/.gamd:
            osFilePath amdDirPath;
            amdDirPath.setFileDirectory(OS_ROOT_USER_HOME_DIR);
            amdDirPath.appendSubDirectory(OS_AMD_DOT_SUBDIR_NAME);

            wcscpy(stringBuffer, amdDirPath.asString().asCharArray());

            // If it does not exist - create it:
            osDirectory amdDir(amdDirPath);
            bool amdDirExists = amdDir.exists();

            if (!amdDirExists)
            {
                amdDirExists = amdDir.create();
            }

            // Verify that the directory exists:
            GT_IF_WITH_ASSERT(amdDirExists)
            {
                retVal = true;
            }
        }
        break;

        case OS_BROWSE_EXECUTABLES_DIRECTORY:   // In Linux, we start browsing in the user's home dir.
        case OS_USER_DOCUMENTS:
        {
            // Get the current user's home directory:
            const char* pCurrUserHomeDirectory = getenv(OS_HOME_DIR_ENV_VARIABLE);
            GT_IF_WITH_ASSERT(pCurrUserHomeDirectory != NULL)
            {
                gtString currUserHomeDirectoryStr;
                currUserHomeDirectoryStr.fromUtf8String(pCurrUserHomeDirectory);

                // Yaki 7/7/2008:
                // When running as a Linux service at boot time, we get "/" as the current user's home directory.
                // We fix that to be "/root".
                if (strcmp(pCurrUserHomeDirectory, "/") == 0)
                {
                    static wchar_t stat_rootHomeDir[] = OS_ROOT_USER_HOME_DIR;
                    // Copy it to the path string buffer:
                    wcscpy(stringBuffer, stat_rootHomeDir);
                }
                else
                {
                    // Copy it to the path string buffer:
                    wcscpy(stringBuffer, currUserHomeDirectoryStr.asCharArray());
                }

                retVal = true;
            }
        }
        break;
        case OS_USER_DOWNLOADS:
        {
            const char* pCurrUserHomeDirectory = getenv(OS_HOME_DIR_ENV_VARIABLE);
            GT_IF_WITH_ASSERT(pCurrUserHomeDirectory != nullptr)
            {
                gtString currUserHomeDirectoryStr;
                currUserHomeDirectoryStr.fromUtf8String(pCurrUserHomeDirectory);
                setFileDirectory(currUserHomeDirectoryStr);
                appendSubDirectory(OS_LINUX_DOWNLOAD_DIR);
                retVal = true;
                copybuffer = false;
            }
        }
        break;
        case OS_COMMON_APPLICATION_DATA:
        {
            // Output the Linux common application data directory:
            wcscpy(stringBuffer, OS_LINUX_COMMON_APPLICATION_DATA_DIR);
            retVal = true;
        }
        break;

        case OS_CURRENT_DIRECTORY:
        {
            // Get current directory:
            std::string utf8Path;
            utf8Path.resize(OS_PATH_STRING_BUFFER_SIZE);
            char* rc = getcwd(&(utf8Path[0]), OS_PATH_STRING_BUFFER_SIZE);
            GT_IF_WITH_ASSERT(rc != NULL)
            {
                // Convert from UTF8 to unicode
                wstring widePath;
                int rc2 = gtUtf8StringToWideString(utf8Path, widePath);
                wcsncpy(stringBuffer, widePath.c_str(), OS_PATH_STRING_BUFFER_SIZE);
                retVal = (rc2 == 0);
            }
        }
        break;

        default:
        {
            // Unknown pre-defined directory:
            GT_ASSERT(false);
        }
        break;
    }

    if (retVal && copybuffer)
    {
        _fileDirectory = stringBuffer;
    }

    return retVal;
}

bool osFilePath::GetUserAppDataFilePath(osFilePath& userAppDataFilePath, bool applyRedirection)
{
    GT_UNREFERENCED_PARAMETER(applyRedirection);

    bool retVal = false;

    wchar_t stringBuffer[OS_PATH_STRING_BUFFER_SIZE];

    // Get the current user's documents directory:
    userAppDataFilePath = osFilePath(osFilePath::OS_USER_DOCUMENTS);

    // Sanity check:
    GT_IF_WITH_ASSERT(!(userAppDataFilePath._fileDirectory.isEmpty()))
    {
        // We define the application data directory to be <users documents dir>/.amd
        userAppDataFilePath.appendSubDirectory(OS_AMD_DOT_SUBDIR_NAME);
        wcscpy(stringBuffer, userAppDataFilePath.asString().asCharArray());

        // If it does not exist - create it:
        osDirectory amdDir(userAppDataFilePath);
        bool amdDirExists = amdDir.exists();

        if (!amdDirExists)
        {
            amdDirExists = amdDir.create();
        }

        // Verify that the directory exists:
        GT_ASSERT(amdDirExists);

        // Set the file directory:
        userAppDataFilePath.setFileDirectory(stringBuffer);

        retVal = true;
    }

    return retVal;
}


// ---------------------------------------------------------------------------
// Name:        osFilePath::adjustToCurrentOS
// Description: Adjust the path string to the current operating system.
// Author:      AMD Developer Tools Team
// Date:        1/11/2006
// Usage Sample:
// Implementation notes:
//  We have to adjust the file path to Linux conventions:
//  a. Replace "\" by "/".
//  b. Remove trailing "/" ("/foo/" -> "/foo").
// ---------------------------------------------------------------------------
void osFilePath::adjustStringToCurrentOS(gtString& filePathString)
{
    // a. Replace "\" by "/"
    int len = filePathString.length();

    for (int i = 0; i < len; i++)
    {
        if (filePathString[i] == '\\')
        {
            filePathString[i] = osPathSeparator;
        }
    }

    // b. Remove trailing "\":
    bool pathEndedInSeparator = false;
    int filePathLength = filePathString.length();

    if (filePathLength > 0)
    {
        pathEndedInSeparator = (filePathString[filePathLength - 1] == osPathSeparator);
    }

    filePathString.removeTrailing(osPathSeparator);

    if (pathEndedInSeparator)
    {
        filePathString.append(osPathSeparator);
    }
}

// ---------------------------------------------------------------------------
// Name:        osFilePath::makeWritable
// Description: Make the file write only
// Return Val:  bool - Success / failure.
// Author:      AMD Developer Tools Team
// Date:        16/8/2011
// ---------------------------------------------------------------------------
bool osFilePath::makeWritable()
{
    bool retVal = false;

    // Convert the file path to UTF8 to support unicode characters
    std::string utf8Path;
    asString().asUtf8(utf8Path);

    int rc = chmod(utf8Path.c_str(), S_IWUSR);
    retVal = (rc == 0);
    return retVal;
}

// ---------------------------------------------------------------------------
// Name:        osFilePath::osWStat
// Description: implement Linux version os wstat
// Author:      AMD Developer Tools Team
// Date:        19/6/2011
// ---------------------------------------------------------------------------
int osWStat(const gtString& fileName, osStatStructure& fileStruct)
{
    // Convert the file path to UTF8 to support unicode characters
    std::string utf8Path;
    fileName.asUtf8(utf8Path);

    gtASCIIString fileNameASCIIStr = utf8Path.c_str();
    fileNameASCIIStr.removeTrailing(osFilePath::osPathSeparator);
    int retVal = stat(fileNameASCIIStr.asCharArray(), &fileStruct);

    return retVal;
}

