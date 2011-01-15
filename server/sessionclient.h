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

#include "message.h"

#include <list>


class Server;



class SessionClient
{


  public:

    SessionClient( Server* parent, const int socket );
    virtual ~SessionClient();


  public:

    static void* pollForData( void* thisPointer );


  private:

    void getMessage( const void* buffer, int size );
    bool readData();
    bool writeData();


  private:

    /// Pointer to the parent server
    Server* server_;

    int socket_;

    std::list<Message*> receivingQueue_;
    std::list<Message*> sendingQueue_;


};



#endif // CLIENTSESSION_H
