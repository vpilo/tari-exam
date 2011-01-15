/**
 * LAN Messenger
 * Copyright © 2011 Valerio Pilo
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "sessionclient.h"

#include "common.h"
#include "message.h"
#include "server.h"

#include <sys/poll.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>



SessionClient::SessionClient( Server* parent, const int socket )
: server_( parent )
, socket_( socket )
{

}



SessionClient::~SessionClient()
{
  close( socket_ );
  server_->removeSession( this );
}



void* SessionClient::pollForData( void* thisPointer )
{
  // Get access to the owner instance
  SessionClient* self = static_cast<SessionClient*>( thisPointer );

  // Poll for both read and write events
  pollfd watched;
  watched.fd = self->socket_;
  watched.events = POLLIN | POLLOUT;

  // Polling will end after this timeout is reached..
  timespec timeout;
  timeout.tv_sec = 1;
  timeout.tv_nsec = 0; // 1.0s

  // ..or a signal is caught
  sigset_t set;
  pthread_sigmask( SIG_SETMASK, &set, NULL );

  // Start the data transfer loop
  bool isClientActive = true;
  while( isClientActive )
  {
    // If there is nothing to send, don't poll for the availability of a write operation
    if( self->sendingQueue_.size() > 0 )
    {
      watched.events = POLLIN | POLLOUT;
    }
    else
    {
      watched.events = POLLIN;
    }

    int ready = ppoll( &watched, 1, &timeout, &set );

    if( ready == 0 )
    {
      continue;
    }
    else if( ready == -1 )
    {
      Common::error( "Client %x: error: Error %d: %s", self, errno, strerror( errno ) );
      isClientActive = false;
    }

    if( watched.revents & POLLERR )
    {
      Common::error( "Client %x: error: Unspecified error condition", self );
      isClientActive = false;
    }
    if( watched.revents & POLLHUP )
    {
      Common::error( "Client %x: error: Client hanged up", self );
      isClientActive = false;
    }
    if( watched.revents & POLLNVAL )
    {
      Common::error( "Client %x: error: Socket is closed", self );
      isClientActive = false;
    }
    if( watched.revents & POLLIN )
    {
      isClientActive = self->readData();
    }
    if( watched.revents & POLLOUT )
    {
      isClientActive = self->writeData();
    }
  }

  delete self;
}


bool SessionClient::readData()
{
  void* buffer = malloc( MAX_MESSAGE_SIZE );

  int readBytes = recv( socket_, buffer, MAX_MESSAGE_SIZE, 0 );

  if( readBytes == 0 )
  {
    free( buffer );
    Common::debug( "Client %x: Socket was closed", this );
    return false;
  }
  else if( readBytes < 0 )
  {
    free( buffer );
    Common::error( "Client %x: error: Socket is closed", this );
    return false;
  }
  // TODO Analyze the buffer for data
  char tmp[ MAX_MESSAGE_SIZE + 1 ];
  strncpy( tmp, (char*)buffer, MAX_MESSAGE_SIZE );
  tmp[ readBytes ] = '\0';
  Common::debug( "Data dump (%d bytes):\n***************\n%s\n***************", readBytes, tmp );

  Common::debug( "Creating Message" );

  Message* message = Message::parseData( buffer, readBytes );

  if( message != NULL && message->type() != Message::MSG_INVALID )
  {
    receivingQueue_.push_back( message );
  }
  else
  {
    delete message;
    // Disconnect the client
    return false;
  }

  free( buffer );
  return true;
}



bool SessionClient::writeData()
{
  return true;
}



//  recv(), send(), etc


