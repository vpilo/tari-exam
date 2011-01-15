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
    switch( message->type() )
    {
      case Message::MSG_HELLO:
      case Message::MSG_BYE:
        server_->changeSessionState( this, message->type() );
        break;

      default:
        Common::error( "Unhandled message type %d received", message->type() );
        break;
    }
    delete message;
  }

}


