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

#include "errors.h"
#include "protocol.h"

#include <netinet/in.h>
#include <pthread.h>
#include <time.h>

#include <deque>


/**
 * @def HISTORY_SIZE
 * Number of rows of text saved as chat history
 */
#define HISTORY_SIZE   128


class SessionServer;



class Client
{

  public:

    Client();
    ~Client();

    Errors::ErrorCode initialize( const in_addr serverIp, const int serverPort );

    bool askQuestion( const char* question, char* answer );
    void changeStatusMessage( const char* message = NULL, bool permanent = false );
    void connectionClosed( SessionServer* connection );

    void gotChatMessage( const char* sender, const char* message );
    void gotNicknameChange( const char* nickName );
    void gotStatusMessage( const char* message );
    void run();
    void sendChatMessage( const char* message );

    void updateView();


  private:

      struct Row
      {
        char sender[ MAX_NICKNAME_SIZE ];
        char message[ MAX_MESSAGE_SIZE ];
        time_t dateTime;
        bool incoming;
        bool special;
      };


  private:

    std::deque<Row*> chatHistory_;

    SessionServer* connection_;

    pthread_t connectionThread_;

    int currentMessagePos_;
    char currentMessage_[ MAX_MESSAGE_SIZE ];

    int maxX_;
    int maxY_;

    int socket_;

    char statusMessage_[ MAX_MESSAGE_SIZE ];
    time_t statusMessageTime_;


};



#endif // CLIENT_H
