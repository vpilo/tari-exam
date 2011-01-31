/**
 * LAN Messenger
 * Copyright (C) 2011 Valerio Pilo
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
  payload_.nickname[ 0 ] = '\0';
}



NicknameMessage::NicknameMessage( const char* nickName )
: Message( Message::MSG_NICKNAME )
{
  setNickName( nickName );
}



NicknameMessage::~NicknameMessage()
{

}



bool NicknameMessage::fromRawBytes( const char* buffer, int size )
{
  int payloadSize = sizeof( Payload );
  if( size != payloadSize )
  {
    Common::error( "Invalid buffer length: got %d, expected %d!", size, payloadSize );
    return false;
  }

  memcpy( &payload_, buffer, payloadSize );

  return true;
}



const char* NicknameMessage::nickName() const
{
  return payload_.nickname;
}



void NicknameMessage::setNickName( const char* newNickName )
{
  // Zero it out first
  memset( payload_.nickname, '\0', MAX_NICKNAME_SIZE );
  strncpy( payload_.nickname, newNickName, MAX_NICKNAME_SIZE );
  payload_.nickname[ NICKNAME_FIELD_SIZE - 1 ] = '\0';
}



const int NicknameMessage::size() const
{
  return ( sizeof( Payload ) );
}



char* NicknameMessage::toRawBytes() const
{
  int payloadSize = size();
  char* buffer = static_cast<char*>( malloc( payloadSize ) );
  memcpy( buffer, &payload_, payloadSize );

  return buffer;
}


