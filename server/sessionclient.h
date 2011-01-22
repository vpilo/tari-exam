/**
 * LAN Messenger
 * Copyright © 2011 Valerio Pilo
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef SESSIONCLIENT_H
#define SESSIONCLIENT_H

#include "errors.h"
#include "sessionbase.h"


class Server;



/**
 * @class SessionClient
 *
 * A connection to a client within the server.
 *
 */
class SessionClient : public SessionBase
{
  public:

    SessionClient( Server* parent, const int socket );
    virtual ~SessionClient();

    Errors::StatusCode fileTransferAccepted() const;

    const char* nickName() const;
    void setNickName( const char* newNickName );


  private:

    virtual void availableMessages();


  private:

    Errors::StatusCode fileTransferStatus_;

    char nickName_[ MAX_NICKNAME_SIZE ];

    /// Pointer to the parent server
    Server* server_;


};



#endif // SESSIONCLIENT_H
