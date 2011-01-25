/**
 * LAN Messenger
 * Copyright © 2011 Valerio Pilo
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "filedatamessage.h"

#include "common.h"

#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>



FileDataMessage::FileDataMessage()
: Message( Message::MSG_FILE_DATA )
{
  payload_.offset = 0ULL;
  payload_.isLast = false;
  payload_.size = 0;
  payload_.data = NULL;
}



FileDataMessage::~FileDataMessage()
{
  free( payload_.data );
}



const char* FileDataMessage::buffer() const
{
  return payload_.data;
}



const int FileDataMessage::bufferSize() const
{
  return payload_.size;
}



const long FileDataMessage::fileOffset() const
{
  return payload_.offset;
}



bool FileDataMessage::fromRawBytes( const char* buffer, int bufferSize )
{
  int payloadSize = size();
  if( bufferSize < payloadSize )
  {
    Common::error( "Invalid buffer length: got %d, expected a minimum of %d!", bufferSize, payloadSize );
    return false;
  }

  const Payload* readPayload = reinterpret_cast<const Payload*>( buffer );
  memcpy( &payload_, readPayload, payloadSize );

  payload_.data = static_cast<char*>( malloc( payload_.size ) );
  memset( payload_.data, '\0', payload_.size );
  memcpy( payload_.data, &(readPayload->data), payload_.size );

  return true;
}



const bool FileDataMessage::isLastBlock() const
{
  return payload_.isLast;
}



void FileDataMessage::markLastBlock()
{
  payload_.isLast = true;
}



void FileDataMessage::setBuffer( const char* buffer, const int size )
{
  payload_.data = static_cast<char*>( malloc( size ) );
  payload_.size = size;

  memset( payload_.data, '\0', size );
  memcpy( payload_.data, buffer, size );
}



void FileDataMessage::setFileOffset( const long offset )
{
  payload_.offset = offset;
}



const int FileDataMessage::size() const
{
  // The pointer to the actual data within the payload structure
  // is just a commodity field, and shouldn't be sent
  return ( sizeof( Payload ) - sizeof( payload_.data ) + payload_.size );
}



char* FileDataMessage::toRawBytes() const
{
  int payloadSize = size();
  Payload* writePayload = static_cast<Payload*>( malloc( payloadSize ) );
  memset( writePayload, '\0', payloadSize );
  memcpy( writePayload, &payload_, payloadSize );
  memcpy( &(writePayload->data), payload_.data, payload_.size );

  return reinterpret_cast<char*>( writePayload );
}


