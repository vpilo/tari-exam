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

    const char* nickName() const;
    void setNickName( const char* newNickName );


  private:

    virtual void availableMessages();


  private:

    char nickName_[ MAX_NICKNAME_SIZE ];

    /// Pointer to the parent server
    Server* server_;


};



#endif // SESSIONCLIENT_H
