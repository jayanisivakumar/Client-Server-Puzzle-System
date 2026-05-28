/**
  @file server.c
  @author Jayani Sivakumar (jsivaku)
    
  This program implements the server for the sliding puzzle assignment.
  The server reads a puzzle state from a file, listens for client commands
  on a POSIX message queue. Executes commands to slide rows or columns of
  the puzzle, or to display the current state. Sends replies back to the
  client queue.
*/
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <mqueue.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include "common.h"

/** Minimum puzzle dimension allowed. */
#define MIN_SIZE 2

/** Maximum puzzle dimension allowed. */
#define MAX_SIZE 8

/** Max characters for parsed command word. */
#define CMD_LEN 16

/** Expected number of command-line arguments (program + puzzle file). */
#define EXPECTED_ARGS 2

/** Buffer size for formatting a single puzzle cell */
#define CELL_BUF_SIZE 8

/** Permissions for message queues */
#define QUEUE_PERMS 0600

// Flag for telling the server to stop running because of a sigint.
// This is safer than trying to print in the signal handler.
static volatile int running = 1;

// puzzle state struct
typedef struct {
  // dimension of puzzle
  int size;
  // puzzle values
  int grid[ GRID_SIZE ][ GRID_SIZE ];
} PuzzleState;

// state of puzzle
static PuzzleState state;

/**
  Print an error message and exit with failure.
  
  @param message error string to print
*/
static void fail( char const *message ) {
  fprintf( stderr, "%s\n", message );
  exit( 1 );
}

/**
  Print the puzzle to stdout with borders.
*/
static void print_puzzle() {
  int n = state.size;
  // top border
  for ( int j = 0; j < n; j++ )
    printf( "+--" );
  printf( "+\n" );

  // inner rows
  for ( int i = 0; i < n; i++ ) {
    for ( int j = 0; j < n; j++ ) {
      printf( "|%2d", state.grid[ i ][ j ] );
    }
    printf( "|\n" );
  }

  // bottom border
  for ( int j = 0; j < n; j++ )
    printf( "+--" );
  printf( "+\n" );
}

/**
  Handle SIGINT by stopping the main loop and printing puzzle.
  
  @param sig signal number
*/
static void signal_handler( int sig ) {
  running = 0;
  printf( "\n" );
  print_puzzle();
}

/**
  Read puzzle state from an input file.
  Exits if file is invalid, puzzle size is out of range,
  numbers are out of range, duplicated, or extra numbers exist.
  
  @param filename path to puzzle input file
*/ 
void read_input_file( const char* filename ) {
  // opening file
  FILE *fp = fopen( filename, "r" );
  if ( !fp ) {
    fprintf( stderr, "Invalid input file: %s\n", filename );
    exit( 1 );
  }
  
  // first number denotes size of puzzle
  if ( fscanf( fp, "%d", &state.size ) != 1 || state.size < MIN_SIZE || state.size > MAX_SIZE ) {
    fprintf( stderr, "Invalid input file: %s\n", filename );
    fclose( fp );
    exit( 1 );
  }
  
  // reading integers from puzzle's dimensions
  bool seen[ GRID_SIZE*GRID_SIZE + 1 ];
  memset( seen, 0, sizeof( seen ) );
  
  // reading in the numbers of puzzle
  for ( int i = 0; i < state.size; i++ ) {
    for ( int j = 0; j < state.size; j++ ) {
      int val;
      if ( fscanf( fp, "%d", &val ) != 1 ) {
        fprintf( stderr, "Invalid input file: %s\n", filename );
        fclose( fp );
        exit( 1 );
      }
      if ( val < 1 || val > state.size*state.size || seen[ val ] ) {
        fprintf( stderr, "Invalid input file: %s\n", filename );
        fclose( fp );
        exit( 1 );
      }
      state.grid[ i ][ j ] = val;
      seen[ val ] = true;
    }
  }
  
  // checking for extra numbers
  int extra;
  if ( fscanf( fp, "%d", &extra ) == 1 ) {
    fprintf( stderr, "Invalid input file: %s\n", filename );
    fclose( fp );
    exit( 1 );
  }

  fclose( fp );
}

/**
  Slide row left by one position.
  
  @param row row index
*/
static void slide_row_left( int row ) {
  int n = state.size;
  int first = state.grid[ row ][ 0 ];
  for ( int j = 0; j < n-1; j++ ) {
    state.grid[ row ][ j ] = state.grid[ row ][ j+1 ];
  }
  state.grid[ row ][ n-1 ] = first;
}

/**
  Slide row right by one position.
 
  @param row row index
*/
static void slide_row_right( int row ) {
  int n = state.size;
  int last = state.grid[ row ][ n-1 ];
  for ( int j = n-1; j > 0; j-- ) {
    state.grid[ row ][ j ] = state.grid[ row ][ j-1 ];
  }
  state.grid[ row ][ 0 ] = last;
}

/**
  Slide column up by one position.
 
  @param col column index
*/
static void slide_col_up( int col ) {
  int n = state.size;
  int first = state.grid[ 0 ][ col ];
  for ( int i = 0; i < n-1; i++ ) {
    state.grid[ i ][ col ] = state.grid[ i+1 ][ col ];
  }
  state.grid[ n-1 ][ col ] = first;
}

/**
  Slide column down by one position.
 
  @param col column index
*/
static void slide_col_down( int col ) {
  int n = state.size;
  int last = state.grid[ n-1 ][ col ];
  for (int i = n-1; i > 0; i--) {
    state.grid[ i ][ col ] = state.grid[ i-1 ][ col ];
  }
  state.grid[ 0 ][ col ] = last;
}

/**
  Check if the puzzle is solved in increasing order.
 
  @return true if solved, else false
*/
static bool is_solved() {
  int expected = 1;
  int n = state.size;
  for ( int i = 0; i < n; i++ ) {
    for ( int j = 0; j < n; j++ ) {
      if ( state.grid[ i ][ j ] != expected )
        return false;
      expected++;
    }
  }
  return true;
}

/**
  Build string of puzzle for "show" command.
 
  @param out destination buffer
  @param outsize buffer size
*/
static void puzzle_string( char *out, size_t outsize ) {
  int n = state.size;
  out[ 0 ] = '\0';

  // top border
  for ( int j = 0; j < n; j++ ) {
    strncat( out, "+--", outsize - strlen(out) - 1 );
  }
  strncat( out, "+\n", outsize - strlen(out) - 1 );

  // inner rows
  for ( int i = 0; i < n; i++ ) {
    for ( int j = 0; j < n; j++ ) {
      char cell[ CELL_BUF_SIZE ];
      snprintf( cell, sizeof( cell ), "|%2d", state.grid[ i ][ j ] );
      strncat( out, cell, outsize - strlen( out ) - 1 );
    }
    strncat( out, "|\n", outsize - strlen( out ) - 1 );
  }

  // bottom border
  for ( int j = 0; j < n; j++ ) {
    strncat( out, "+--", outsize - strlen( out ) - 1 );
  }
  strncat( out, "+\n", outsize - strlen( out ) - 1 );
}

/**
  Main program that opens message queues, processes client commands, 
  and handles SIGINT.

  @param argc number of command-line arguments
  @param argv argument vector
  @return exit status
*/
int main( int argc, char *argv[] ) {
  char buffer[ MESSAGE_LIMIT ];
  char reply[ MESSAGE_LIMIT ];
  if ( argc != EXPECTED_ARGS ) {
    fprintf( stderr, "usage: server PUZZLE-FILE\n" );
    exit( 1 );
  }
  
  // retrieving puzzle from file
  read_input_file( argv[ 1 ] );
  
  // SIGINT handler
  struct sigaction sa;
  sa.sa_handler = signal_handler;
  sigemptyset( &sa.sa_mask );
  sa.sa_flags = 0;
  sigaction( SIGINT, &sa, NULL );
  
  // Remove both queues, in case, last time, this program terminated
  // abnormally with some queued messages still queued.
  mq_unlink( SERVER_QUEUE );
  mq_unlink( CLIENT_QUEUE );

  // Prepare structure indicating maximum queue and message sizes.
  struct mq_attr attr;
  attr.mq_flags = 0;
  attr.mq_maxmsg = 1;
  attr.mq_msgsize = MESSAGE_LIMIT;

  // Make both the server and client message queues.
  mqd_t serverQueue = mq_open( SERVER_QUEUE, O_RDONLY | O_CREAT, QUEUE_PERMS, &attr );
  mqd_t clientQueue = mq_open( CLIENT_QUEUE, O_WRONLY | O_CREAT, QUEUE_PERMS, &attr );
  if ( serverQueue == -1 || clientQueue == -1 )
    fail( "Can't create the needed message queues" );

  // Repeatedly read and process client messages.
  while ( running ) {
    ssize_t len = mq_receive( serverQueue, buffer, sizeof ( buffer ), NULL );
    if ( len < 0 ) {
      // signal interrupt
      if ( errno == EINTR ) {
        continue;
      }
      fail( "mq_receive failed" );
    }
    // null terminate
    buffer[ len ] = '\0';
    
    strcpy( reply, "Error: unknown command" );

    // parsing through command
    char cmd[ CMD_LEN ];
    int idx;
    int n = state.size;
    if ( sscanf( buffer, "%15s %d", cmd, &idx ) >= 1 ) {
      if ( strcmp( cmd, "show" ) == 0 ) {
        puzzle_string( reply, sizeof( reply ) );
      } else if ( strcmp( cmd, "left" ) == 0 ) {
        if ( idx >= 1 && idx <= n ) {
          slide_row_left( idx-1 );
          strcpy( reply, is_solved() ? "Solved" : "OK" );
        } else {
          strcpy(reply, "Error");
        }
      } else if ( strcmp( cmd, "right" ) == 0 ) {
        if ( idx >= 1 && idx <= n ) {
          slide_row_right( idx-1 );
          strcpy( reply, is_solved() ? "Solved" : "OK" );
          } else {
            strcpy(reply, "Error");
          }
      } else if ( strcmp( cmd, "up" ) == 0 ) {
        if (idx >= 1 && idx <= n ) {
          slide_col_up( idx-1 );
          strcpy( reply, is_solved() ? "Solved" : "OK" );
        } else {
          strcpy(reply, "Error");
        }
      } else if ( strcmp( cmd, "down" ) == 0 ) {
        if ( idx >= 1 && idx <= n ) {
          slide_col_down( idx-1 );
          strcpy( reply, is_solved() ? "Solved" : "OK" );
        } else {
          strcpy(reply, "Error");
        }
      }
    }

    // send the reply
    mq_send( clientQueue, reply, strlen( reply )+1, 0 );

  }

  // Close our two message queues (and delete them).
  mq_close( clientQueue );
  mq_close( serverQueue );

  mq_unlink( SERVER_QUEUE );
  mq_unlink( CLIENT_QUEUE );

  return 0;
}

