/**
 * LAN Messenger
 * Copyright (C) 2011 Valerio Pilo
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "sessionserver.h"

#include "common.h"
#include "client.h"

#include "byemessage.h"
#include "chatmessage.h"
#include "filedatamessage.h"
#include "filetransfermessage.h"
#include "hellomessage.h"
#include "nicknamemessage.h"
#include "statusmessage.h"

#include "errno.h"
#include "string.h"



SessionServer::SessionServer( Client* parent, const int socket )
: SessionBase( socket )
, client_( parent )
, fileTransferHandle_( NULL )
, fileTransferOffset_( 0LL )
, isReceivingFile_( false )
, isSendingFile_( false )
, hasFileTransferStarted_( false )
{
  *fileName_ = '\0';

  sendMessage( new HelloMessage() );
}



SessionServer::~SessionServer()
{
  client_->connectionClosed( this );
}



void SessionServer::availableMessages()
{
  Message* message;
  while( ( message = receiveMessage() ) != NULL )
  {
    switch( message->type() )
    {
      case Message::MSG_STATUS:
      {
        StatusMessage* statusMessage = dynamic_cast<StatusMessage*>( message );
        Common::error( "The server reports status code %d", statusMessage->statusCode() );

        switch( statusMessage->statusCode() )
        {
          case Errors::Status_NickNameAlreadyRegistered:
            // Keep the original nickname, the wanted one wasn't accepted
            Common::error( "The nickname change wasn't accepted." );
            client_->gotStatusMessage( "Unable to change nickname!" );
            break;

          case Errors::Status_ChattingAlone:
            client_->gotStatusMessage( "There are no other participants to the chat!" );
            break;

          case Errors::Status_AcceptFileTransfer:

            if( ! isSendingFile_ || strlen( fileName_ ) == 0 )
            {
              Common::fatal( "Client doesn't have started a file transfer!" );
            }

            hasFileTransferStarted_ = true; // let cycle() go
            client_->gotStatusMessage( "The transfer of \"%s\" has started.", fileName_ );
            break;

          case Errors::Status_RejectFileTransfer:

            if( ! isSendingFile_ || strlen( fileName_ ) == 0 )
            {
              Common::fatal( "Client doesn't have started a file transfer!" );
            }

            disableFileTransferMode();
            client_->gotStatusMessage( "The file transfer was rejected by the other participants." );
            break;

          case Errors::Status_FileTransferCanceled:
            if( ! isSendingFile_ )
            {
              Common::fatal( "Client doesn't have started a file transfer!" );
            }

            disableFileTransferMode();
            client_->gotStatusMessage( "Unable to send the file! Only one file may be in transfer at a time." );
            break;

          default:
            break;
        }
        break;
      }

      case Message::MSG_NICKNAME:
      {
        NicknameMessage* nickNameMessage = dynamic_cast<NicknameMessage*>( message );

        // We'll take whatever nickname the server gives us
        memset( nickName_, '\0', MAX_NICKNAME_SIZE );
        strncpy( nickName_, nickNameMessage->nickName(), MAX_NICKNAME_SIZE - 1 );
        Common::debug( "Name changed to %s", nickName_ );
        client_->gotNicknameChange( nickName_ );
        break;
      }

      case Message::MSG_CHAT:
      {
        ChatMessage* chatMessage = dynamic_cast<ChatMessage*>( message );

        Common::debug( "Got message by '%s': %s", chatMessage->sender(), chatMessage->message() );
        client_->gotChatMessage( chatMessage->sender(), chatMessage->message() );
        break;
      }

      case Message::MSG_FILE_REQUEST:
      {
        FileTransferMessage* fileMessage = dynamic_cast<FileTransferMessage*>( message );

        Common::debug( "Got file transfer request by '%s': %s", fileMessage->sender(), fileMessage->fileName() );

        bool accepted = client_->gotFileTransferRequest( fileMessage->sender(),fileMessage->fileName(), fileName_ );
        isReceivingFile_ = accepted;

        Common::debug( "File transfer %s", accepted ? "accepted" : "rejected" );
        break;
      }

      case Message::MSG_FILE_DATA:
      {
        // We had ignored the file request
        if( ! isReceivingFile_ )
        {
          break;
        }

        FileDataMessage* dataMessage = dynamic_cast<FileDataMessage*>( message );
        saveData( dataMessage->buffer(), dataMessage->bufferSize(), dataMessage->fileOffset() );

        if( dataMessage->isLastBlock() )
        {
          client_->gotStatusMessage( "The file \"%s\" was received.", fileName_ );
          disableFileTransferMode();
        }
        break;
      }

      default:
        break;
    }

    delete message;
  }
}



void SessionServer::chat( const char* message )
{
  sendMessage( new ChatMessage( message ) );
}



void SessionServer::cycle()
{
  if( ! isSendingFile_ || ! hasFileTransferStarted_ )
  {
    return;
  }

  if( fileTransferHandle_ == NULL )
  {
    fileTransferHandle_ = fopen( fileName_, "r" );

    if( fileTransferHandle_ == NULL )
    {
      // Opening the file failed somehow
      Common::error( "Couldn't open %s: %s", fileName_, strerror( errno ) );

      client_->gotStatusMessage( "Unable to open file %s! %s", fileName_, strerror( errno ) );
      disableFileTransferMode();
      return;
    }
  }

  // The file is open, we can send the data if the queue isn't full
  if( ! canSendMessages() )
  {
    return;
  }

  FileDataMessage* message = new FileDataMessage();


  const int maxPayloadSize = MAX_PAYLOAD_SIZE - message->size();

  int offset;
  bool endOfFile = false;
  for( offset = 0; offset < maxPayloadSize; offset++ )
  {
    int read = fread( fileTransferBuffer_ + offset, 1, 1, fileTransferHandle_ );

    // EOF reached
    if( read == 0 && feof( fileTransferHandle_ ) != 0 )
    {
      Common::debug( "End of file reached." );
      client_->gotStatusMessage( "The file has been sent." );
      endOfFile = true;
      break;
    }
  }

//   Common::debug( "File: Read %d (max %d) chars from offset %lu, last? %s", offset, maxPayloadSize, fileTransferOffset_, endOfFile?"yes":"no" );

  message->setBuffer( fileTransferBuffer_, offset );
  message->setFileOffset( fileTransferOffset_ );

  fileTransferOffset_ += offset;

  if( endOfFile )
  {
    message->markLastBlock();
    disableFileTransferMode();
  }

  sendMessage( message );
}



void SessionServer::disableFileTransferMode()
{
  // Reset the state variables

  if( fileTransferHandle_ != NULL )
  {
    fclose( fileTransferHandle_ );
    fileTransferHandle_ = NULL;
  }

  fileTransferOffset_ = 0;
  isReceivingFile_ = false;
  isSendingFile_ = false;
  hasFileTransferStarted_ = false;
  *fileName_ = '\0';
}



void SessionServer::disconnect()
{
  if( isConnected() )
  {
    sendMessage( new ByeMessage() );
    SessionBase::disconnect();
  }
}



const char* SessionServer::fileTransferName() const
{
  return fileName_;
}



bool SessionServer::hasFileTransfer() const
{
  return ( isReceivingFile_ || isSendingFile_ );
}



const char* SessionServer::nickName() const
{
  return nickName_;
}



void SessionServer::saveData( const char* buffer, int size, long offset )
{
  if( ! isReceivingFile_ )
  {
    return;
  }

  if( fileTransferHandle_ == NULL )
  {
    fileTransferHandle_ = fopen( fileName_, "w" );

    if( fileTransferHandle_ == NULL )
    {
      // Opening the file failed somehow
      Common::error( "Couldn't open %s: %s", fileName_, strerror( errno ) );
      char message[ MAX_CHATMESSAGE_SIZE ];
      sprintf( message, "Unable to open file %s! %s", fileName_, strerror( errno ) );
      client_->gotStatusMessage( message );
      disableFileTransferMode();
      return;
    }
  }

  fseek( fileTransferHandle_, offset, SEEK_SET );
  fwrite( buffer, size, 1, fileTransferHandle_ );

  Common::debug( "File: Saved %d chars at offset %ld", size, offset );
}



void SessionServer::sendFile( const char* fileName )
{
  isSendingFile_ = true;
  strncpy( fileName_, fileName, MAX_PATH_SIZE );

  sendMessage( new FileTransferMessage( fileName ) );
}



void SessionServer::setNickName( const char* nickName )
{
  sendMessage( new NicknameMessage( nickName ) );
}


