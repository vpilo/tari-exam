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
#include "statusmessage.h"
#include "nicknamemessage.h"
#include "server.h"
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
        setNickName( nickName() );
        break;
      }
      case Message::MSG_NICKNAME:
      {
        Errors::StatusCode result;
        NicknameMessage* nickNameMessage = dynamic_cast<NicknameMessage*>( message );
        if( ! server_->clientChangedNickName( this, nickNameMessage->nickName() ) )
        {
          // The nickname could not be changed, report the problem to the client
          result = Errors::Status_NickNameAlreadyRegistered;
          setNickName( nickName() );
        }
        else
        {
          // Change the nickname
          result = Errors::Status_Ok;
          setNickName( nickNameMessage->nickName() );
        }

        sendMessage( new StatusMessage( result ) );
        break;
      }
      case Message::MSG_STATUS:
      {
        StatusMessage* statusMessage = dynamic_cast<StatusMessage*>( message );
        Common::debug( "The client reports status code %d", statusMessage->statusCode() );
        /// TODO Parse the error message
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
  strncpy( nickName_, newNickName, MAX_NICKNAME_SIZE - 1 );
  sendMessage( new NicknameMessage( nickName_ ) );
}


