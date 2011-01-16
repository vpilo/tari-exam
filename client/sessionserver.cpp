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


