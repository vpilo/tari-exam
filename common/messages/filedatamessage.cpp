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



const unsigned long long FileDataMessage::fileOffset() const
{
  return payload_.offset;
}



bool FileDataMessage::fromRawBytes( const char* buffer, int bufferSize )
{
  int payloadSize = size();
  if( bufferSize < payloadSize )
  {
//     Common::error( "Invalid buffer length: got %d, expected %d!", size, payloadSize );
    return true;
  }

  const Payload* readPayload = reinterpret_cast<const Payload*>( buffer );
  memcpy( &payload_, readPayload, payloadSize );
//   memset( message_, '\0', MAX_CHATMESSAGE_SIZE );
//   memcpy( message_, &(readPayload->message), payload_.messageSize );

/*
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
*/

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



void FileDataMessage::setFileOffset( const unsigned long long offset )
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

  Common::debug( "Message: Copied %d chars from offset %lu, last? %s", payload_.size, payload_.offset, ( payload_.isLast < MAX_MESSAGE_SIZE )?"yes":"no" );
  /*
  memset(&payload_, '\x00', sizeof( payload_ ));
  memset(&payload_.offset, '\xDD', sizeof( payload_.offset ));
  memset(&payload_.isLast, '\xCC', sizeof( payload_.isLast ));
  memset(&payload_.size, '\xEE', sizeof( payload_.size ));
  memset(&payload_.data, '\xFF', sizeof( payload_.data ));
  Common::debug( "raw payload size is: %d", size() );
  Common::printData( (char*)&payload_, size() );
******************************************************************************
00000 : dd dd dd dd dd dd dd dd cc 00 00 00 ee ee ee ee : ................
00016 : ff ff ff ff ff ff ff ff                         : ........
******************************************************************************
*/
  return reinterpret_cast<char*>( writePayload );
}


