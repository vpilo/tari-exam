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
     * Convert the message contents into raw data.
     *
     * Reimplements Message::data().
     *
     * @see Message::data()
     * @param size
     *  This will be set to the amount of bytes used by the raw data.
     * @return
     *  Raw data buffer with the message, or NULL on error
     */
    virtual char* data( int& size ) const;

    /**
     * Analyzes a data buffer to retrieve the specific message type's data.
     *
     * Reimplements Message::parseData().
     *
     * @see Message::parseData()
     */
    virtual bool parseData( const char* buffer, int size );


  private:

    /**
     * Container for the nickname message data
     */
    struct NicknameMessageContents
    {
      MessageHeader header;
      char nickname[ NICKNAME_FIELD_SIZE ];
    };

    char nickname_[ NICKNAME_FIELD_SIZE ];
};



#endif // NICKNAMEMESSAGE_H
