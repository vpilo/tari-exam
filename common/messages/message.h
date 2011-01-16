/**
 * LAN Messenger
 * Copyright © 2011 Valerio Pilo
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef MESSAGE_H
#define MESSAGE_H

#include "protocol.h"



class Message
{
  // Allow SessionBase to access createMessage and command()
  friend class SessionBase;

  public:

    enum Type
    {
      MSG_INVALID
    , MSG_HELLO
    , MSG_NICKNAME
    , MSG_BYE
    , MSG_CHAT
    , MSG_FILE_REQUEST
    , MSG_FILE_RESPONSE
    , MSG_FILE_DATA
    , MSG_MAX /// Total number of message types. Do not use.
    };


  public:

    virtual ~Message();
    virtual bool operator==( const Message& other ) const;

    Type type() const;


  protected:

    Message();
    Message( Type type );
    Message( const Message& other );

    /**
     * Get the message header for this kind of message.
     *
     * @return The commands used in messages when they're
     * transferred through the network.
     */
    static const char* command( Message::Type type );

    /**
     * Convert the message contents into raw data.
     *
     * Use this to create packets to send over the network.
     * Subclasses can override this method to implement custom packets.
     *
     * @note The programmer is responsible of free()ing the buffer after its use.
     * @param size
     *  This will be set to the amount of bytes used by the raw data.
     * @return
     *  Raw data buffer with the message, or NULL on error
     */
    virtual char* data( int& size ) const;

    /**
     * Analyzes a data buffer to retrieve the specific message type's data.
     *
     * @return false on error (invalid data in the buffer)
     */
    virtual bool parseData( const void* buffer, int size );


  private:

    Type type_;


};



#endif // MESSAGE_H
