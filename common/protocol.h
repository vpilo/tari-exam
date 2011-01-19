/**
 * LAN Messenger
 * Copyright © 2011 Valerio Pilo
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef PROTOCOL_H
#define PROTOCOL_H


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
#define MAX_MESSAGE_SIZE   1024


/**
 * @def MAX_NICKNAME_SIZE
 *
 * Maximum length of the user nickname in bytes.
 */
#define MAX_NICKNAME_SIZE  36


/**
 * @def MAX_CHATMESSAGE_SIZE
 *
 * A text message can be long at most this long.
 */
#define MAX_CHATMESSAGE_SIZE   1024


/**
 * Container for the common message data
 */
struct MessageHeader
{
  // All commands are of the same size
  char command[ COMMAND_SIZE ];
  int size;
};



#endif // PROTOCOL_H
