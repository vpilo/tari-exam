/**
 * LAN Messenger
 * Copyright © 2011 Valerio Pilo
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef CLIENTSESSION_H
#define CLIENTSESSION_H


class Server;



class ClientSession
{

  public:
    ClientSession( Server* parent, const int socket );
    virtual ~ClientSession();

  private:
    int socket_;

    /// Pointer to the parent server
    Server* server_;

};



#endif // CLIENTSESSION_H
