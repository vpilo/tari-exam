/**
 * LAN Messenger
 * Copyright © 2011 Valerio Pilo
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "server.h"
#include "common.h"
#include "errors.h"

#include <semaphore.h>
#include <signal.h>
#include <unistd.h>


// Semaphore used to quit
sem_t quitSignal;



void handleSignal( const int signal )
{
  // More signals are available at http://www.comptechdoc.org/os/linux/programming/linux_pgsignals.html
  switch( signal )
  {
    case SIGINT:
      Common::debug( "Interrupt signal catched!" );
      break;
    case SIGHUP:
      Common::debug( "Hangup signal catched!" );
      break;
    case SIGTERM:
      Common::debug( "Terminate signal catched!" );
      break;
    case SIGQUIT:
      Common::debug( "Quit signal catched!" );
      break;
    default:
      Common::debug( "Signal %d catched!", signal );
      break;
  }

  sem_post( &quitSignal );
}



/**
 * Server application entry point.
 */
int main( int argc, char* argv[] )
{
//   Common::setLogFile( "lanmessenger-server.log" );
  Common::debug( "LAN Messenger server" );

  Server* server = new Server();

  // Create a semaphore as a quit condition
  sem_init( &quitSignal, 0, 0 );

  // Handle signals
  signal( SIGCHLD,SIG_IGN );       // ignore child
  signal( SIGTSTP,SIG_IGN );       // ignore tty signals
  signal( SIGINT, handleSignal );  // interrupt signal
  signal( SIGHUP, handleSignal );  // hangup signal
  signal( SIGTERM, handleSignal ); // terminate signal
  signal( SIGQUIT, handleSignal ); // quit signal

  Errors::ErrorCode status = server->initialize( "0.0.0.0", SERVER_PORT );
  if( status != Errors::Error_None )
  {
    Common::error( "Server could not be started: error %d", status );

    delete server;
    return status;
  }

  // Wait for a quit signal to arrive
  sem_wait( &quitSignal );

  delete server;

  return Errors::Error_None;
}


