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
#include <ctype.h>
#include <string.h>
#include <stdlib.h>


FILE* Common::logFileHandle_ = 0;
char  Common::logFilePath_[] = "";
bool  Common::useLogFile_( false );
timeval Common::startTime_ = { 0L, 0L };



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



void Common::fatal( const char* errorString, ... )
{
  char outputString[ MAX_STRING_LENGTH ];
  va_list args;

  // Get all the parameters that have been passed to this function
  va_start( args, errorString );
  vsprintf( outputString, errorString, args );
  va_end( args );

  writeLine( "ERROR: ", outputString );

  abort();
}



void Common::printData( const char* buffer, int bufferSize, bool isIncoming, const char* label )
{
  #define BYTES_PER_LINE  16

  char text[ MAX_STRING_LENGTH ];
  int pos = 0;
  int offset = 0;

  debug( "%s: %s (%d bytes)",
         ( isIncoming ? " In" : "Out" ),
         ( label ? label : "Raw data dump" ),
         bufferSize );
  writeRawData( "******************************************************************************\n" );

  while( offset < bufferSize )
  {
    // Print the offset
    sprintf( text, "%05d : ", offset );
    writeRawData( text );

    // Then the hexadecimal data
    for( pos = 0; pos < BYTES_PER_LINE; pos++ )
    {
      if( ( pos + offset ) < bufferSize )
      {
        sprintf( text, "%02x ", (unsigned char) buffer[ pos + offset ] );
        writeRawData( text );
      }
      else
      {
        writeRawData( "   " );
      }
    }

    writeRawData( ": " );

    // Finally the ASCII data
    for( pos = 0; pos < BYTES_PER_LINE; pos++ )
    {
      char current = ' ';

      if( ( pos + offset ) < bufferSize )
      {
        current = buffer[ pos + offset ];
      }

      if( ( current & 0x80 ) == 0 && isprint( (int)current ) )
      {
        // Treat % differently, to avoid it from being parsed by sprintf()
        if( current == '%' )
        {
          writeRawData( "%%" );
        }
        else
        {
          sprintf( text, "%c", current );
          writeRawData( text );
        }
      }
      else
      {
        writeRawData( "." );
      }
    }

    offset += BYTES_PER_LINE;

    writeRawData( "\n" );
  }

  writeRawData( "******************************************************************************\n" );
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
  // Calculate the time since program start
  if( ! startTime_.tv_sec &&  ! startTime_.tv_usec )
  {
    gettimeofday( &startTime_, NULL );
  }
  timeval end;
  gettimeofday( &end, NULL );

  double elapsed =   ( end.tv_sec  - startTime_.tv_sec  )
                   + ( end.tv_usec - startTime_.tv_usec ) / 1000000.f;

  // Compose the debug line, adding the elapsed time (very useful when multithreading)
  char line[ MAX_STRING_LENGTH ];
  sprintf( line, "%7.3f> %s%s\n", elapsed, prefix, string );

  writeRawData( line );
}


void Common::writeRawData( const char* data )
{
  // Write it to standard output..
  if( ! useLogFile_ )
  {
    printf( data );
    return;
  }

  // ..or to file

  // Open the file
  if( logFileHandle_ == 0 )
  {
    logFileHandle_ = fopen( logFilePath_, "w" );

    // The file won't be closed (no destructor in static class)
    // the OS will close it when we're done
  }

  fwrite( data, strlen( data ), 1, logFileHandle_ );
  fflush( logFileHandle_ );
}


