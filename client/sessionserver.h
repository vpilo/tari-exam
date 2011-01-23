/**
 * LAN Messenger
 * Copyright © 2011 Valerio Pilo
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef SESSIONSERVER_H
#define SESSIONSERVER_H

#include "sessionbase.h"

#include <stdio.h>


class Client;



/**
 * @class SessionServer
 *
 * Manages the connection to the server within the client application.
 */
class SessionServer : public SessionBase
{

  public:

    SessionServer( Client* parent, const int socket );
    virtual ~SessionServer();

    void chat( const char* message );
    virtual void disconnect();
    bool hasFileTransfer() const;
    const char* fileTransferName() const;
    const char* nickName() const;
    void setNickName( const char* nickName );
    void saveData( const char* buffer, int size, long int offset );
    void sendFile( const char* fileName );


  private:

    virtual void availableMessages();
    virtual void cycle();
    void disableFileTransferMode(  );


  private:

    /// Pointer to the parent client
    Client* client_;

    FILE* fileTransferHandle_;
    unsigned long long fileTransferOffset_;
    bool isReceivingFile_;
    bool isSendingFile_;
    bool hasFileTransferStarted_;
    char fileTransferBuffer_[ MAX_MESSAGE_SIZE ];

    char fileName_[ MAX_PATH_SIZE ];

    char nickName_[ MAX_NICKNAME_SIZE ];


};



#endif // SESSIONSERVER_H
