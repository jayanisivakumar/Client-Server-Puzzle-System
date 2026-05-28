/**
  @file common.h
  @author Jayani Sivakumar (jsivaku)

  This header defines constants shared by both the server and client
  in the sliding puzzle program. It specifies the names of the message
  queues, the maximum message size, and the maximum grid size allowed
  for the puzzle.
*/

// Name for the queue of messages going to the server.
#define SERVER_QUEUE "/jsivaku-server-queue"

// Name for the queue of messages going to the current client.
#define CLIENT_QUEUE "/jsivaku-client-queue"

// Maximum length for a message in the queue
// (Long enough to hold any server request or response)
#define MESSAGE_LIMIT 1024

// Maximum height and width of the grid of numbers.
#define GRID_SIZE 10
