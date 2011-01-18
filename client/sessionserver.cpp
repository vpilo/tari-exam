/**
 * LAN Messenger
 * Copyright © 2011 Valerio Pilo
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
#include "hellomessage.h"
#include "nicknamemessage.h"
#include "statusmessage.h"

#include "string.h"



SessionServer::SessionServer( Client* parent, const int socket )
: SessionBase( socket )
, client_( parent )
{
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
        strncpy( nickName_, nickNameMessage->nickName(), MAX_NICKNAME_SIZE - 1 );
        Common::debug( "Name changed to %s", nickName_ );
        break;
      }

      default:
        break;
    }

    delete message;
  }
}



void SessionServer::disconnect()
{
  sendMessage( new ByeMessage() );
  SessionBase::disconnect();
}



void SessionServer::setNickName( const char* nickName )
{
  sendMessage( new NicknameMessage( nickName ) );
}


