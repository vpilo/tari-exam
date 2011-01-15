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


/**
 * @def MAX_MESSAGE_SIZE
 *
 * A message can be long at most this long.
 */
#define MAX_MESSAGE_SIZE   5000


class Message
{

  public:

    enum Type
    {
      MSG_INVALID
    , MSG_HELLO
    , MSG_BYE
    , MSG_CHAT
    , MSG_FILE_REQUEST
    , MSG_FILE_ACK
    , MSG_FILE_NACK
    , MSG_FILE_DATA
    };


  public:

    Message();
    Message( const Message& other );
    virtual ~Message();
    virtual bool operator==( const Message& other ) const;
    Type type() const;


  public:

    /**
     * Identifies a received message within a data buffer.
     *
     * @return
     *  An instance of a Message subclass; or NULL in case of an error
     */
    static Message* parseData( const void* buffer, int size );


  private:

    Type type_;


};



#endif // MESSAGE_H
