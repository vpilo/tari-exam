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
#include "chatmessage.h"
#include "errors.h"
#include "sessionserver.h"
#include "statusmessage.h"

#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <ctype.h>
#include <curses.h>
#include <errno.h>
#include <netdb.h>
#include <semaphore.h>
#include <string.h>
#include <unistd.h>


/**
 * @def KEYCODE_ESC
 * NCurses doesn't have a define for the ESC key
 */
#define KEYCODE_ESC   27

/**
 * @def KEYCODE_ENTER
 * NCurses doesn't have a define for the ENTER key
 */
#define KEYCODE_ENTER   10

/**
 * @def KEYCODE_BACKSPACE
 * NCurses doesn't have a define for the Backspace key
 */
#define KEYCODE_BACKSPACE   263

/**
 * @def KEYCODE_CANC
 * NCurses doesn't have a define for the Canc key
 */
#define KEYCODE_CANC   330

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
, socket_( 0 )
{
  int result = pthread_mutex_init( &inputMutex_, NULL );
  if( result != 0 )
  {
    Common::fatal( "Input mutex creation failed: error %d", result );
  }

  sem_init( &sessionEndSignal, 0, 0 );

  // Enable ncurses
  initscr();
  noecho();
  keypad( stdscr, true );
  notimeout( stdscr, true );
  curs_set( 1 );
  raw(); // Disable Ctrl-C

  changeStatusMessage( "Welcome!", true );
  updateView();
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

  std::deque<Row*>::iterator it;
  for( it = chatHistory_.begin(); it != chatHistory_.end(); it++ )
  {
    delete (*it);
  }

  pthread_mutex_destroy( &inputMutex_ );
}



bool Client::askQuestion( const char* question, char* answer, const int answerSize )
{
  Common::debug( "Asking question: \"%s\"", question );


  int pos = strlen( answer );
  bool done = false;
  const int cursorPos = strlen( question ) + 1; // Add a space after the question

  while( ! done )
  {
    mvaddstr( maxY_, 0, question );
    mvaddstr( maxY_, cursorPos, answer );
    mvaddch( maxY_, cursorPos - 1, ' ' );
    wmove( stdscr, maxY_, pos + cursorPos );
    clrtoeol();
    refresh();

    // Get the typed character, if is there any
    int ch = getch();

    // No characters have been typed
    if( ch <= 1 )
    {
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
      case KEYCODE_ESC:
        Common::debug( "Got ESC, stopping asking question" );
        done = true;
        pos = 0;
        answer[ pos ] = '\0';
        break;

      case KEYCODE_ENTER:
        answer[ pos ] = '\0';
        Common::debug( "Enter pressed, stopping asking question. Answer: \"%s\"", answer );
        done = true;
        break;

      case KEYCODE_BACKSPACE:
        if( pos == 0 )
        {
          break;
        }

        --pos;
        mvaddch( maxY_, pos, ' ' );
        answer[ pos ] = '\0';
        move( maxY_, pos );
        break;

      default:
        if( pos >= ( answerSize - 1 ) )
        {
            break;
        }

        answer[ pos++ ] = ch;
        addch( ch );
        break;
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
  memset( statusMessage_, '\0', MAX_CHATMESSAGE_SIZE );

  if( message != NULL )
  {
    strncpy( statusMessage_, message, MAX_CHATMESSAGE_SIZE );
  }

  if( permanent )
  {
    statusMessageTime_ = 0;
  }
  else
  {
    statusMessageTime_ = time( NULL );
  }

  updateView();
}



void Client::connectionClosed( SessionServer* connection )
{
  if( connection != connection_ )
  {
    Common::fatal( "Unknown connection was closed!" );
  }

  changeStatusMessage( "Disconnected! Bye!", true );

  connection_ = 0;

  Common::debug( "Connection closed" );

  sem_post( &sessionEndSignal );
}



void Client::gotChatMessage( const char* sender, const char* message )
{
  Row* row = new Row();
  row->incoming = true;

  memset( row->sender, '\0', MAX_NICKNAME_SIZE );
  if( strlen( sender ) > 0 )
  {
    strncpy( row->sender, sender, MAX_NICKNAME_SIZE );
  }
  memset( row->message, '\0', MAX_CHATMESSAGE_SIZE );
  strncpy( row->message, message, MAX_CHATMESSAGE_SIZE );
  row->dateTime = time( NULL );
  row->special = false;

  if( chatHistory_.size() > HISTORY_SIZE )
  {
    delete chatHistory_.front();
    chatHistory_.pop_front();
  }

  chatHistory_.push_back( row );

  changeStatusMessage();
  updateView();
}



bool Client::gotFileTransferRequest( const char* sender, const char* filename, char* targetFileName )
{
  // Make the run() loop to block while we're asking the user to accept or reject
  pthread_mutex_lock( &inputMutex_ );
  ungetch( 1 ); // force the run() loop to get to the lock

  char string[ MAX_CHATMESSAGE_SIZE ];
  gotStatusMessage( "Received a request to transfer \"%s\" from \"%s\"", filename, sender );

  sprintf( string, "Do you want to accept the file \"%s\" from \"%s\"? [Y/n]", filename, sender );

  bool accept = false;
  bool answered = false;
  char acceptStr[3];
  strcpy( acceptStr, "y" );

  while( ! answered )
  {
    if( ! askQuestion( string, acceptStr, 5 ) )
    {
      accept = false;
      break;
    }

    accept = ( strcasecmp( acceptStr, "y" ) == 0 );
    answered = ( accept || ( strcasecmp( acceptStr, "n" ) == 0 ) );
  }

  // Ask the path where to put the saved file
  if( accept )
  {
    // Put the sender's file name as the default answer
    strcpy( targetFileName, filename );

    answered = false;
    sprintf( string, "Please enter a name for the saved file:"  );
    if( ! askQuestion( string, targetFileName, MAX_PATH_SIZE ) )
    {
      accept = false;
    }
  }

  Errors::StatusCode status = ( accept ? Errors::Status_AcceptFileTransfer
                                       : Errors::Status_RejectFileTransfer );

  connection_->sendMessage( new StatusMessage( status ) );

  gotStatusMessage( "File transfer request %s.", accept ? "accepted" : "rejected" );

  changeStatusMessage();
  updateView();

  pthread_mutex_unlock( &inputMutex_ );
  return accept;
}



void Client::gotNicknameChange( const char* nickName )
{
  gotStatusMessage( "Your nickname is now \"%s\"", nickName );

  changeStatusMessage();
  updateView();
}



void Client::gotStatusMessage( const char* format, ... )
{
  char statusMessage[ MAX_CHATMESSAGE_SIZE ];
  va_list args;

  // Get all the parameters that have been passed to this function
  va_start( args, format );
  vsprintf( statusMessage, format, args );
  va_end( args );


  Row* row = new Row();
  row->incoming = true;
  row->special = true;

  memset( row->sender, '\0', MAX_NICKNAME_SIZE );
  strncpy( row->sender, "SERVER", MAX_NICKNAME_SIZE );
  strncpy( row->message, statusMessage, MAX_CHATMESSAGE_SIZE );
  row->dateTime = time( NULL );

  if( chatHistory_.size() > HISTORY_SIZE )
  {
    delete chatHistory_.front();
    chatHistory_.pop_front();
  }

  chatHistory_.push_back( row );

  changeStatusMessage();
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
  changeStatusMessage( "Connected, logging in...", true );

  return Errors::Error_None;
}



void Client::run()
{
  bool quit = false;

  while( ! quit )
  {
    // Get the typed character, if is there any
    int ch = getch();

    // Lock the loop while there's something else grabbing input
    int result = pthread_mutex_trylock( &inputMutex_ );
    if( result == EBUSY )
    {
      Common::debug( "run() - Waiting for input lock to end" );
      pthread_mutex_lock( &inputMutex_ );
      pthread_mutex_unlock( &inputMutex_ );
      Common::debug( "run() - Input lock ended" );
      continue;
    }
    else if( result == 0 )
    {
      pthread_mutex_unlock( &inputMutex_ );
    }

    // No characters have been typed
    if( ch <= 1 )
    {
      continue;
    }

    // Debugging print of the keypress
    char str[ 32 ];
    sprintf(str, "Keypress %i, '%c'", ch, isprint( ch ) ? ch : '?' );
    mvaddstr( maxY_ - 2, maxX_- strlen( str ), str );

    move( maxY_, currentMessagePos_ );
    refresh();

    // If we've been disconnected, the pointer will be null
    if( ! connection_ )
    {
      quit = true;
      continue;
    }

    switch( ch )
    {
      case KEYCODE_ESC:
        Common::debug( "Got ESC, quitting" );
        changeStatusMessage( "Quitting...", true );
        updateView();
        connection_->disconnect();
        quit = true;
        break;

      case KEY_F( 2 ):
      {
        Common::debug( "Changing name..." );

        char newName[ MAX_NICKNAME_SIZE ];
        strcpy( newName, connection_->nickName() );

        if( askQuestion( "Insert a new nickname:", newName, MAX_NICKNAME_SIZE ) && strlen( newName ) >= 1 )
        {
          connection_->setNickName( newName );
        }
        break;
      }

      case KEY_F( 4 ):
      {
        if( connection_->hasFileTransfer() )
        {
          gotStatusMessage( "A file transfer for \"%s\" is already in progress.", connection_->fileTransferName() );
          break;
        }

        Common::debug( "Sending file..." );

        char fileName[ MAX_PATH_SIZE ];
        memset( fileName, '\0', MAX_PATH_SIZE );

        if( askQuestion( "Choose a file name:", fileName, MAX_PATH_SIZE ) && strlen( fileName ) >= 1 )
        {
          connection_->sendFile( fileName );
        }

        gotStatusMessage( "Waiting for the other participants to answer the request." );
        break;
      }

      case KEYCODE_ENTER:
        Common::debug( "Enter pressed" );
        if( currentMessagePos_ <= 0 )
        {
          break;
        }

        currentMessage_[ currentMessagePos_ ] = '\0';
        Common::debug( "Sending message: %s (%d bytes)", currentMessage_, currentMessagePos_ );
        sendChatMessage( currentMessage_ );

        currentMessagePos_ = 0;

        updateView();
        break;

      case KEYCODE_BACKSPACE:
        if( currentMessagePos_ <= 0 )
        {
          break;
        }

        --currentMessagePos_;
        mvaddch( maxY_, currentMessagePos_, ' ' );
        currentMessage_[ currentMessagePos_ ] = '\0';
        move( maxY_, currentMessagePos_ );
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
  row->special = false;

  memset( row->sender, '\0', MAX_NICKNAME_SIZE );
  memset( row->message, '\0', MAX_CHATMESSAGE_SIZE );
  strncpy( row->sender, connection_->nickName(), MAX_NICKNAME_SIZE );
  strncpy( row->message, message, MAX_CHATMESSAGE_SIZE );
  row->dateTime = time( NULL );

  connection_->chat( message );

  if( chatHistory_.size() > HISTORY_SIZE )
  {
    delete chatHistory_.front();
    chatHistory_.pop_front();
  }

  chatHistory_.push_back( row );

  changeStatusMessage();
  updateView();
}



void Client::updateView()
{
  getmaxyx( stdscr, maxY_, maxX_ );

  // This way I don't have to use max - 1 to refer to the last line/col
  maxX_--;
  maxY_--;

  // Rows available for chat history
  const int chatHistoryFirstRow = 2;
  const int chatHistoryLastRow = maxY_ - 3;

  const int lineAbove = chatHistoryFirstRow - 1;
  const int lineBelow = chatHistoryLastRow + 1;

  for( int i = 0; i < maxX_; i++ )
  {
    // First line with status messages
    mvaddch( 0, i, ' ' );
    // Separators
    mvaddch( lineAbove, i, '=' );
    mvaddch( lineBelow, i, '=' );
  }

  char string[ MAX_CHATMESSAGE_SIZE ];

  // Don't expire the status message if it's permanent (value 0)
  if( statusMessageTime_ != 0 && ( time( NULL ) - statusMessageTime_ ) > STATUS_MESSAGE_TIMEOUT )
  {
    *statusMessage_ = '\0';
  }

  if( strlen( statusMessage_ ) < 1 )
  {
    // The previous status message has expired, change it with the default
    if( connectionThread_ != 0 && connection_ != 0 )
    {
      sprintf( statusMessage_, "In chat as %s", connection_->nickName() );
    }
    else
    {
      sprintf( statusMessage_, "Disconnected" );
    }
  }

  sprintf( string, "LAN Messenger - %s", statusMessage_ );
  mvaddstr( 0, 1, string );


  // Display the messages history

  int currentRow = chatHistoryLastRow;
  std::deque<Row*>::reverse_iterator it;
  for( it = chatHistory_.rbegin(); it != chatHistory_.rend(); it++ )
  {
    Row* row = (*it);

    struct tm* timeinfo = localtime( &( row->dateTime ) );

    char dateTime[ 16 ];
    strftime( dateTime, 16, "%H.%M", timeinfo );

    sprintf( string, "%s <%s> %s", dateTime, row->sender, row->message );

    if( row->special )
    {
      attron( A_BOLD );
    }

    mvaddnstr( currentRow--, 0, string, maxX_ );

    if( row->special )
    {
      attroff( A_BOLD );
    }

    clrtoeol();

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
      mvaddch( maxY_, i, ' ' );
    }
  }
  wmove( stdscr, maxY_, currentMessagePos_ );

  refresh();
}


