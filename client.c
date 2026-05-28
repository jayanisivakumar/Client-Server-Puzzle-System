/**
  @file client.c
  @author Jayani Sivakumar (jsivaku)

  This program sends commands to the sliding puzzle server using
  POSIX message queues. The client builds a request string from its
  command-line arguments, sends the request to the server queue, waits
  for the server's reply on the client queue, and prints the reply.
*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <mqueue.h>
#include <errno.h>
#include "common.h"

/** Minimum number of arguments (program, command). */
#define MIN_ARGS 2

/** Maximum number of arguments (program, command, index). */
#define MAX_ARGS 3

/** Index of optional index argument. */
#define ARG_INDEX 2

/** Print a usage message and exit with failure. */
static void usage() {
  fprintf( stderr, "usage: client command [index]\n" );
  exit( 1 );
}

/**
  Main function for the client program.
  
  @param argc number of command-line arguments
  @param argv argument vectors
  @return 0 if successful, 1 if error
*/
int main( int argc, char *argv[] ) {
    
  // check if valid argument
  if ( argc < MIN_ARGS || argc > MAX_ARGS ) {
    usage();
  }

  // message string
  char message[ MESSAGE_LIMIT ];
  if ( argc == MIN_ARGS ) {
    // show command
    snprintf( message, sizeof( message ), "%s", argv[1] );
  } else {
    // command + index
    snprintf( message, sizeof( message ), "%s %s", argv[1], argv[ARG_INDEX] );
  }

  // message queues
  mqd_t serverQueue = mq_open( SERVER_QUEUE, O_WRONLY );
  mqd_t clientQueue = mq_open( CLIENT_QUEUE, O_RDONLY );
  if ( serverQueue == -1 || clientQueue == -1 ) {
    perror( "Can't open message queues" );
    exit( 1 );
  }

  // sending message to server
  if ( mq_send( serverQueue, message, strlen(message) + 1, 0 ) == -1 ) {
    perror( "mq_send failed" );
    exit( 1 );
  }
  
  // waiting for server's reply
  char reply[ MESSAGE_LIMIT ];
  ssize_t len = mq_receive( clientQueue, reply, sizeof(reply), NULL );
  if ( len < 0 ) {
    perror( "mq_receive failed" );
    exit( 1 );
  }
  // null terminate
  reply[ len ] = '\0';

  // server's reply
  printf( "%s\n", reply );
  
  mq_close( serverQueue );
  mq_close( clientQueue );

  return 0;
}

