//==============================================================================
// Copyright (c) 2013-2015 Advanced Micro Devices, Inc. All rights reserved.
/// \author AMD Developer Tools Team
/// \file
/// \brief  TODO add documentation
//==============================================================================

#ifndef GPS_DEFINES_H
#define GPS_DEFINES_H

// Define a helper TODO macro to track todo messages through #pragma message
// Usage: #pragma TODO("Fix Me")
#define hidden_quote( s ) #s
#define hidden_numquote( n ) hidden_quote( n )
#define TODO( msg ) message( __FILE__ "(" hidden_numquote( __LINE__ ) "): TODO: " msg )

// Replace a common windows value
#define PS_MAX_PATH 260

#endif // GPS_DEFINES_H
