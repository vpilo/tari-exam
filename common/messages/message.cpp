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
#include "nicknamemessage.h"



Message::Message()
: type_( Message::MSG_INVALID )
{

}



Message::Message( Message::Type type )
: type_( type )
{

}



Message::Message( const Message& other )
: type_( other.type_ )
{

}



Message::~Message()
{

}



const char* Message::command( Message::Type type )
{
  // The maximum message command is defined as COMMAND_SIZE in message.h
  switch( type )
  {
     case Message::MSG_HELLO:          return "HELLO";
     case Message::MSG_NICKNAME:       return "NICK";
     case Message::MSG_BYE:            return "BYE";
     case Message::MSG_CHAT:           return "MSG";
     case Message::MSG_FILE_REQUEST:   return "FTQ";
     case Message::MSG_FILE_RESPONSE:  return "FTA";
     case Message::MSG_FILE_DATA:      return "FTD";
     case Message::MSG_INVALID:
     default:
       break;
  }

  Common::fatal( "Attempted to call command() on an invalid message!" );
  return NULL;
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
    case Message::MSG_NICKNAME:
      message = new NicknameMessage;
    default:
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
  MessageHeader rawData;
  rawData.size = 0; // No extra fields

  switch( type_ )
  {
    case Message::MSG_HELLO:
    case Message::MSG_BYE:
      strcpy( rawData.command, command( type_ ) );
      break;

    default:
      // We can't manage more complex message types
      return NULL;
  }

  size = sizeof( MessageHeader );
  void* buffer = malloc( size );
  memcpy( buffer, &rawData, size );

  return buffer;
}


bool Message::parseData( const void*, int )
{
  // Does nothing: class Message has no extra fields
  return true;
}



Message* Message::parseHeader( const void* buffer, int size )
{
  if( size < 1 )
  {
    Common::error( "Received empty message!" );
    return NULL;
  }

  int messageHeaderSize = sizeof( MessageHeader );

  // Received data is shorter than the minimum message size, cannot be a valid message
  if( size < messageHeaderSize )
  {
    Common::error( "Received invalid message! Minimum size is %d, but received %d", messageHeaderSize, size );
    return NULL;
  }

  MessageHeader messageData;
  memcpy( &messageData, buffer, messageHeaderSize );

  // Identify the command
  Message::Type type = Message::MSG_INVALID;
  for( int i = Message::MSG_INVALID + 1; i < Message::MSG_MAX; i++ )
  {
    Message::Type current = static_cast<Type>( i );
    if( strcmp( messageData.command, command( current ) ) == 0 )
    {
      type = current;
      break;
    }
  }

  if( type == Message::MSG_INVALID )
  {
    Common::error( "Received invalid command \"%s\"!", messageData.command );

    return NULL;
  }

  Message* newMessage = createMessage( type );

  char* dataBuffer = reinterpret_cast<char*>( const_cast<void*>( buffer ) );
  dataBuffer += messageHeaderSize;

  // Parse the message-specific data
  bool isOk = newMessage->parseData( dataBuffer, size - messageHeaderSize );
  if( ! isOk )
  {
    delete newMessage;
    newMessage = NULL;
  }

  return newMessage;
}



Message::Type Message::type() const
{
  return type_;
}



bool Message::operator == ( const Message& other ) const
{
  return ( type_ == other.type_ );
}


