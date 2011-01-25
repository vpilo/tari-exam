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
 * Use powers of 2 to keep the header structure byte-aligned and therefore,
 * to keep them better packed up through the network: compilers might add padding
 * bytes otherwise.
 *
 * Add one for the terminating NULL character.
 */
#define COMMAND_SIZE  4


/**
 * @def MAX_MESSAGE_SIZE
 *
 * A packet containing a message can be long at most this long.
 */
#define MAX_MESSAGE_SIZE   65495


/**
 * @def MAX_PATH_SIZE
 *
 * Maximum length of a filename path in bytes.
 */
#define MAX_PATH_SIZE  64


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
 * @def MAX_NETWORK_MESSAGE_QUEUE
 *
 * The OS will keep this amount of messages queued in case the server can't immediately process them.
 */
#define MAX_NETWORK_MESSAGE_QUEUE   40


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
