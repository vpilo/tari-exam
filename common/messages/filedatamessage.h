/**
 * LAN Messenger
 * Copyright © 2011 Valerio Pilo
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef FILEDATAMESSAGE_H
#define FILEDATAMESSAGE_H

#include "message.h"
#include "protocol.h"



class FileDataMessage : public Message
{

  public:

    FileDataMessage();
    virtual ~FileDataMessage();

    const char* buffer() const;
    const int bufferSize() const;
    const unsigned long long fileOffset() const;
    const bool isLastBlock() const;

    void markLastBlock();
    void setBuffer( const char* buffer, const int size );
    void setFileOffset( const unsigned long long offset );

    /**
     * Override, tells how big the message-specific payload is.
     */
    virtual const int size() const;


  protected:

    /**
     * Override, analyzes a data buffer to retrieve the specific message type's data.
     * @see Overrides::fromRawBytes()
     */
    virtual bool fromRawBytes( const char* buffer, int bufferSize );

    /**
     * Override, convert the message contents into raw data.
     * @see Message::toRawBytes()
     */
    virtual char* toRawBytes() const;


  private:

    struct Payload
    {
      unsigned long long offset;
      bool isLast;
      int size;
      char* data;
    };

    /// Internal message data
    Payload payload_;


};



#endif // FFILEDATAMESSAGE_H
