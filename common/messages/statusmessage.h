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

    StatusMessage( const Errors::StatusCode statusCode );
    virtual ~StatusMessage();

    const Errors::StatusCode statusCode() const;


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
     * Container for the status message data
     */
    struct StatusMessageContents
    {
      MessageHeader header;
      int status;
    };

    Errors::StatusCode statusCode_;


};



#endif // STATUSMESSAGE_H
