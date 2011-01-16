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
 * @def COMMAND_SIZE
 *
 * Size of the command message in bytes.
 *
 * Add one for the terminating NULL character.
 */
#define COMMAND_SIZE  6


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
    virtual void* data( int& size ) const;

    Type type() const;


  public:

    /**
     * Identifies a received message within a data buffer.
     *
     * @return
     *  An instance of the proper Message subclass; or NULL in case of an error
     */
    static Message* parseHeader( const void* buffer, int size );

    /**
     * Creates a message of a certain type.
     *
     * @return
     *  An instance of the proper Message subclass; or NULL in case of an error
     */
    static Message* createMessage( const Type type );


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
     * Analyzes a data buffer to retrieve the specific message type's data.
     *
     * @return false on error (invalid data in the buffer)
     */
    virtual bool parseData( const void* buffer, int size );


  protected:

    /**
     * Container for the common message data
     */
    struct MessageHeader
    {
      // All commands are of the same size
      char command[ COMMAND_SIZE ];
      int size;
    };


  private:

    Type type_;


};



#endif // MESSAGE_H
