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
, statusCode_( Errors::Status_Ok )
{
}



StatusMessage::StatusMessage( const Errors::StatusCode statusCode )
: Message( Message::MSG_STATUS )
, statusCode_( statusCode )
{
}



StatusMessage::~StatusMessage()
{

}



char* StatusMessage::data( int& size ) const
{
  if( type() != Message::MSG_STATUS )
  {
    Common::fatal( "Invalid status message!" );
  }

  StatusMessageContents data;
  strcpy( data.header.command, command( type() ) );
  data.header.size = sizeof( int );
  data.status = statusCode_;

  size = sizeof( StatusMessageContents );
  char* buffer = static_cast<char*>( malloc( size ) );
  memcpy( buffer, &data, size );

  Common::debug( "Made status message buffer (%d bytes)", size );
  return buffer;
}



const Errors::StatusCode StatusMessage::statusCode() const
{
  return statusCode_;
}



bool StatusMessage::parseData( const char* buffer, int size )
{
  // We only have one field of specific size
  if( size != sizeof( int ) )
  {
    Common::error( "Invalid buffer length: got %d, expected %d!", size, sizeof( int ) );
    return false;
  }

  // Clamp the buffer contents into the int size
  statusCode_ = *( reinterpret_cast<const Errors::StatusCode*>( buffer ) );

  Common::debug( "Read status code: %d", statusCode_ );
  return true;
}


