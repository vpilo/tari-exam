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
     case Message::MSG_STATUS:         return "STATE";
     case Message::MSG_CHAT:           return "MSG";
     case Message::MSG_FILE_REQUEST:   return "FILE";
     case Message::MSG_FILE_DATA:      return "FDATA";
     case Message::MSG_INVALID:
     default:
       break;
  }

  Common::fatal( "Attempted to call command() on an invalid message!" );
  return NULL;
}



char* Message::toRawBytes() const
{
  // Does nothing: class Message has no extra fields
  return NULL;
}


bool Message::fromRawBytes( const char*, int )
{
  // Does nothing: class Message has no extra fields
  return true;
}



const int Message::size() const
{
  // Does nothing: class Message has no extra fields
  return 0;
}



Message::Type Message::type() const
{
  return type_;
}



bool Message::operator == ( const Message& other ) const
{
  return ( type_ == other.type_ );
}


