/**
 * LAN Messenger
 * Copyright © 2011 Valerio Pilo
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef SESSIONBASE_H
#define SESSIONBASE_H

#include "message.h"

#include <list>



class SessionBase
{
  public:

    SessionBase( const int socket );
    virtual ~SessionBase();

    virtual void disconnect();


  public:

    static void* pollForData( void* thisPointer );


  protected:

    /**
     * Take a message from the received message list.
     *
     * @return The next message to process, or NULL if there are none.
     */
    Message* receiveMessage();

    void sendMessage( Message* message );

    /**
     * New data is incoming and may be processed.
     */
    virtual void availableMessages() = 0;


  private:

    /**
     * Read some data from the socket.
     * @return true on error
     */
    bool readData();

    /**
     * Write queued messages to the socket.
     * @return true on error
     */
    bool writeData();


  private:

    bool disconnectionFlag_;

    std::list<Message*> receivingQueue_;

    std::list<Message*> sendingQueue_;

    int socket_;


};



#endif // SESSIONBASE_H
