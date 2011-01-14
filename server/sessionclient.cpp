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
#include "server.h"

#include <sys/poll.h>
#include <pthread.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>



SessionClient::SessionClient( Server* parent, const int socket )
: server_( parent )
, socket_( socket )
{

}



SessionClient::~SessionClient()
{
  server_->removeSession( this );
  close( socket_ );
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
      self->readData();
    }
    if( watched.revents & POLLOUT )
    {
      self->writeData();
    }
  }

}

//  recv(), send(), etc


