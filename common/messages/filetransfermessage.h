/**
 * LAN Messenger
 * Copyright © 2011 Valerio Pilo
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef FILETRANSFERMESSAGE_H
#define FILETRANSFERMESSAGE_H

#include "message.h"
#include "protocol.h"



class FileTransferMessage : public Message
{

  public:

    FileTransferMessage();
    FileTransferMessage( const char* fileName );
    virtual ~FileTransferMessage();

    const char* fileName() const;
    void setFileName( const char* fileName );

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

    struct Payload
    {
      char sender[ MAX_NICKNAME_SIZE ];
      char fileName[ MAX_PATH_SIZE ];
    };

    /// Internal message data
    Payload payload_;


};



#endif // FILETRANSFERMESSAGE_H
