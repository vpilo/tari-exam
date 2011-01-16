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

#include "errors.h"
#include "message.h"

#include <netinet/in.h>
#include <pthread.h>

#include <map>


class SessionClient;



class Server
{
public:
  Server();
  ~Server();

    Errors::ErrorCode initialize( const char* address, const int port );

    void addSession( int newSocket );
    void removeSession( SessionClient* client );
    void checkSessionStateChange( SessionClient* client, Message::Type messageType );


private:

  enum ClientState
  {
    CLIENT_STATE_INVALID   /// The client is disconnected or in an error state
  , CLIENT_STATE_START     /// The client has just connected, but it did not salute nor identify yet
  , CLIENT_STATE_IDENTIFY  /// The client saluted but didn't identify itself yet
  , CLIENT_STATE_READY     /// The client is connected and can transfer messages
  , CLIENT_STATE_END       /// The client is about to disconnect
  };

  struct SessionData
  {
    SessionClient* client;
    pthread_t thread;
    ClientState state;
  };


private:

  SessionData* findSession( SessionClient* client );

  static void* waitConnections( void* thisPointer );


private:

  int listenSocket_;

  pthread_t listenThread_;

  pthread_mutex_t accessMutex_;

  std::map<SessionClient*,SessionData*> sessions_;


};



#endif // SERVER_H
