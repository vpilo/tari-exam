/**
 * LAN Messenger
 * Copyright © 2011 Valerio Pilo
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "connection.h"

#include <unistd.h>



Connection::Connection( const int socket )
: socket_( socket )
{

}



Connection::~Connection()
{
  close( socket_ );
}


