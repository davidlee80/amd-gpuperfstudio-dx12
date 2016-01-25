//==============================================================================
// Copyright (c) 2015 Advanced Micro Devices, Inc. All rights reserved.
/// \author AMD Developer Tools Team
/// \file
/// \brief  Version information
//==============================================================================

#ifndef __VERSIONINFO_H
#define __VERSIONINFO_H

// Values read by the Windows OS and displayed as DLL/EXE file properties:
// Stand alone version are higher then package versions
// Need also to update Q:\Workspaces\CodeXLVSPackage\CodeXLVSPackageVSIX\Properties\AssemblyInfo.cs

#define FILEVER                     0,0,0,0
#define PRODUCTVER                  0,0,0,0
#define VS_PACKAGE_FILEVER          0,0,0,0
#define VS_PACKAGE_PRODUCTVER       0,0,0,0
#define STRFILEVER                  L"0,0,0,0\0"
#define STRPRODUCTVER               L"0,0,0,0\0"
#define VS_PACKAGE_STRFILEVER       L"0,0,0,0\0"
#define VS_PACKAGE_STRPRODUCTVER    L"0,0,0,0\0"
#define STRCOMPANYNAME              L"AMD Inc.\0"
#define STRCOPYRIGHT                L"© 2011-2014 AMD Inc. All rights reserved.\0"
#define STRFILEDESCRIPTION          L"CodeXL - A comprehensive tool suite for the performance-aware developer\0"
#define STRPRODUCTNAME              L"CodeXL - A comprehensive tool suite for the performance-aware developer\0"
#define VS_PACKAGE_STRPRODUCTNAME   L"CodeXL - A comprehensive tool suite for the performance-aware developer\0"


// Revision for specific versions:
#define VI_VS_PACKAGE_VERSION_6_0_REVISION 389097
#define VI_VERSION_6_0_REVISION 12100
#define VI_VERSION_5_8_REVISION 11830
#define VI_VERSION_5_7_REVISION 11390
#define VI_VERSION_5_6_REVISION 11000
#define VI_VERSION_CL_BETA1_REVISION 10740
#define VI_VERSION_5_5_REVISION 10370
#define VI_VERSION_5_4_REVISION 10200
#define VI_VERSION_5_3_REVISION 9580
#define VI_VERSION_IPHONE_BETA5_REVISION 9465
#define VI_VERSION_5_2_1_REVISION 9245
#define VI_VERSION_5_2_REVISION 9170
#define VI_VERSION_IPHONE_BETA2_REVISION 9050
#define VI_VERSION_5_1_REVISION 8860
#define VI_VERSION_5_0_REVISION 8780
#define VI_VERSION_MAC_OFFICIAL_REVISION 8688
#define VI_VERSION_MAC_BETA3_REVISION 8642
#define VI_VERSION_MAC_BETA2_REVISION 8550
#define VI_VERSION_4_5_REVISION 8500
#define VI_VERSION_MAC_BETA1_REVISION 8420
#define VI_VERSION_4_4_REVISION 8010
#define VI_VERSION_4_3_REVISION 7600
#define VI_VERSION_4_2_REVISION 7110
#define VI_VERSION_4_1_REVISION 6770
#define VI_VERSION_4_0_REVISION 6525
#define VI_VERSION_3_1_1_REVISION 5110
#define VI_VERSION_3_1_REVISION 5050
#define VI_VERSION_3_0_REVISION 4105
#define VI_VERSION_2_5_REVISION 3785

// Free license length (in days):
#define VI_FREE_LICENSE_LENGTH_IN_DAYS 365

// License or Maintenance reminder period:
#define VI_LICENSE_OR_MAINTANANCE_REMINDER_DAYS 31

// Hard coded free version expiration date:
#define VI_HC_FREE_YEAR 2015
#define VI_HC_FREE_MONTH 12
#define VI_HC_FREE_DAY 31

// Hard coded license server expiration date:
#define VI_HC_LICENSE_SERVER_YEAR 2020
#define VI_HC_LICENSE_SERVER_MONTH 12
#define VI_HC_LICENSE_SERVER_DAY 31

// Hard coded free license generator cgi-bin expiration date:
#define VI_HC_FREE_LICENSE_GENERATOR_CGI_YEAR 2012
#define VI_HC_FREE_LICENSE_GENERATOR_CGI_MONTH 12
#define VI_HC_FREE_LICENSE_GENERATOR_CGI_DAY 31


#endif //__VERSIONINFO_H
