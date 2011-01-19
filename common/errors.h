/**
 * LAN Messenger
 * Copyright © 2011 Valerio Pilo
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef ERRORS_H
#define ERRORS_H



class Errors
{
  public:

    enum ErrorCode
    {
      Error_None
    , Error_Invalid_Address
    , Error_Socket_Init
    , Error_Socket_Option
    , Error_Socket_Bind
    , Error_Socket_Listen
    , Error_Socket_Connection
    };


    enum StatusCode
    {
      Status_Ok                        = 200
    , Status_ChattingAlone
    , Status_NickNameAlreadyRegistered
    };


};



#endif // ERRORS_H
