/**
 * LAN Messenger
 * Copyright © 2011 Valerio Pilo
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "sessionbase.h"

#include "common.h"
#include "message.h"

#include <netinet/in.h>
#include <sys/poll.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>



SessionBase::SessionBase( const int socket )
: disconnectionFlag_( false )
, socket_( socket )
{

}



SessionBase::~SessionBase()
{
  close( socket_ );
}



void SessionBase::disconnect()
{
  disconnectionFlag_ = true;
}



void* SessionBase::pollForData( void* thisPointer )
{
  // Get access to the owner instance
  SessionBase* self = static_cast<SessionBase*>( thisPointer );

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

  // Start the data transfer loop:
  // end when we have to disconnect and all pending messages have been sent
  while( ! self->disconnectionFlag_ || self->sendingQueue_.size() > 0 )
  {
/*
    Common::debug( "** pollForData(): %s; %d in send queue; %d in receive queue",
                   self->disconnectionFlag_ ? "Exiting" : "Running",
                   self->sendingQueue_.size(),
                   self->receivingQueue_.size() );
*/

    // If there is nothing to send, don't poll for the availability of a write operation
    if( self->sendingQueue_.size() > 0 )
    {
      watched.events = POLLIN | POLLOUT;
    }
    else
    {
      watched.events = POLLIN;
    }

    bool hasError = false;

    int ready = ppoll( &watched, 1, &timeout, &set );

    if( ready == 0 )
    {
      continue;
    }
    else if( ready == -1 )
    {
      Common::error( "Session 0x%X: error: Error %d: %s", self, errno, strerror( errno ) );
      hasError = true;
    }

    if( watched.revents & POLLERR )
    {
      Common::error( "Session 0x%X: error: Unspecified error condition", self );
      hasError = true;
    }
    if( watched.revents & POLLHUP )
    {
      Common::error( "Session 0x%X: error: Remote end hanged up", self );
      hasError = true;
    }
    if( watched.revents & POLLNVAL )
    {
      Common::error( "Session 0x%X: error: Socket is closed", self );
      hasError = true;
    }
    if( watched.revents & POLLIN )
    {
      hasError = self->readData();
    }
    if( watched.revents & POLLOUT )
    {
      hasError = self->writeData();
    }

    if( hasError )
    {
      self->disconnectionFlag_ = true;
    }
  }

  delete self;

  return NULL; // Unused value
}


bool SessionBase::readData()
{
  Common::debug( "Session 0x%X: Receiving message...", this );

  void* buffer = malloc( MAX_MESSAGE_SIZE );

  int readBytes = recv( socket_, buffer, MAX_MESSAGE_SIZE, 0 );

  if( readBytes == 0 )
  {
    free( buffer );
    Common::debug( "Session 0x%X: Socket was closed", this );
    return true;
  }
  else if( readBytes < 0 )
  {
    free( buffer );
    Common::error( "Session 0x%X: error: Socket is closed", this );
    return true;
  }

  Common::printData( (const char*)buffer, readBytes );

  Message* message = Message::parseHeader( buffer, readBytes );
  if( message != NULL && message->type() != Message::MSG_INVALID )
  {
    receivingQueue_.push_back( message );
    availableMessages();
  }
  else
  {
    delete message;
    // Disconnect the peer
    return true;
  }

  free( buffer );
  return false;
}



Message* SessionBase::receiveMessage()
{
  if( receivingQueue_.size() == 0 )
  {
    return NULL;
  }

  Message* message = receivingQueue_.back();
  receivingQueue_.pop_back();

  return message;
}



void SessionBase::sendMessage( Message* message )
{
  sendingQueue_.push_back( message );
}




bool SessionBase::writeData()
{
  Common::debug( "Session 0x%X: Sending queued data...", this );

  if( sendingQueue_.size() == 0 )
  {
    Common::debug( "Session 0x%X: Nothing to send", this );
    return false;
  }

  Message* message = sendingQueue_.front();
  sendingQueue_.pop_front();

  int size = -1;
  void* buffer = message->data( size );

  Common::printData( (const char*)buffer, size );
  send( socket_, buffer, size, 0 );

  free( buffer );
  delete message;

  return false;
}


