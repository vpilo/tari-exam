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

#include "sessionclient.h"
#include "common.h"
#include "errors.h"

#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <unistd.h>

#include <algorithm>



/**
 * @def MAX_MESSAGE_QUEUE
 *
 * The OS will keep this amount of messages queued in case the server can't immediately process them.
 */
#define MAX_MESSAGE_QUEUE   40



Server::Server()
: listenThread_( 0 )
{
  int result = pthread_mutex_init( &accessMutex_, NULL );
  if( result != 0 )
  {
    Common::fatal( "Server mutex creation failed: error %d", result );
  }
}



Server::~Server()
{
  while( sessions_.size() > 0 )
  {
    std::map<SessionClient*,SessionData*>::iterator it;
    for( it = sessions_.begin(); it != sessions_.end(); it++ )
    {
      SessionData* session = (*it).second;
      // Tell the session to disconnect; it will auto-delete itself and end its thread
      session->client->disconnect();
      pthread_join( session->thread, NULL );
    }
  }

  pthread_cancel( listenThread_ );
  pthread_mutex_destroy( &accessMutex_ );
}



void Server::addSession( int newSocket )
{
  // The client session will take care of the socket and free it up when done.
  // It will also self-destruct when not needed anymore.

  SessionData* newSession = new SessionData;
  newSession->client = new SessionClient( this, newSocket );
  newSession->state = CLIENT_STATE_START;

  pthread_create( &newSession->thread, NULL, &SessionClient::pollForData, newSession->client );

  pthread_mutex_lock( &accessMutex_ );
  sessions_[ newSession->client ] = newSession;
  pthread_mutex_unlock( &accessMutex_ );

  Common::debug( "Session 0x%X registered, %lu active", newSession->client, sessions_.size() );
}



void Server::checkSessionStateChange( SessionClient* client, Message::Type messageType )
{
  SessionData* current = findSession( client );

  if( ! current )
  {
    Common::fatal( "Received a session state change from unknown session 0x%X!", client );
  }

  /*
   * How does this work:
   * Each message which changes the client state can be received when
   * the client is in a certain state; for example, clients can't send HELLO when
   * they have already authenticated.
   * The list contains all the states in which a client can be if it wants to
   * send a certain message.
   */
  std::list<ClientState> expectedStates;
  ClientState nextState;

  switch( messageType )
  {
    case Message::MSG_HELLO:
      expectedStates.push_back( CLIENT_STATE_START );
      nextState = CLIENT_STATE_IDENTIFY;
      break;
    case Message::MSG_NICKNAME:
      expectedStates.push_back( CLIENT_STATE_IDENTIFY );
      nextState = CLIENT_STATE_READY;
      break;
    case Message::MSG_BYE:
      expectedStates.push_back( CLIENT_STATE_START );
      expectedStates.push_back( CLIENT_STATE_IDENTIFY );
      expectedStates.push_back( CLIENT_STATE_READY );
      nextState = CLIENT_STATE_INVALID;
      break;

    default:
      Common::error( "Session 0x%X sent an invalid state change message of type %d", client, messageType );
      client->disconnect();
      return;
  }

// States: CLIENT_STATE_INVALID CLIENT_STATE_START CLIENT_STATE_IDENTIFY CLIENT_STATE_READY CLIENT_STATE_END

  if( std::find( expectedStates.begin(), expectedStates.end(), current->state ) == expectedStates.end() )
  {
    Common::error( "Session 0x%X sent a wrong state message of type %d", client, messageType );
    client->disconnect();
    return;
  }

  current->state = nextState;
  Common::debug( "Session 0x%X changed state to %d", client, nextState );
}



Server::SessionData* Server::findSession( SessionClient* client )
{
  std::map<SessionClient*,SessionData*>::iterator it = sessions_.find( client );

  if( it == sessions_.end() )
  {
    return NULL;
  }

  return (*it).second;
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
  pthread_mutex_lock( &accessMutex_ );

  SessionData* current = findSession( client );

  if( ! current )
  {
    Common::fatal( "Received a session state change from unknown session 0x%X!", client );
  }

  sessions_.erase( client );
  delete current;

  pthread_mutex_unlock( &accessMutex_ );

  Common::debug( "Session 0x%X ended, %lu remaining", client, sessions_.size() );

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

  return NULL; // Unused value
}

