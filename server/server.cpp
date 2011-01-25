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

#include "chatmessage.h"
#include "filedatamessage.h"
#include "filetransfermessage.h"
#include "nicknamemessage.h"
#include "statusmessage.h"
#include "common.h"
#include "errors.h"
#include "sessionclient.h"

#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>

#include <algorithm>



Server::Server()
: connectionsCounter_( 0 )
, fileTransferModeActive_( false )
, listenThread_( 0 )
{
  int result = pthread_mutex_init( &accessMutex_, NULL );
  if( result != 0 )
  {
    Common::fatal( "Server mutex creation failed: error %d", result );
  }
}



Server::~Server()
{
  while( sessions_.size() > 0 )
  {
    std::map<SessionClient*,SessionData*>::iterator it;
    for( it = sessions_.begin(); it != sessions_.end(); it++ )
    {
      SessionData* session = (*it).second;
      // Tell the session to disconnect; it will auto-delete itself and end its thread
      session->client->disconnect();
      pthread_join( session->thread, NULL );
    }
  }

  pthread_cancel( listenThread_ );
  pthread_mutex_destroy( &accessMutex_ );
}



void Server::addSession( int newSocket )
{
  connectionsCounter_++;

  // The client session will take care of the socket and free it up when done.
  // It will also self-destruct when not needed anymore.

  SessionData* newSession = new SessionData;
  newSession->client = new SessionClient( this, newSocket );
  newSession->state = CLIENT_STATE_START;
  newSession->isFileTransferSender = false;

  // Assign a default unique name to the client
  char nickName[ MAX_NICKNAME_SIZE ];
  sprintf( nickName, "User %d", connectionsCounter_ );
  newSession->client->setNickName( nickName );

  pthread_create( &newSession->thread, NULL, &SessionClient::pollForData, newSession->client );

  pthread_mutex_lock( &accessMutex_ );
  sessions_[ newSession->client ] = newSession;
  pthread_mutex_unlock( &accessMutex_ );

  Common::debug( "Session \"%s\" registered, %lu active", nickName, sessions_.size() );
}



void Server::checkSessionStateChange( SessionClient* client, Message::Type messageType )
{
  SessionData* current = findSession( client );
  if( ! current )
  {
    Common::fatal( "Received a message from unknown session 0x%X!", client );
  }

  /*
   * How does this work:
   * Each message which changes the client state can be received when
   * the client is in a certain state; for example, clients can't send HELLO when
   * they have already authenticated.
   * The list contains all the states in which a client can be if it wants to
   * send a certain message.
   */
  std::list<ClientState> expectedStates;
  ClientState nextState;

  switch( messageType )
  {
    case Message::MSG_HELLO:
      expectedStates.push_back( CLIENT_STATE_START );
      nextState = CLIENT_STATE_IDENTIFY;
      break;
    case Message::MSG_NICKNAME:
      expectedStates.push_back( CLIENT_STATE_IDENTIFY );
      expectedStates.push_back( CLIENT_STATE_READY );
      nextState = CLIENT_STATE_READY;
      break;
    case Message::MSG_BYE:
      expectedStates.push_back( CLIENT_STATE_START );
      expectedStates.push_back( CLIENT_STATE_IDENTIFY );
      expectedStates.push_back( CLIENT_STATE_READY );
      nextState = CLIENT_STATE_INVALID;
      break;

    default:
      // The message doesn't change the session state.
      return;
  }

// States: CLIENT_STATE_INVALID CLIENT_STATE_START CLIENT_STATE_IDENTIFY CLIENT_STATE_READY CLIENT_STATE_END

  if( std::find( expectedStates.begin(), expectedStates.end(), current->state ) == expectedStates.end() )
  {
    Common::error( "Session \"%s\" sent a wrong state message of type %d", client->nickName(), messageType );
    client->disconnect();
    return;
  }

  if( current->state == nextState )
  {
    return;
  }

  current->state = nextState;
  Common::debug( "Session \"%s\" changed state to %d", client->nickName(), nextState );
}



bool Server::clientChangedNickName( SessionClient* client, const NicknameMessage* message )
{
  SessionData* current = findSession( client );
  if( ! current )
  {
    Common::fatal( "Received a message from unknown session 0x%X!", client );
  }

  // Remove non-printable chars from the name
  const char* newNickName = message->nickName();
  char verifiedNickName[ MAX_NICKNAME_SIZE ];
  for( uint i = 0; i <= strlen( newNickName ); i++ )
  {
    verifiedNickName[ i ] = isprint( newNickName[ i ] ) ? newNickName[ i ] : ' ';
  }

  // Check if the new name is unique
  std::map<SessionClient*,SessionData*>::iterator it;
  for( it = sessions_.begin(); it != sessions_.end(); it++ )
  {
    const SessionClient* peer = (*it).first;

    if( peer == client )
    {
        continue;
    }

    if( strcasecmp( verifiedNickName, peer->nickName() ) == 0 )
    {
      return false;
    }
  }

  Common::debug( "Session \"%s\" is now known as \"%s\"", current->client->nickName(), newNickName );

  return true;
}



bool Server::clientSentChatMessage( SessionClient* client, const ChatMessage* message )
{
  SessionData* current = findSession( client );
  if( ! current )
  {
    Common::fatal( "Received a message from unknown session 0x%X!", client );
  }

  // The user is alone by him/herself in chat
  if( sessions_.size() == 1 )
  {
    return false;
  }

  const char* chatMessage = message->message();
  const char* sender = client->nickName();

  Common::debug( "Session \"%s\" sent message \"%s\"", sender, chatMessage );

  // Send the same message to everybody but the sender
  std::map<SessionClient*,SessionData*>::iterator it;
  for( it = sessions_.begin(); it != sessions_.end(); it++ )
  {
    SessionClient* peer = (*it).first;

    // Don't send back the same message
    if( peer == client )
    {
      continue;
    }

    ChatMessage* newMessage = new ChatMessage( chatMessage );
    newMessage->setSender( sender );
    peer->sendMessage( newMessage );
  }

  return true;
}



void Server::clientSentFileData( SessionClient* client, const FileDataMessage* message )
{
  SessionData* current = findSession( client );
  if( ! current )
  {
    Common::fatal( "Received a message from unknown session 0x%X!", client );
  }

  // The user is alone by him/herself in chat
  if( sessions_.size() == 1 )
  {
    return;
  }

  // Send the same message to everybody but the sender
  std::map<SessionClient*,SessionData*>::iterator it;
  for( it = sessions_.begin(); it != sessions_.end(); it++ )
  {
    SessionClient* peer = (*it).first;

    // Don't send back the same message
    if( peer == client )
    {
      continue;
    }

    // Don't send the message to a client who refused the file transfer
    if( peer->fileTransferAccepted() != Errors::Status_AcceptFileTransfer )
    {
      continue;
    }

    FileDataMessage* dataMessage = new FileDataMessage();
    dataMessage->setBuffer( message->buffer(), message->bufferSize() );
    dataMessage->setFileOffset( message->fileOffset() );
    if( message->isLastBlock() )
    {
      dataMessage->markLastBlock();
    }

    peer->sendMessage( dataMessage );
  }

  if( message->isLastBlock() )
  {
    current->isFileTransferSender = false;
    fileTransferModeActive_ = false;
  }
}



bool Server::clientSentFileTransferRequest( SessionClient* client, const FileTransferMessage* message )
{
  SessionData* current = findSession( client );
  if( ! current )
  {
    Common::fatal( "Received a message from unknown session 0x%X!", client );
  }

  // The user is alone by him/herself in chat
  if( sessions_.size() == 1 )
  {
    return false;
  }

  // Enable the file transfer
  fileTransferModeActive_ = true;

  const char* filePath = message->fileName();
  const char* fileName = basename( filePath );
  const char* sender = client->nickName();

  Common::debug( "Session \"%s\" wants to send file \"%s\"", sender, fileName );

  // Send the same message to everybody but the sender
  std::map<SessionClient*,SessionData*>::iterator it;
  for( it = sessions_.begin(); it != sessions_.end(); it++ )
  {
    SessionClient* peer = (*it).first;

    // Don't send back the same message
    if( peer == client )
    {
      continue;
    }

    FileTransferMessage* transferMessage = new FileTransferMessage( fileName );
    transferMessage->setSender( sender );
    peer->sendMessage( transferMessage );
  }

  // Allow to identify the initial sender
  current->isFileTransferSender = true;

  return true;
}



bool Server::clientSentFileTransferResponse( SessionClient* client, bool accept )
{
  SessionData* current = findSession( client );
  if( ! current )
  {
    Common::fatal( "Received a message from unknown session 0x%X!", client );
  }

  // The user is alone by him/herself in chat
  if( sessions_.size() == 1 )
  {
    return false;
  }

  Common::debug( "Checking whether the transfer can be started" );

  // If all clients have confirmed, allow the sender to start transmitting the file
  bool allClientsConfirmed = true;
  bool canStart = false;
  SessionClient* sender = 0;

  std::map<SessionClient*,SessionData*>::iterator it;
  for( it = sessions_.begin(); it != sessions_.end(); it++ )
  {
    SessionClient* peer = (*it).first;
    SessionData* peerData = (*it).second;

    // Don't send back the same message
    if( peer == client )
    {
      continue;
    }

    if( peer->fileTransferAccepted() == Errors::Status_FileTransferCanceled )
    {
      allClientsConfirmed = false;
      break;
    }

    if( peer->fileTransferAccepted() == Errors::Status_AcceptFileTransfer )
    {
      canStart = true;
    }

    if( peerData->isFileTransferSender )
    {
      sender = peer;
    }
  }

  Common::debug( "All clients have confirmed? %s. Can the FT start? %s. Is sender set? %s",
                 allClientsConfirmed ? "yes" : "no",
                 canStart ? "yes" : "no",
                 sender ? "yes" : "no" );

  // Send the file transfer initiator the OK to send the file,
  // if everybody has answered the request and at least one has said yes
  if( allClientsConfirmed && sender )
  {
    Errors::StatusCode code;
    if( canStart )
    {
      code = Errors::Status_AcceptFileTransfer;
    }
    else
    {
      code = Errors::Status_RejectFileTransfer;
    }
    sender->sendMessage( new StatusMessage( code ) );
  }

  return true;
}



Server::SessionData* Server::findSession( SessionClient* client )
{
  std::map<SessionClient*,SessionData*>::iterator it = sessions_.find( client );

  if( it == sessions_.end() )
  {
    return NULL;
  }

  return (*it).second;
}



Errors::ErrorCode Server::initialize( const char* address, const int port )
{
  listenSocket_ = socket( AF_INET, SOCK_STREAM, 0 );
  if( listenSocket_ == -1 )
  {
    return Errors::Error_Socket_Init;
  }

  int yes = 1;
  int result;
  result = setsockopt( listenSocket_, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof( int ) );
  if( result == -1 )
  {
    return Errors::Error_Socket_Option;
  }

  // Convert the IP string to an usable binary address
  in_addr listenAddr;
  if( inet_aton( address, &listenAddr ) == 0 )
  {
    return Errors::Error_Invalid_Address;
  }

  // Set up the address where to listen
  sockaddr_in myAddress;
  myAddress.sin_family = AF_INET;
  myAddress.sin_port = htons( port );
  myAddress.sin_addr.s_addr = listenAddr.s_addr;
  memset( &( myAddress.sin_zero ), '\0', 8 );
  Common::debug( "Will listen on %s:%d", address, port );

  // Listen on the specified address and port
  result = bind( listenSocket_, reinterpret_cast<sockaddr*>( &myAddress ), sizeof( sockaddr ) );
  if( result == -1 )
  {
    return Errors::Error_Socket_Bind;
  }
  result = listen( listenSocket_, MAX_NETWORK_MESSAGE_QUEUE );
  if( result == -1 )
  {
    return Errors::Error_Socket_Listen;
  }

  // Start accepting connections
  pthread_create( &listenThread_, NULL, &Server::waitConnections, this );

  return Errors::Error_None;
}



bool Server::isFileTransferModeActive()
{
  return fileTransferModeActive_;
}



void Server::removeSession( SessionClient* client )
{
  pthread_mutex_lock( &accessMutex_ );

  SessionData* current = findSession( client );

  if( ! current )
  {
    Common::fatal( "Received a session state change from unknown session 0x%X!", client );
  }

  // If the client was sending out a file, cancel the transfer for everybody else
  if( fileTransferModeActive_ && current->isFileTransferSender )
  {
    fileTransferModeActive_ = false;

    std::map<SessionClient*,SessionData*>::iterator it;
    for( it = sessions_.begin(); it != sessions_.end(); it++ )
    {
      SessionClient* peer = (*it).first;

      if( peer == client )
      {
        continue;
      }

      client->sendMessage( new StatusMessage( Errors::Status_FileTransferCanceled ) );
    }
  }
  sessions_.erase( client );

  Common::debug( "Session \"%s\" ended, %lu remaining", current->client->nickName(), sessions_.size() );

  delete current;

  pthread_mutex_unlock( &accessMutex_ );

  // We won't delete the SessionClient, it does so by itself
}



void* Server::waitConnections( void* thisPointer )
{
  // Get access to the calling instance
  Server* self = static_cast<Server*>( thisPointer );

  sockaddr_in remote;
  socklen_t addressSize = sizeof( sockaddr_in );
  int newConnection;

  Common::debug( "Server is now accepting connections" );

  while( true )
  {
    newConnection = accept( self->listenSocket_,
                            reinterpret_cast<sockaddr*>( &remote ),
                            &addressSize );
    char* remoteAddress = inet_ntoa( remote.sin_addr );
    if( newConnection == -1 )
    {
      Common::error( "Unable to connect to %s:%d", remoteAddress, remote.sin_port );
      continue;
    }

    Common::debug( "Incoming connection from %s:%d", remoteAddress, remote.sin_port );

    self->addSession( newConnection );
  }

  return NULL; // Unused value
}

