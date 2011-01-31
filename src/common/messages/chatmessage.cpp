/**
 * LAN Messenger
 * Copyright (C) 2011 Valerio Pilo
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
  payload_.messageSize = 0;
  setSender( NULL );
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



bool ChatMessage::fromRawBytes( const char* buffer, int bufferSize )
{
  int payloadSize = size();
  if( bufferSize < payloadSize )
  {
    Common::error( "Invalid buffer length: got %d, expected %d!", bufferSize, payloadSize );
    return false;
  }

  const Payload* readPayload = reinterpret_cast<const Payload*>( buffer );
  memcpy( &payload_, readPayload, payloadSize );

  if( bufferSize != ( payloadSize + payload_.messageSize ) )
  {
    Common::error( "Invalid payload length: got %d, expected %d!", bufferSize, ( payloadSize + payload_.messageSize ) );
    return false;
  }
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



const int ChatMessage::size() const
{
  // The pointer to the actual message within the payload structure
  // is just a commodity field, and shouldn't be sent
  return ( sizeof( Payload ) - sizeof( payload_.message ) + payload_.messageSize );
}



char* ChatMessage::toRawBytes() const
{
  int payloadSize = size();
  Payload* writePayload = static_cast<Payload*>( malloc( payloadSize ) );
  memset( writePayload, '\0', payloadSize );
  memcpy( writePayload, &payload_, payloadSize );
  memcpy( &(writePayload->message), message_, payload_.messageSize );

  return reinterpret_cast<char*>( writePayload );
}


