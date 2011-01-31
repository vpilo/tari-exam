/**
 * LAN Messenger
 * Copyright (C) 2011 Valerio Pilo
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "sessionclient.h"

#include "common.h"
#include "message.h"
#include "server.h"

#include "byemessage.h"
#include "chatmessage.h"
#include "filedatamessage.h"
#include "filetransfermessage.h"
#include "statusmessage.h"
#include "nicknamemessage.h"

#include <string.h>



SessionClient::SessionClient( Server* parent, const int socket )
: SessionBase( socket )
, fileTransferStatus_( Errors::Status_FileTransferCanceled )
, server_( parent )
{
}



SessionClient::~SessionClient()
{
  server_->removeSession( this );
}



void SessionClient::availableMessages()
{
  Message* message;
  while( ( message = receiveMessage() ) != NULL )
  {
    server_->checkSessionStateChange( this, message->type() );

    switch( message->type() )
    {
      case Message::MSG_HELLO:
      {
        // Send the client its initial nickname
        sendMessage( new NicknameMessage( nickName_ ) );
        break;
      }

      case Message::MSG_NICKNAME:
      {
        NicknameMessage* nickNameMessage = dynamic_cast<NicknameMessage*>( message );
        if( ! server_->clientChangedNickName( this, nickNameMessage ) )
        {
          // The nickname could not be changed, report the problem to the client
          sendMessage( new StatusMessage( Errors::Status_NickNameAlreadyRegistered ) );
        }
        else
        {
          // Change the nickname
          setNickName( nickNameMessage->nickName() );
          sendMessage( new NicknameMessage( nickName_ ) );
        }

        break;
      }

      case Message::MSG_STATUS:
      {
        StatusMessage* statusMessage = dynamic_cast<StatusMessage*>( message );
        Common::debug( "The client reports status code %d", statusMessage->statusCode() );

        Errors::StatusCode code = statusMessage->statusCode();
        switch( code )
        {
          case Errors::Status_Ok:
            Common::debug( "Session \"%s\" sent status OK", nickName_ );
            break;

          case Errors::Status_AcceptFileTransfer:
          case Errors::Status_RejectFileTransfer:
            if( ! server_->isFileTransferModeActive() )
            {
              sendMessage( new StatusMessage( Errors::Status_FileTransferCanceled ) );
              break;
            }

            // Save the status
            fileTransferStatus_ = code;

            server_->clientSentFileTransferResponse( this, code == Errors::Status_AcceptFileTransfer );
            break;

          default:
            Common::debug( "Session \"%s\" sent status %d", nickName_, code );
            break;
        }
        break;
      }

      case Message::MSG_CHAT:
      {
        ChatMessage* chatMessage = dynamic_cast<ChatMessage*>( message );
        if( ! server_->clientSentChatMessage( this, chatMessage ) )
        {
          sendMessage( new StatusMessage( Errors::Status_ChattingAlone ) );
        }
        break;
      }

      case Message::MSG_FILE_REQUEST:
      {
        // Deny new file transfers if one is already active
        if( server_->isFileTransferModeActive() )
        {
          sendMessage( new StatusMessage( Errors::Status_FileTransferCanceled ) );
          break;
        }

        FileTransferMessage* fileMessage = dynamic_cast<FileTransferMessage*>( message );
        if( ! server_->clientSentFileTransferRequest( this, fileMessage ) )
        {
          sendMessage( new StatusMessage( Errors::Status_ChattingAlone ) );
          break;
        }

        fileTransferStatus_ = Errors::Status_AcceptFileTransfer;
        break;
      }

      case Message::MSG_FILE_DATA:
      {
        FileDataMessage* dataMessage = dynamic_cast<FileDataMessage*>( message );

        server_->clientSentFileData( this, dataMessage );

        // Reset the file transfer status for the next file
        if( dataMessage->isLastBlock() )
        {
          fileTransferStatus_ = Errors::Status_FileTransferCanceled;
        }
        break;
      }

      default:
        break;
    }

    delete message;
  }
}



void SessionClient::disconnect()
{
  if( isConnected() )
  {
    sendMessage( new ByeMessage() );
    SessionBase::disconnect();
  }
}



Errors::StatusCode SessionClient::fileTransferAccepted() const
{
  return fileTransferStatus_;
}



const char* SessionClient::nickName() const
{
  return nickName_;
}



void SessionClient::setNickName( const char* newNickName )
{
  memset( nickName_, '\0', MAX_NICKNAME_SIZE );
  strncpy( nickName_, newNickName, MAX_NICKNAME_SIZE - 1 );
}


