/**
 * LAN Messenger
 * Copyright © 2011 Valerio Pilo
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef SERVER_H
#define SERVER_H

#include <netinet/in.h>
#include <pthread.h>

#include <list>

#include "errors.h"

class ClientSession;



class Server
{
public:
  Server();
  ~Server();

    Errors::ErrorCode initialize( const char* address, const int port );

    void addSession( int newSocket );
    void removeSession( ClientSession* session );

private:

  static void* waitConnections( void* thisPointer );

private:

  int listenSocket_;

  pthread_t listenThread_;

  std::list<ClientSession*> sessions_;

};



#endif // SERVER_H
