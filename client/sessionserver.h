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
    virtual void disconnect();


  public:

    void setNickName( const char* nickName );


  private:

    virtual void availableMessages();


  private:

    /// Pointer to the parent client
    Client* client_;


};



#endif // SESSIONSERVER_H
