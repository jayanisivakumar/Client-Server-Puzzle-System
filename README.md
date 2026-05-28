# Client-Server Puzzle System

A multi-threaded C client-server application that uses POSIX message queues to process puzzle-solving requests between multiple clients and a central server.

This project demonstrates systems programming concepts such as inter-process communication, concurrent request handling, structured client/server design, and low-level error handling in C.

## Overview

The system is built around a server process that receives puzzle-related requests from client processes. Clients submit requests through POSIX message queues, and the server processes each request before returning the appropriate response.

The project focuses on building a reliable request/response system where communication between processes is structured, predictable, and fault-aware.

## Features

- Client-server architecture written in C
- POSIX message queue-based communication
- Multi-client request handling
- Shared header file for common constants and message structures
- Puzzle input files for testing request behavior
- Separate client and server programs
- Modular source files for easier organization
- Makefile-based build workflow, if included

## Tech Stack

- C
- POSIX IPC
- POSIX Message Queues
- Multithreading / Concurrent Request Handling
- Linux / Unix Development
- Command-Line Programming

## Repository Structure

```text
Client-Server-Puzzle-System/
├── README.md
├── client.c
├── server.c
├── letterman.c
├── common.h
├── interleavings.txt
├── list-1.txt
├── list-2.txt
├── list-3.txt
├── list-4.txt
├── problem1.txt
├── puzzle-2.txt
├── puzzle-3.txt
├── puzzle-4.txt
├── puzzle-5.txt
├── puzzle-6.txt
└── rec_steps_q4.txt
