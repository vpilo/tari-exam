/**
 * LAN Messenger
 * Copyright © 2011 Valerio Pilo
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "nicknamemessage.h"

#include "common.h"

#include <string.h>
#include <stdlib.h>



NicknameMessage::NicknameMessage()
: Message( Message::MSG_NICKNAME )
{
  nickname_[ 0 ] = '\0';
}



NicknameMessage::NicknameMessage( const char* nickName )
: Message( Message::MSG_NICKNAME )
{
  setNickName( nickName );
}



NicknameMessage::~NicknameMessage()
{

}



char* NicknameMessage::data( int& size ) const
{
  if( type() != Message::MSG_NICKNAME )
  {
    Common::fatal( "Invalid nickname message!" );
  }

  NicknameMessageContents data;
  strcpy( data.header.command, command( type() ) );
  data.header.size = NICKNAME_FIELD_SIZE;
  strncpy( data.nickname, nickname_, NICKNAME_FIELD_SIZE );

  size = sizeof( NicknameMessageContents );
  char* buffer = static_cast<char*>( malloc( size ) );
  memcpy( buffer, &data, size );

  Common::debug( "Made nickname message buffer (%d bytes)", size );
  return buffer;
}



bool NicknameMessage::parseData( const void* buffer, int size )
{
  // We only have one field of specific size
  if( size != NICKNAME_FIELD_SIZE )
  {
    Common::error( "Invalid buffer length: got %d, expected %d!", size, NICKNAME_FIELD_SIZE );
    return false;
  }

  return true;
}



void NicknameMessage::setNickName( const char* newNickName )
{
  strncpy( nickname_, newNickName, MAX_NICKNAME_SIZE );
  nickname_[ NICKNAME_FIELD_SIZE - 1 ] = '\0';
}


