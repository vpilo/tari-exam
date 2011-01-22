/**
 * LAN Messenger
 * Copyright © 2011 Valerio Pilo
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "statusmessage.h"

#include "common.h"

#include "string.h"
#include "stdlib.h"



StatusMessage::StatusMessage()
: Message( Message::MSG_STATUS )
{
  payload_.status = Errors::Status_Ok;
}



StatusMessage::StatusMessage( const Errors::StatusCode statusCode )
: Message( Message::MSG_STATUS )
{
  payload_.status = statusCode;
}



StatusMessage::~StatusMessage()
{

}



const Errors::StatusCode StatusMessage::statusCode() const
{
  return payload_.status;
}



bool StatusMessage::fromRawBytes( const char* buffer, int size )
{
  int payloadSize = sizeof( Payload );
  if( size != payloadSize )
  {
    Common::error( "Invalid buffer length: got %d, expected %d!", size, payloadSize );
    return false;
  }

  memcpy( &payload_, buffer, payloadSize );

  Common::debug( "Read status code: %d", payload_.status );

  return true;
}



const int StatusMessage::size() const
{
  return ( sizeof( Payload ) );
}



char* StatusMessage::toRawBytes() const
{
  int payloadSize = size();
  char* buffer = static_cast<char*>( malloc( payloadSize ) );
  memcpy( buffer, &payload_, payloadSize );

  Common::debug( "Made message buffer for status %d (%d bytes)", payload_.status, payloadSize );

  return buffer;
}


