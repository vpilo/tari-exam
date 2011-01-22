/**
 * LAN Messenger
 * Copyright © 2011 Valerio Pilo
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "filetransfermessage.h"

#include "common.h"

#include <string.h>
#include <stdlib.h>



FileTransferMessage::FileTransferMessage()
: Message( Message::MSG_FILE_REQUEST )
{
  setFileName( NULL );
  setSender( NULL );
}



FileTransferMessage::FileTransferMessage( const char* fileName )
: Message( Message::MSG_FILE_REQUEST )
{
  setFileName( fileName );
  setSender( NULL ); // Message from the user
}



FileTransferMessage::~FileTransferMessage()
{

}



bool FileTransferMessage::fromRawBytes( const char* buffer, int size )
{
  int payloadSize = sizeof( Payload );
  if( size < payloadSize )
  {
    Common::error( "Invalid buffer length: got %d, expected %d!", size, payloadSize );
    return false;
  }

  const Payload* readPayload = reinterpret_cast<const Payload*>( buffer );
  memcpy( &payload_, readPayload, payloadSize );

  return true;
}



const char* FileTransferMessage::fileName() const
{
  return payload_.fileName;
}



const char* FileTransferMessage::sender() const
{
  return payload_.sender;
}



void FileTransferMessage::setFileName( const char* fileName )
{
  memset( payload_.fileName, '\0', MAX_PATH_SIZE );
  if( fileName != NULL && strlen( fileName ) > 0 )
  {
    strncpy( payload_.fileName, fileName, MAX_PATH_SIZE );
  }
}



void FileTransferMessage::setSender( const char* sender )
{
  memset( payload_.sender, '\0', MAX_NICKNAME_SIZE );
  if( sender != NULL && strlen( sender ) > 0 )
  {
    strncpy( payload_.sender, sender, MAX_NICKNAME_SIZE );
  }
}



const int FileTransferMessage::size() const
{
  return ( sizeof( Payload ) );
}



char* FileTransferMessage::toRawBytes() const
{
  int payloadSize = size();
  Payload* writePayload = static_cast<Payload*>( malloc( payloadSize ) );
  memset( writePayload, '\0', payloadSize );
  memcpy( writePayload, &payload_, payloadSize );

  return reinterpret_cast<char*>( writePayload );
}


