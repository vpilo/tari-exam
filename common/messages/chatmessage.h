/**
 * LAN Messenger
 * Copyright © 2011 Valerio Pilo
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef CHATMESSAGE_H
#define CHATMESSAGE_H

#include "message.h"
#include "protocol.h"


/**
 * @def CHATMESSAGE_FIELD_SIZE
 *
 * Size of the "message" field in the message contents, in bytes.
 * Used to ensure a NULL char is always present.
 */
#define CHATMESSAGE_FIELD_SIZE   ( MAX_CHATMESSAGE_SIZE + 1 )



class ChatMessage : public Message
{

  public:

    ChatMessage();
    ChatMessage( const char* message );
    virtual ~ChatMessage();

    const char* message() const;
    void setMessage( const char* message );

    /**
     * NULL if the sender is the user
     */
    const char* sender() const;
    void setSender( const char* sender );


  protected:

    /**
     * Override, analyzes a data buffer to retrieve the specific message type's data.
     * @see Overrides::fromRawBytes()
     */
    virtual bool fromRawBytes( const char* buffer, int size );

    /**
     * Override, convert the message contents into raw data.
     * @see Message::toRawBytes()
     */
    virtual char* toRawBytes( int& size ) const;


  private:

    /// Container for the chat message data
    struct Payload
    {
      char sender[ MAX_NICKNAME_SIZE ];
      int messageSize;
      char* message;
    };

    char message_[ CHATMESSAGE_FIELD_SIZE ];

    /// Internal message data
    Payload payload_;


};



#endif // CHATMESSAGE_H
