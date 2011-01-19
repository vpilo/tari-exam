/**
 * LAN Messenger
 * Copyright © 2011 Valerio Pilo
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <sys/time.h>


/**
 * @def SERVER_PORT
 *
 * Port where the server listens to.
 */
#define SERVER_PORT    12345


/**
 * @def MAX_STRING_LENGTH
 *
 * Maximum size of debug strings.
 */
#define MAX_STRING_LENGTH    512



class Common
{

  public:

    /**
     * Prints debug output to a separate file. Just for debugging a bit, no concurrency checking.
     *
     * Acts like printf(), by taking all the parameters you want.
     *
     * @param debugString The string to display. See printf for details
     * @see std::printf()
     */
    static void debug( const char* debugString, ... );

    /**
     * Prints out an error message to the 'standard error' output.
     *
     * Acts like printf(), by taking all the parameters you want.
     *
     * @param errorString The string to display. See printf for details
     * @see std::printf()
     */
    static void error( const char* errorString, ... );

    /**
     * Prints out an error message to the 'standard error' output, and abort.
     *
     * @param errorString The string to display. See printf for details
     * @see std::printf()
     * @see error()
     */
    static void fatal( const char* errorString, ... );

    /**
     * Prints binary data.
     *
     * @param buffer The data to display
     * @param size The size of the buffer
     */
    static void printData( const char* buffer, int bufferSize, bool isIncoming = true, const char* label = 0 );

    static void setLogFile( const char* logFilePath );


private:
    static void writeLine( const char* prefix, const char* string );
    static void writeRawData( const char* data );

private:

  static FILE *logFileHandle_;
  static char logFilePath_[ MAX_STRING_LENGTH ];
  static bool useLogFile_;
  static timeval startTime_;

};

#endif // COMMON_H
