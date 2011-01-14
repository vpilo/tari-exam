/**
 * LAN Messenger
 * Copyright © 2011 Valerio Pilo
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "server.h"

#include "clientsession.h"
#include "common.h"
#include "errors.h"

#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <unistd.h>


/**
 * @def MAX_MESSAGE_QUEUE
 *
 * The OS will keep this amount of messages queued in case the server can't immediately process them.
 */
#define MAX_MESSAGE_QUEUE   40



Server::Server()
: listenThread_( 0 )
{
  int result = pthread_mutex_init( accessMutex_, NULL );
  if( result != 0 )
  {
    Common::fatal( "Server mutex creation failed: error %d", result );
  }
}



Server::~Server()
{
  pthread_cancel( listenThread_ );
  pthread_mutex_destroy( accessMutex_ );
}



void Server::addSession( int newSocket )
{
  // The client session will take care of the socket and free it up when done.
  // It will also self-destruct when not needed anymore.

  SessionData* newSession = new SessionData;
  newSession->client = new SessionClient( this, newSocket );

  pthread_create( &newSession->thread, NULL, &SessionClient::pollForData, newSession->client );

  pthread_mutex_lock( accessMutex_ );
  sessions_.push_back( newSession );
  pthread_mutex_unlock( accessMutex_ );

  Common::debug( "Session %x registered, %ul active", newSession->client, sessions_.size() );
}



Errors::ErrorCode Server::initialize( const char* address, const int port )
{
  listenSocket_ = socket( AF_INET, SOCK_STREAM, 0 );
  if( listenSocket_ == -1 )
  {
    return Errors::Error_Socket_Init;
  }

  int yes = 1;
  int result;
  result = setsockopt( listenSocket_, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof( int ) );
  if( result == -1 )
  {
    return Errors::Error_Socket_Option;
  }

  // Convert the IP string to an usable binary address
  in_addr listenAddr;
  if( inet_aton( address, &listenAddr ) == 0 )
  {
    return Errors::Error_Invalid_Address;
  }
  in_addr_t listenIP = inet_addr( address );

  // Set up the address where to listen
  sockaddr_in myAddress;
  myAddress.sin_family = AF_INET;
  myAddress.sin_port = htons( port );
  myAddress.sin_addr.s_addr = listenAddr.s_addr;
  memset( &( myAddress.sin_zero ), '\0', 8 );
  Common::debug( "Will listen on %s:%d", address, port );

  // Listen on the specified address and port
  result = bind( listenSocket_, reinterpret_cast<sockaddr*>( &myAddress ), sizeof( sockaddr ) );
  if( result == -1 )
  {
    return Errors::Error_Socket_Bind;
  }
  result = listen( listenSocket_, MAX_MESSAGE_QUEUE );
  if( result == -1 )
  {
    return Errors::Error_Socket_Listen;
  }

  // Start accepting connections
  pthread_create( &listenThread_, NULL, &Server::waitConnections, this );

  return Errors::Error_None;
}



void Server::removeSession( SessionClient* client )
{
  bool found = false;
  std::list<SessionData*>::iterator it;
  SessionData* current;

  pthread_mutex_lock( accessMutex_ );

  for( it = sessions_.begin(); it != sessions_.end(); it++ )
  {
    current = (*it);
    if( current->client == client )
    {
      found = true;
      break;
    }
  }

  if( found )
  {
    sessions_.remove( current );
    delete current;
  }
  else
  {
    Common::error( "Session %x was not found!" );
  }

  pthread_mutex_unlock( accessMutex_ );

  Common::debug( "Session %x ended, %ul remaining", client, sessions_.size() );

  // We won't delete the SessionClient, it does so by itself
}



void* Server::waitConnections( void* thisPointer )
{
  // Get access to the calling instance
  Server* self = static_cast<Server*>( thisPointer );

  sockaddr_in remote;
  socklen_t addressSize = sizeof( sockaddr_in );
  int newConnection;

  Common::debug( "Server is now accepting connections" );

  while( true )
  {
    newConnection = accept( self->listenSocket_,
                            reinterpret_cast<sockaddr*>( &remote ),
                            &addressSize );
    char* remoteAddress = inet_ntoa( remote.sin_addr );
    if( newConnection == -1 )
    {
      Common::error( "Unable to connect to %s:%d", remoteAddress, remote.sin_port );
      continue;
    }

    Common::debug( "Incoming connection from %s:%d", remoteAddress, remote.sin_port );

    self->addSession( newConnection );
  }
}

