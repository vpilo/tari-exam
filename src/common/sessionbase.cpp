/**
 * LAN Messenger
 * Copyright (C) 2011 Valerio Pilo
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

#include "byemessage.h"
#include "chatmessage.h"
#include "filedatamessage.h"
#include "filetransfermessage.h"
#include "hellomessage.h"
#include "nicknamemessage.h"
#include "statusmessage.h"

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



bool SessionBase::canSendMessages()
{
  return ( sendingQueue_.size() <= MAX_NETWORK_MESSAGE_QUEUE );
}



void SessionBase::disconnect()
{
  disconnectionFlag_ = true;
}



bool SessionBase::isConnected() const
{
  return ( ! disconnectionFlag_ );
}



Message* SessionBase::parseMessage()
{
  // Current position in the first message contained in the buffer
  int messageOffset_ = 0;

  int messageHeaderSize = sizeof( MessageHeader );

  MessageHeader messageHeader;
  memcpy( &messageHeader, buffer_, messageHeaderSize );

  messageOffset_ += messageHeaderSize;

  // Identify the command
  Message::Type type = Message::MSG_INVALID;
  for( int i = Message::MSG_INVALID + 1; i < Message::MSG_MAX; i++ )
  {
    Message::Type current = static_cast<Message::Type>( i );
    if( strcmp( messageHeader.command, Message::command( current ) ) == 0 )
    {
      type = current;
      break;
    }
  }


  // Validate the header fields

  // Command
  if( type == Message::MSG_INVALID )
  {
    Common::error( "Received invalid command \"%s\"!", messageHeader.command );
    return new Message();
  }
  // Payload size upper limit
  if( messageHeader.size > (int)MAX_PAYLOAD_SIZE )
  {
    Common::error( "Received invalid message payload size %d, it should have been at most %d!", messageHeader.size, MAX_PAYLOAD_SIZE );
    return new Message();
  }
  // A more precise check: if the message should contain X bytes but we have a X-n buffer,
  // posticipate the parsing
  if( ( bufferOffset_ - messageHeaderSize ) < messageHeader.size )
  {
    Common::debug( "Not enough payload data has arrived yet, waiting for more..." );
    return NULL;
  }

  // Make the message and pass to it only the message-specific data

  Message* message = NULL;

  switch( type )
  {
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
    case Message::MSG_FILE_REQUEST:
      message = new FileTransferMessage();
      break;
    case Message::MSG_FILE_DATA:
      message = new FileDataMessage();
      break;
    default:
      Common::error( "Could not create the message. Invalid type %d", type );
      return new Message();
  }

  // Position in the buffer where the first payload byte is located
  char* payloadBuffer = buffer_ + messageHeaderSize;

  bool isOk = message->fromRawBytes( payloadBuffer, messageHeader.size );
  if( ! isOk )
  {
    delete message;
    return NULL;
  }

  // Message is OK, move the rest of the buffer data at the start of the buffer
  // so the next one can be read

  messageOffset_ += message->size();

  int remainder = MAX_MESSAGE_SIZE - messageOffset_;
  char* remainderBuffer = static_cast<char*>( malloc( remainder ) );
  memcpy( remainderBuffer, buffer_ + messageOffset_, remainder );

  // Also zero it out so it's clearer where messages end
  memset( buffer_, '\0', MAX_MESSAGE_SIZE );
  memcpy( buffer_, remainderBuffer, remainder );
  bufferOffset_ -= messageOffset_;

  free( remainderBuffer );

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
  sigemptyset( &set );
  pthread_sigmask( SIG_BLOCK, NULL, &set );


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
#ifdef NETWORK_DEBUG
    Common::debug( "Used buffer:" );
    Common::printData( self->buffer_, self->bufferOffset_ );
    Common::debug( "Free buffer:" );
    Common::printData( self->buffer_ + self->bufferOffset_, MAX_MESSAGE_SIZE - self->bufferOffset_ );
#endif
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
    else if( ! self->disconnectionFlag_ )
    {
      self->cycle();
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
    Common::debug( "Session 0x%X: Nothing was read. Socket closed?", this );
    return true;
  }
  else if( readBytes < 0 )
  {
    Common::error( "Session 0x%X: Socket was closed!", this );
    return true;
  }

  bufferOffset_ += readBytes;

  // Received data is shorter than the minimum message size, cannot be a valid message
  int messageHeaderSize = sizeof( MessageHeader );
  if( bufferOffset_ < messageHeaderSize )
  {
    Common::debug( "Not enough data yet. Got %d out of %d minimum.", bufferOffset_, messageHeaderSize );
    return false;
  }

#ifdef NETWORK_DEBUG
  Common::printData( buffer_, bufferOffset_, true, "Incoming data" );
#endif

  Message* message = parseMessage();

  // There is not enough data yet for this kind of message
  if( message == NULL )
  {
    return false;
  }

  // The returned message isn't valid, something bad happened
  if( message->type() == Message::MSG_INVALID )
  {
    delete message;
    return true;
  }

  receivingQueue_.push_back( message );
  availableMessages();

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



bool SessionBase::sendMessage( Message* message )
{
  if( sendingQueue_.size() > MAX_NETWORK_MESSAGE_QUEUE )
  {
    return false;
  }

  sendingQueue_.push_back( message );
  return true;
}




bool SessionBase::writeData()
{
//   Common::debug( "Session 0x%X: Sending queued data...", this );

  if( sendingQueue_.size() == 0 )
  {
    Common::debug( "Session 0x%X: Nothing to send", this );
    return false;
  }

  Message* message = sendingQueue_.front();
  sendingQueue_.pop_front();

  // Get the message payload
  int payloadSize = message->size();
  char* payload = message->toRawBytes();

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

#ifdef NETWORK_DEBUG
  Common::printData( sendBuffer, sendBufferSize, false, "Sent message" );
#endif

  send( socket_, sendBuffer, sendBufferSize, 0 );

  free( sendBuffer );
  free( payload );
  delete message;

  return false;
}


