/**
 * LAN Messenger
 * Copyright © 2011 Valerio Pilo
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef NICKNAMEMESSAGE_H
#define NICKNAMEMESSAGE_H

#include "message.h"
#include "protocol.h"


/**
 * @def NICKNAME_FIELD_SIZE
 *
 * Size of the "nickname" field in the message contents, in bytes.
 * Used to ensure a NULL char is always present.
 */
#define NICKNAME_FIELD_SIZE   ( MAX_NICKNAME_SIZE + 1 )



class NicknameMessage : public Message
{

  public:

    NicknameMessage();
    NicknameMessage( const char* nickName );
    virtual ~NicknameMessage();

    const char* nickName() const;
    void setNickName( const char* newNickName );


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

    /// Container for the nickname message data
    struct Payload
    {
      char nickname[ NICKNAME_FIELD_SIZE ];
    };

    /// Internal message data
    Payload payload_;


};



#endif // NICKNAMEMESSAGE_H
