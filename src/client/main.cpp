/**
 * LAN Messenger
 * Copyright (C) 2011 Valerio Pilo
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "client.h"
#include "common.h"
#include "errors.h"

#include <netdb.h>
#include <string.h>
#include <unistd.h>


#define DEFAULT_SERVER_IP  "127.0.0.1"



void usage( const char* programName )
{
  fprintf( stderr, "Usage: %s [address]\n",programName );
  fprintf( stderr, "If the [address] argument is omitted, the client will try to connect to %s.\n", DEFAULT_SERVER_IP );
}



/**
 * Client application entry point.
 */
int main( int argc, char* argv[] )
{
  char logFile[ MAX_PATH_SIZE ];
  sprintf( logFile, "lanmessenger-client.%d.log", getpid() );
  Common::setLogFile( logFile );
  Common::debug( "LAN Messenger client" );

  // Check command-line arguments:

  in_addr serverIp;
  serverIp.s_addr = htonl( INADDR_LOOPBACK );

  const char* serverIpString = argv[ 1 ];
  if( argc == 2 )
  {
    if( strcmp( serverIpString, "-h" ) == 0 )
    {
      usage( argv[ 0 ] );
      return 1;
    }

    hostent* host = gethostbyname( serverIpString );

    if( ! host || h_errno != NETDB_SUCCESS )
    {
      fprintf( stderr, "Unable to connect to \"%s\": %s\n", serverIpString, hstrerror( h_errno ) );
      return 2;
    }
    serverIp = *( reinterpret_cast<in_addr*>( host->h_addr ) );
  }
  else
  {
    serverIpString = DEFAULT_SERVER_IP;
  }

  Client* client = new Client();

  Errors::ErrorCode status = client->initialize( serverIp, SERVER_PORT );
  if( status != Errors::Error_None )
  {
    fprintf( stderr, "Unable to connect to the server at %s: error %d\n", serverIpString, status );

    delete client;
    return status;
  }

  client->run();

  delete client;

  Common::debug( "Goodbye!" );

  return Errors::Error_None;
}


