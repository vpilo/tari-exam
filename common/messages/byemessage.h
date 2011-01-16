/**
 * LAN Messenger
 * Copyright © 2011 Valerio Pilo
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef BYEMESSAGE_H
#define BYEMESSAGE_H

#include "message.h"



class ByeMessage : public Message
{

  public:
    ByeMessage() : Message( Message::MSG_BYE ) {};
};



#endif // BYEMESSAGE_H
