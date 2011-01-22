/**
 * LAN Messenger
 * Copyright © 2011 Valerio Pilo
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef STATUSMESSAGE_H
#define STATUSMESSAGE_H

#include "message.h"
#include "errors.h"
#include "protocol.h"



class StatusMessage : public Message
{

  public:

    StatusMessage();
    StatusMessage( const Errors::StatusCode statusCode );
    virtual ~StatusMessage();

    /**
     * Override, tells how big the message-specific payload is.
     */
    virtual const int size() const;

    const Errors::StatusCode statusCode() const;


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
    virtual char* toRawBytes() const;


  private:

    /// Container for the status message data
    struct Payload
    {
      Errors::StatusCode status;
    };

    /// Internal message data
    Payload payload_;


};



#endif // STATUSMESSAGE_H
