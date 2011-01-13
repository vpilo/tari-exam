/**
 * LAN Messenger
 * Copyright © 2011 Valerio Pilo
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef CONNECTION_H
#define CONNECTION_H



class Connection
{

public:
    Connection( const int socket );
    virtual ~Connection();

private:
    int socket_;

};



#endif // CONNECTION_H
