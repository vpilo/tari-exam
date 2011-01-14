/**
 * LAN Messenger
 * Copyright © 2011 Valerio Pilo
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "clientsession.h"
#include "server.h"

#include <unistd.h>



ClientSession::ClientSession( Server* parent, const int socket )
: server_( parent )
, socket_( socket )
{

}



ClientSession::~ClientSession()
{
  server_->removeSession( this );
  close( socket_ );
}


