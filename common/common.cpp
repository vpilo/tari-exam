/**
 * LAN Messenger
 * Copyright © 2011 Valerio Pilo
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "common.h"

#include <cstdio>
#include <cstdarg>
#include <ctime>
#include <string.h>


FILE* Common::logFileHandle_ = 0;
char  Common::logFilePath_[] = "";
bool  Common::useLogFile_( false );



void Common::debug( const char* debugString, ... )
{
  char outputString[ MAX_STRING_LENGTH ];
  va_list args;

  // Get all the parameters that have been passed to this function
  va_start( args, debugString );

  // Apply the parameters to the debug string
  vsprintf( outputString, debugString, args );
  va_end( args );

  writeLine( "", outputString );
}



void Common::error( const char* errorString, ... )
{
  char outputString[ MAX_STRING_LENGTH ];
  va_list args;

  // Get all the parameters that have been passed to this function
  va_start( args, errorString );
  vsprintf( outputString, errorString, args );
  va_end( args );

  writeLine( "ERROR: ", outputString );
}



void Common::setLogFile( const char* logFilePath )
{
   // If a non null and non empty string was given, use the logfile (good for threads)
  useLogFile_ = ( logFilePath != 0 && *logFilePath != '\0' );

  strncpy( logFilePath_, logFilePath, MAX_STRING_LENGTH - 1 );
  logFilePath_[ MAX_STRING_LENGTH - 1 ] = '\0';  // Ensure the last byte is a null
}



void Common::writeLine( const char* prefix, const char* string )
{
  double elapsed = std::clock() / (double)CLOCKS_PER_SEC;
  char line[ MAX_STRING_LENGTH ];

  sprintf( line, "%6.4f> %s%s\n", elapsed, prefix, string );

  // Write it to standard output
  if( ! useLogFile_ )
  {
    printf( line );
    return;
  }

  // Write it to file

  // Open the file
  if( logFileHandle_ == 0 )
  {
    logFileHandle_ = fopen( logFilePath_, "w" );

    // The file won't be closed (no destructor in static class)
    // the OS will close it when we're done
  }

  fwrite( line, strlen( line ), 1, logFileHandle_ );
  fflush( logFileHandle_ );

}


