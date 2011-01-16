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
#include "sessionserver.h"

#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <semaphore.h>
#include <string.h>
#include <unistd.h>


// Semaphore used to end the session
sem_t sessionEndSignal;



Client::Client()
: connection_( NULL )
, connectionThread_( 0 )
{
  sem_init( &sessionEndSignal, 0, 0 );
}



Client::~Client()
{
  if( connectionThread_ != 0 )
  {
    pthread_cancel( connectionThread_ );
  }

  if( connection_ )
  {
    delete connection_;
  }

  close( socket_ );
}



void Client::connectionClosed( SessionServer* connection )
{
  if( connection != connection_ )
  {
    Common::fatal( "Unknown connection was closed!" );
  }

  connection_ = 0;

  Common::debug( "Connection closed" );

  sem_post( &sessionEndSignal );
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

  connection_ = new SessionServer( this, socket_ );

  pthread_create( &connectionThread_, NULL, &SessionServer::pollForData, connection_ );

  Common::debug( "Connection established" );

  return Errors::Error_None;
}



void Client::run()
{
  // Ask the nickname to the client
  // TODO Actually ask it
  connection_->setNickName( "Valerio" );

  sleep( 1 );

  // If we've been disconnected, the pointer will be null
  if( connection_ )
  {
    connection_->disconnect();
  }

  // Wait for the connection to be closed, then return
  sem_wait( &sessionEndSignal );
}


