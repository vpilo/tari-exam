/**
 * LAN Messenger
 * Copyright (C) 2011 Valerio Pilo
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
    bool isConnected() const;

    /**
     * Send a message.
     * @return False if the queue is full
     */
    bool sendMessage( Message* message );


  public:

    static void* pollForData( void* thisPointer );


  protected:

    /**
     * New data is incoming and may be processed.
     */
    virtual void availableMessages() = 0;

    /**
     * Return whether messages can be sent.
     * @return bool
     */
    virtual bool canSendMessages();

    /**
     * Take a message from the received message list.
     *
     * @return The next message to process, or NULL if there are none.
     */
    Message* receiveMessage();

    /**
     * Invoked every time the class does anything.
     *
     * This method can be used to perform extra operations at each network transfer.
     */
    virtual void cycle() { /* Do nothing */ };


  private:

    /**
     * Identifies a received message within the data buffer.
     *
     * @return
     *  NULL if no messages are available yet;
     *  an instance of the proper Message subclass if a good message is found;
     *  an instance of an invalid Message in case of error
     */
    Message* parseMessage();

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

    char* buffer_;

    /// Total amount of unread bytes in the buffer
    int bufferOffset_;

    bool disconnectionFlag_;

    std::list<Message*> receivingQueue_;

    std::list<Message*> sendingQueue_;

    int socket_;


};



#endif // SESSIONBASE_H
