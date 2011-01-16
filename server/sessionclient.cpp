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
#include "nicknamemessage.h"
#include "server.h"



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
      case Message::MSG_NICKNAME:
      {
        NicknameMessage* nickNameMessage = dynamic_cast<NicknameMessage*>( message );
        server_->clientChangedNickName( this, nickNameMessage->nickName() );
        break;
      }

      default:
        break;
    }

    delete message;
  }

}


