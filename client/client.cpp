/**
 * LAN Messenger
 * Copyright © 2011 Valerio Pilo
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "client.h"

#include "common.h"
#include "errors.h"

#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>



Client::Client()
{
}



Client::~Client()
{
  close( socket_ );
}



Errors::ErrorCode Client::initialize( const in_addr serverIp, const int serverPort )
{
  socket_ = socket( AF_INET, SOCK_STREAM, 0 );
  if( socket_ == -1 )
  {
    Common::error( "Socket initialization failure" );
    return Errors::Error_Socket_Init;
  }

  sockaddr_in serverAddress;
  serverAddress.sin_family = AF_INET;
  serverAddress.sin_addr = serverIp;
  serverAddress.sin_port = htons( serverPort );
  memset( &(serverAddress.sin_zero), '\0', 8 );

  Common::debug( "Connecting to %s:%d...", inet_ntoa( serverIp ), serverPort );

  int result;
  result = connect( socket_, reinterpret_cast<sockaddr*>( &serverAddress ), sizeof( sockaddr ) );
  if( result == -1 )
  {
    Common::error( "Connection failure" );
    return Errors::Error_Socket_Connection;
  }

  return Errors::Error_None;
}


