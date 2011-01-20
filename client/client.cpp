/**
 * LAN Messenger
 * Copyright © 2011 Valerio Pilo
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "client.h"

#include "common.h"
#include "errors.h"
#include "sessionserver.h"

#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <curses.h>
#include <netdb.h>
#include <semaphore.h>
#include <string.h>
#include <unistd.h>
#include <chatmessage.h>
#include <ctype.h>


/**
 * @def KEY_ESC
 * NCurses doesn't have a define for the ESC key
 */
#define KEY_ESC   27

/**
 * @def KEY_ENTER
 * NCurses doesn't have a define for the ENTER key
 */
#define KEY_RETURN   10

/**
 * @def STATUS_MESSAGE_TIMEOUT
 * Hide status messages after this amount of time
 */
#define STATUS_MESSAGE_TIMEOUT   5


// Semaphore used to end the session
sem_t sessionEndSignal;



Client::Client()
: connection_( NULL )
, connectionThread_( 0 )
, currentMessagePos_( 0 )
, maxX_( 0 )
, maxY_( 0 )
{
  sem_init( &sessionEndSignal, 0, 0 );

  // Enable ncurses
  initscr();
  noecho();
  halfdelay( 5 ); // 5 tenths of second timeout for character input
  keypad( stdscr, true );
  curs_set( 1 );
  raw(); // Disable Ctrl-C
}



Client::~Client()
{
  updateView();

  // Disable ncurses
  noraw(); // Re-enable Ctrl-C
  nocbreak();
  echo();
  keypad( stdscr, false );
  endwin();

  if( connectionThread_ != 0 )
  {
    pthread_cancel( connectionThread_ );
  }

  if( connection_ )
  {
    delete connection_;
  }

  close( socket_ );
}



bool Client::askQuestion( const char* question, char* answer )
{
  Common::debug( "Asking question: \"%s\"", question );

  bool done = false;
  const int cursorPos = strlen( question ) + 1; // Add a space after the question
  int pos = 0;

  mvaddstr( maxY_, 0, question );

  while( ! done )
  {
    wmove( stdscr, maxY_, pos + cursorPos );
    refresh();

    // Get the typed character, if is there any
    int ch = getch();

    // No characters have been typed
    if( ch < 0 )
    {
      Common::debug( "Got invalid char '%d'", ch );
      continue;
    }

    // If we've been disconnected, the pointer will be null
    if( ! connection_ )
    {
      pos = 0;
      done = true;
      continue;
    }

    switch( ch )
    {
      case KEY_ESC:
        Common::debug( "Got ESC, stopping asking question" );
        done = true;
        pos = 0;
        answer[ pos ] = '\0';
        break;

      case KEY_RETURN:
        answer[ pos ] = '\0';
        Common::debug( "Enter pressed, stopping asking question. Answer: \"%s\"", answer );
        done = true;
        break;

      default:
        answer[ pos++ ] = ch;
        addch( ch );
    }
  }

  if( pos == 0 )
  {
    return false;
  }

  return true;
}



void Client::changeStatusMessage( const char* message, bool permanent )
{
  memset( statusMessage_, '\0', MAX_MESSAGE_SIZE );
  strncpy( statusMessage_, message, MAX_MESSAGE_SIZE );

  if( permanent )
  {
    statusMessageTime_ = 0;
  }
  else
  {
    statusMessageTime_ = time( NULL );
  }
}



void Client::connectionClosed( SessionServer* connection )
{
  if( connection != connection_ )
  {
    Common::fatal( "Unknown connection was closed!" );
  }

  connection_ = 0;

  Common::debug( "Connection closed" );

  sem_post( &sessionEndSignal );
}



void Client::gotChatMessage( const char* sender, const char* message )
{
  Row* row = new Row();
  row->incoming = true;

  memset( row->sender, '\0', MAX_NICKNAME_SIZE );
  strncpy( row->sender, sender, MAX_NICKNAME_SIZE );
  memset( row->message, '\0', MAX_MESSAGE_SIZE );
  strncpy( row->message, message, MAX_MESSAGE_SIZE );
  row->dateTime = time( NULL );

  if( chatHistory_.size() > HISTORY_SIZE )
  {
    delete chatHistory_.front();
    chatHistory_.pop_front();
  }

  chatHistory_.push_back( row );

  updateView();
}



void Client::gotNicknameChange( const char* nickName )
{
  updateView();
}



Errors::ErrorCode Client::initialize( const in_addr serverIp, const int serverPort )
{
  changeStatusMessage( "Connecting...", true );

  socket_ = socket( AF_INET, SOCK_STREAM, 0 );
  if( socket_ == -1 )
  {
    Common::error( "Socket initialization failure" );
    return Errors::Error_Socket_Init;
  }

  sockaddr_in serverAddress;
  serverAddress.sin_family = AF_INET;
  serverAddress.sin_addr = serverIp;
  serverAddress.sin_port = htons( serverPort );
  memset( &(serverAddress.sin_zero), '\0', 8 );

  Common::debug( "Connecting to %s:%d...", inet_ntoa( serverIp ), serverPort );

  int result;
  result = connect( socket_, reinterpret_cast<sockaddr*>( &serverAddress ), sizeof( sockaddr ) );
  if( result == -1 )
  {
    Common::error( "Connection failure" );
    return Errors::Error_Socket_Connection;
  }

  connection_ = new SessionServer( this, socket_ );

  pthread_create( &connectionThread_, NULL, &SessionServer::pollForData, connection_ );

  Common::debug( "Connection established" );

  updateView();

  return Errors::Error_None;
}



void Client::run()
{
  bool quit = false;

  while( ! quit )
  {
    // Get the typed character, if is there any
    int ch = getch();

    // Debugging print of the keypress
    char str[ 32 ];
    sprintf(str, "Keypress %i, '%c'", ch, isprint( ch ) ? ch : '?' );
    mvaddstr( maxY_ - 2, maxX_- strlen( str ), str );

    wmove( stdscr, maxY_, currentMessagePos_ );
    refresh();

    // No characters have been typed
    if( ch < 0 )
    {
      Common::debug( "Got invalid char '%d'", ch );
      continue;
    }

    // If we've been disconnected, the pointer will be null
    if( ! connection_ )
    {
      quit = true;
      continue;
    }

    switch( ch )
    {
      case KEY_ESC:
        Common::debug( "Got ESC, quitting" );
        connection_->disconnect();
        quit = true;
        break;

      case KEY_F( 2 ):
      {
        Common::debug( "Changing name..." );

        char newName[ MAX_NICKNAME_SIZE ];

        if( askQuestion( "Insert a new nickname:", newName ) && strlen( newName ) >= 1 )
        {
          connection_->setNickName( newName );
        }
        break;
      }

      case KEY_RETURN:
        Common::debug( "Enter pressed" );
        if( currentMessagePos_ == 0 )
        {
          break;
        }

        Common::debug( "Sending message..." );
        currentMessage_[ currentMessagePos_ ] = '\0';
        currentMessagePos_ = 0;
        sendChatMessage( currentMessage_ );
        updateView();
        break;

      default:
        currentMessage_[ currentMessagePos_++ ] = ch;
        addch( ch );
        break;
    }
  }

  // Quit
  if( connection_ )
  {
    connection_->disconnect();
  }

  // Wait for the connection to be closed, then return
  sem_wait( &sessionEndSignal );
}



void Client::sendChatMessage( const char* message )
{
  Row* row = new Row();
  row->incoming = false;
  row->sender[0] = '\0';

  memset( row->sender, '\0', MAX_NICKNAME_SIZE );
  memset( row->message, '\0', MAX_MESSAGE_SIZE );
  strncpy( row->message, message, MAX_MESSAGE_SIZE );
  row->dateTime = time( NULL );

  connection_->chat( message );

  if( chatHistory_.size() > HISTORY_SIZE )
  {
    delete chatHistory_.front();
    chatHistory_.pop_front();
  }

  chatHistory_.push_back( row );

  updateView();
}



void Client::updateView()
{
  getmaxyx( stdscr, maxY_, maxX_ );

  // Rows available for chat history
  const int chatHistoryFirstRow = 2;
  const int chatHistoryLastRow = maxY_ - 3;

  const int lineAbove = chatHistoryFirstRow - 1;
  const int lineBelow = chatHistoryLastRow + 1;

  char string[ MAX_MESSAGE_SIZE ];

  if( ( time( NULL ) - statusMessageTime_ ) > STATUS_MESSAGE_TIMEOUT )
  {
    // The previous status message has expired, change it with the default
    if( connectionThread_ != 0 && connection_ != 0 )
    {
      sprintf( statusMessage_, "In chat as %s", connection_->nickName() );
    }
    else
    {
      *string = '\0';
    }
    changeStatusMessage( string, true );
  }

  sprintf( string, "LAN Messenger - %s", statusMessage_ );
  mvaddstr( 0, 1, string );

  for( int i = 0; i < maxX_; i++ )
  {
    mvaddch( lineAbove, i, '=' );
    mvaddch( lineBelow, i, '=' );
  }


  // Display the messages history

  int currentRow = chatHistoryLastRow;
  std::deque<Row*>::reverse_iterator it;
  for( it = chatHistory_.rbegin(); it != chatHistory_.rend(); it++ )
  {
    Row* row = (*it);

    char dateTime[ 16 ];
    struct tm* timeinfo = localtime ( &( row->dateTime ) );

    strftime( dateTime, 16, "%H.%M", timeinfo );
    sprintf( string, "%s<%s> %s", dateTime, row->sender, row->message );
    mvaddnstr( currentRow--, 0, string, maxX_ );

    if( currentRow < chatHistoryFirstRow )
    {
      break;
    }
  }

  // Show the current typed message
  for( int i = 0; i < maxX_; i++ )
  {
    if( i < currentMessagePos_ )
    {
      mvaddch( maxY_, i, currentMessage_[ i ] );
    }
    else
    {
      mvaddch( maxX_, i, ' ' );
    }
  }

  refresh();
}


