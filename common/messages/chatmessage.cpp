/**
 * LAN Messenger
 * Copyright © 2011 Valerio Pilo
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "chatmessage.h"

#include "common.h"

#include <string.h>
#include <stdlib.h>



ChatMessage::ChatMessage()
: Message( Message::MSG_CHAT )
{
  payload_.message = NULL;
  setSender( NULL ); // Message from the user
}



ChatMessage::ChatMessage( const char* message )
: Message( Message::MSG_CHAT )
{
  setMessage( message );
  setSender( NULL ); // Message from the user
}



ChatMessage::~ChatMessage()
{

}



bool ChatMessage::fromRawBytes( const char* buffer, int size )
{
  int payloadSize = sizeof( Payload );
  if( size < payloadSize )
  {
    Common::error( "Invalid buffer length: got %d, expected %d!", size, payloadSize );
    return false;
  }

  const Payload* readPayload = reinterpret_cast<const Payload*>( buffer );
  memcpy( &payload_, readPayload, payloadSize );
  memset( message_, '\0', MAX_CHATMESSAGE_SIZE );
  memcpy( message_, &(readPayload->message), payload_.messageSize );

  return true;
}



const char* ChatMessage::message() const
{
  return message_;
}



const char* ChatMessage::sender() const
{
  return payload_.sender;
}



void ChatMessage::setMessage( const char* message )
{
  memset( message_, '\0', MAX_CHATMESSAGE_SIZE );
  strncpy( message_, message, MAX_CHATMESSAGE_SIZE );
  payload_.messageSize = strlen( message_ );
}



void ChatMessage::setSender( const char* sender )
{
  memset( payload_.sender, '\0', MAX_NICKNAME_SIZE );
  if( sender != NULL && strlen( sender ) > 0 )
  {
    strncpy( payload_.sender, sender, MAX_NICKNAME_SIZE );
  }
}



char* ChatMessage::toRawBytes( int& size ) const
{
  size = sizeof( Payload ) + payload_.messageSize;
  Payload* writePayload = static_cast<Payload*>( malloc( size ) );
  memset( writePayload, '\0', size );
  memcpy( writePayload, &payload_, size );
  memcpy( &(writePayload->message), message_, payload_.messageSize );

  return reinterpret_cast<char*>( writePayload );
}


