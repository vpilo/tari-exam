/**
 * LAN Messenger
 * Copyright © 2011 Valerio Pilo
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef CLIENT_H
#define CLIENT_H

#include <netinet/in.h>
#include <pthread.h>

#include <list>

#include "errors.h"


class SessionServer;



class Client
{

public:

  Client();
  ~Client();

  Errors::ErrorCode initialize( const in_addr serverIp, const int serverPort );

  void connectionClosed( SessionServer* connection );

  void gotChatMessage( const char* sender, const char* message );
  void run();

private:

  int socket_;

  SessionServer* connection_;
  pthread_t connectionThread_;


};



#endif // CLIENT_H
