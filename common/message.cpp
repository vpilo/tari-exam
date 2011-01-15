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
#include <stdlib.h>


#define COMMAND_HELLO "HELLO"
#define COMMAND_BYE   "BYE"



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



Message* Message::createMessage( const Message::Type type )
{
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

  if( message )
  {
    Common::debug( "Made new message of type %d", message->type_ );
  }
  else
  {
    Common::debug( "Could not create the message. Invalid type %d", type );
  }
  return message;
}



void* Message::data( int& size ) const
{
  MessageContents rawData;
  rawData.size = 0; // No extra fields

  switch( type_ )
  {
    case Message::MSG_HELLO:
      strcpy( rawData.command, COMMAND_HELLO );
      break;

    case Message::MSG_BYE:
      strcpy( rawData.command, COMMAND_BYE );
      break;

    default:
      // We can't manage more complex message types
      return NULL;
  }

  size = sizeof( MessageContents );
  void* buffer = malloc( size );
  memcpy( buffer, &rawData, size );

  return buffer;
}



Message* Message::parseData( const void* buffer, int size )
{
  if( size < 1 )
  {
    Common::error( "Received empty message!" );
    return NULL;
  }

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
  if     ( strcmp( messageData.command, COMMAND_HELLO ) == 0 ) type = Message::MSG_HELLO;
  else if( strcmp( messageData.command, COMMAND_BYE   ) == 0 ) type = Message::MSG_BYE;
  // TODO Add the rest of the commands
  else
  {
    Common::error( "Received invalid command \"%s\"!", messageData.command );

    return NULL;
  }

  return createMessage( type );
}



Message::Type Message::type() const
{
  return type_;
}



bool Message::operator == ( const Message& other ) const
{
  return ( type_ == other.type_ );
}


