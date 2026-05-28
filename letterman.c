/**
  @file letterman.c
  @author Jayani Sivakumar (jsivaku)

  This program reads a list of words from stdin and finds all pairs of words
  that differ by exactly one letter. The work is divided among multiple
  worker processes, which report their findings to the parent process via a pipe.
*/
#include <unistd.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <limits.h>
#include <stdbool.h>

/** Maximum word length */
#define WORD_LEN 24

/** Initial capacity for word array. */
#define INIT_CAPACITY 1000

/** Default number of workers */
#define DEFAULT_WORKERS 4

/** Number of file descriptors returned by pipe(). */
#define PIPE_FDS 2

/** Minimum number of command-line arguments (program + workers). */
#define MIN_ARGS 2

/** Maximum number of command-line arguments (program + workers + report). */
#define MAX_ARGS 3

/** Factor by which the word array capacity grows when resized. */
#define GROWTH_FACTOR 2

/**
  Print an error message and exit.
  
  @param message error string
*/
static void fail( char const *message ) {
  fprintf( stderr, "%s\n", message );
  exit( 1 );
}

// Print out a usage message, then exit.
static void usage() {
  printf( "usage: letterman <workers>\n" );
  printf( "       letterman <workers> report\n" );
  exit( 1 );
}

// Type for a word of up to 23 letters.
typedef char Word[ WORD_LEN ];

/**
  Determine whether two words differ by exactly one letter.
 
  @param a first word
  @param b second word
  @return true if words are same length and differ in exactly one position
*/
static bool similar( const char *a, const char *b ) {
  size_t la = strlen( a );
  size_t lb = strlen( b );
  // words are not of the same length
  if ( la != lb ) {
    return false;
  }
  int different = 0;
  for ( size_t i = 0; i < la; i++ ) {
    if ( a[i] != b[i] ) {
      different++;
      // more than one letter differences
      if ( different > 1 ) {
        return false;
      }
    }
  }
  // exactly one letter different
  return ( different == 1 );
}

/**
  Main function for the letterman program. Parses command-line arguments,
  reads input words, forks worker processes to find similar pairs, and
  aggregates results from all workers.
  
  @param argc number of command-line arguments
  @param argv array of argument strings
  @return 0 on success, 1 on error
 */
int main( int argc, char *argv[] ) {
  bool report = false;
  int workers = DEFAULT_WORKERS;

  // Parse command-line arguments.
  if ( argc < MIN_ARGS || argc > MAX_ARGS )
    usage();

  if ( sscanf( argv[ 1 ], "%d", &workers ) != 1 ||
       workers < 1 )
    usage();

  // If there's a second argument, it better be the "report"
  if ( argc == MAX_ARGS ) {
    if ( strcmp( argv[ MAX_ARGS - 1 ], "report" ) != 0 )
      usage();
    report = true;
  }

  // You will need to add some code here.
  // initial capacity
  int capacity = INIT_CAPACITY;
  // numbers of words read
  int count = 0;
  Word *words = malloc( capacity * sizeof( Word ) );
  if ( !words ) {
    fail( "Out of memory" );
  }
    
  // reading until EOF
  while ( scanf( "%23s", words[ count ]) == 1 ) {
    count++;
    // check if we need to resize
    if ( count >= capacity ) {
      capacity *= GROWTH_FACTOR;
      words = realloc( words, capacity * sizeof(Word) );
      if ( !words ) {
        fail( "Out of memory" );
      }
    }
  }
    
  // create the pipe
  int pfd[ PIPE_FDS ];
  if ( pipe( pfd ) != 0 ) {
    fail( "Can't create pipe" );
  }
    
  // fork worker processes
  for ( int w = 0; w < workers; w++ ) {
    pid_t pid = fork();
    if ( pid < 0 ) {
      fail( "fork failed" );
    }

    if ( pid == 0 ) {
      // child process
      close( pfd[ 0 ] ); // no reading required  
      int lCount = 0;
      for ( int i = w; i < count; i += workers ) {
        for ( int j = 0; j < i; j++ ) {
          if ( similar( words[ i ], words[ j ] ) ) {
            lCount++;
            // if report flag present
            if ( report ) {
              printf( "Worker %d : %s <-> %s\n", w, words[ j ], words[ i ] );
              fflush( stdout );
            }
          }
        }
      }

      // report count to parent using our pipe
      // locking pipe
      lockf( pfd[ 1 ], F_LOCK, 0 );
      // writing count
      write( pfd[ 1 ], &lCount, sizeof(int) );
      // unlocking pipe
      lockf( pfd[ 1 ], F_ULOCK, 0 );

      close( pfd[ 1 ] );
      free( words );
      exit( 0 );
    }
  }

  // no writing required
  close( pfd[ 1 ] );

  int total = 0;
  int lCount;

  // reading child's result obtained form pipe
  for ( int w = 0; w < workers; w++ ) {
    if ( read( pfd[ 0 ], &lCount, sizeof( int ) ) == sizeof( int ) ) {
      total += lCount;
    }
  }

  // wait for children to be done
  while ( wait( NULL ) > 0 );
  printf( "Total pairs: %d\n", total );

  close( pfd[ 0 ] );
  free( words );

  return EXIT_SUCCESS;
}

