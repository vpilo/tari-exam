/**
 * LAN Messenger
 * Copyright © 2011 Valerio Pilo
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "message.h"

#include "common.h"

#include <string.h>


/**
 * @def COMMAND_SIZE
 *
 * Size of the command message in bytes.
 *
 * Add one for the terminating NULL character.
 */
#define COMMAND_SIZE  6



Message::Message()
: type_( MSG_INVALID )
{

}



Message::Message( const Message& other )
: type_( other.type_ )
{

}



Message::~Message()
{

}



bool Message::operator == ( const Message& other ) const
{
  return ( type_ == other.type_ );
}



Message* Message::parseData( const void* buffer, int size )
{
  if( size < 1 )
  {
    Common::error( "Received empty message!" );
    return NULL;
  }

  // Container for the common message data
  struct MessageContents
  {
    // All commands are of the same size
    char command[ COMMAND_SIZE ];
    int size;
  };
  int messageDataSize = sizeof( MessageContents );

  // Received data is shorter than the minimum message size, cannot be a valid message
  if( size < messageDataSize )
  {
    Common::error( "Received invalid message! Minimum size is %d, but received %d", messageDataSize, size );
    return NULL;
  }

  MessageContents messageData;
  memcpy( &messageData, buffer, messageDataSize );

  // Identify the command
  Message::Type type = Message::MSG_INVALID;
  if     ( messageData.command == "HELLO" ) type = Message::MSG_HELLO;
  else if( messageData.command == "BYE"   ) type = Message::MSG_BYE;
  // TODO Add the rest of the commands
  else
  {
    Common::error( "Received invalid command \"%s\"!", messageData.command );

    return NULL;
  }

  Message* message = NULL;

  switch( type )
  {
    // Simple messages don't need a specialized class
    case Message::MSG_HELLO:
    case Message::MSG_BYE:
      message = new Message;
      message->type_ = type;
      break;

    default:
//     message = new HelloMessage();
      break;
  }

  return message;
}



Message::Type Message::type() const
{
  return type_;
}


