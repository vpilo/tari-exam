/**
 * LAN Messenger
 * Copyright © 2011 Valerio Pilo
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

#include "chatmessage.h"
#include "filetransfermessage.h"
#include "statusmessage.h"
#include "nicknamemessage.h"

#include <string.h>



SessionClient::SessionClient( Server* parent, const int socket )
: SessionBase( socket )
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
        if( ! server_->clientChangedNickName( this, nickNameMessage->nickName() ) )
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

        if( statusMessage->statusCode() != Errors::Status_Ok )
        {
          /// TODO Parse the error message
          Common::error( "Error!" );
        }
        break;
      }

      case Message::MSG_CHAT:
      {
        ChatMessage* chatMessage = dynamic_cast<ChatMessage*>( message );
        if( ! server_->clientSentChatMessage( this, chatMessage->message() ) )
        {
          sendMessage( new StatusMessage( Errors::Status_ChattingAlone ) );
        }
        break;
      }

      case Message::MSG_FILE_REQUEST:
      {
        FileTransferMessage* fileMessage = dynamic_cast<FileTransferMessage*>( message );
        if( ! server_->clientSentFileTransferMessage( this, fileMessage->fileName() ) )
        {
          sendMessage( new StatusMessage( Errors::Status_ChattingAlone ) );
        }
        break;
      }

      default:
        break;
    }

    delete message;
  }
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


