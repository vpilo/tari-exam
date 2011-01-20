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
#include "protocol.h"

#include "hellomessage.h"
#include "byemessage.h"
#include "nicknamemessage.h"
#include "statusmessage.h"
#include "chatmessage.h"

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
: bufferOffset_( 0 )
, disconnectionFlag_( false )
, socket_( socket )
{
 buffer_ = static_cast<char*>( malloc( MAX_MESSAGE_SIZE ) );
}



SessionBase::~SessionBase()
{
  close( socket_ );

  free( buffer_ );
}



void SessionBase::disconnect()
{
  disconnectionFlag_ = true;
}



Message* SessionBase::parseMessage()
{
  int messageHeaderSize = sizeof( MessageHeader );

  MessageHeader messageData;
  memcpy( &messageData, buffer_, messageHeaderSize );

  // Identify the command
  Message::Type type = Message::MSG_INVALID;
  for( int i = Message::MSG_INVALID + 1; i < Message::MSG_MAX; i++ )
  {
    Message::Type current = static_cast<Message::Type>( i );
    if( strcmp( messageData.command, Message::command( current ) ) == 0 )
    {
      type = current;
      break;
    }
  }

  // Validate the header fields
  if( type == Message::MSG_INVALID )
  {
    Common::error( "Received invalid command \"%s\"!", messageData.command );
    return NULL;
  }
  if( ( messageHeaderSize + messageData.size ) > MAX_MESSAGE_SIZE )
  {
    Common::error( "Received invalid message data size \"%d\"!", messageData.size );
    return NULL;
  }

  // Make the message and pass to it only the message-specific data
  char* dataBuffer = buffer_ + messageHeaderSize;

  Message* message = NULL;

  switch( type )
  {
    // Simple messages don't need a specialized class
    case Message::MSG_HELLO:
      message = new HelloMessage();
      break;
    case Message::MSG_BYE:
      message = new ByeMessage();
      break;
    case Message::MSG_NICKNAME:
      message = new NicknameMessage();
      break;
    case Message::MSG_STATUS:
      message = new StatusMessage();
      break;
    case Message::MSG_CHAT:
      message = new ChatMessage();
      break;
    default:
      Common::error( "Could not create the message. Invalid type %d", type );
      break;
  }

  if( message == NULL )
  {
    return NULL;
  }

  bool isOk = message->fromRawBytes( dataBuffer, messageData.size );
  if( ! isOk )
  {
    delete message;
    return NULL;
  }

  // Message is OK, move the rest of the buffer data at the start of the buffer
  // so the next one can be read

  bufferOffset_ += messageData.size;
  int remainder = MAX_MESSAGE_SIZE - bufferOffset_;
  memcpy( buffer_, buffer_ + bufferOffset_, remainder );
  bufferOffset_ = 0;

  // and zero out the remainder
  memset( buffer_ + MAX_MESSAGE_SIZE - remainder, '\0', remainder );

  return message;
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

  bool hasError = false;

  // Start the data transfer loop:
  // end when we have to disconnect and all pending messages have been sent
  while( ! hasError && ( ! self->disconnectionFlag_ || self->sendingQueue_.size() > 0 ) )
  {
/*
    Common::debug( "** pollForData(): %s; %d in send queue; %d in receive queue",
                   self->disconnectionFlag_ ? "Exiting" : "Running",
                   self->sendingQueue_.size(),
                   self->receivingQueue_.size() );
    Common::debug( "Used buffer:" );
    Common::printData( self->buffer_, self->bufferOffset_ );
    Common::debug( "Free buffer:" );
    Common::printData( self->buffer_ + self->bufferOffset_, MAX_MESSAGE_SIZE - self->bufferOffset_ );
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
//   Common::debug( "Session 0x%X: Receiving data...", this );

  int readBytes = recv( socket_,
                        buffer_ + bufferOffset_,
                        MAX_MESSAGE_SIZE - bufferOffset_,
                        0 );

  if( readBytes == 0 )
  {
    Common::debug( "Session 0x%X: Socket was closed", this );
    return true;
  }
  else if( readBytes < 0 )
  {
    Common::error( "Session 0x%X: error: Socket is closed", this );
    return true;
  }

  bufferOffset_ += readBytes;

  // Received data is shorter than the minimum message size, cannot be a valid message
  int messageHeaderSize = sizeof( MessageHeader );
  if( bufferOffset_ < messageHeaderSize )
  {
    Common::debug( "Not enough data yet. Minimum size is %d bytes, received only %d", messageHeaderSize, bufferOffset_ );
    return false;
  }

  Common::printData( buffer_, bufferOffset_, true, "Incoming message" );

  Message* message = parseMessage();

  if( message != NULL && message->type() != Message::MSG_INVALID )
  {
    receivingQueue_.push_back( message );
    availableMessages();
  }
  else
  {
    // Invalid message
    delete message;
    return true;
  }

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

  // Get the message payload
  int payloadSize = -1;
  char* payload = message->toRawBytes( payloadSize );

  // Generate the header
  MessageHeader header;
  int headerSize = sizeof( MessageHeader );

  memset( header.command, '\0', COMMAND_SIZE );
  strncpy( header.command, Message::command( message->type() ), COMMAND_SIZE );
  header.size = payloadSize;

  int sendBufferSize = headerSize + payloadSize;
  char* sendBuffer = static_cast<char*>( malloc( sendBufferSize ) );
  memcpy( sendBuffer, &header, headerSize );

  // If there's any payload, add it to the send buffer
  if( payloadSize > 0 )
  {
    memcpy( sendBuffer + headerSize, payload, payloadSize );
  }

  Common::printData( sendBuffer, sendBufferSize, false, "Sent message" );
  send( socket_, sendBuffer, sendBufferSize, 0 );

  free( sendBuffer );
  free( payload );
  delete message;

  return false;
}


