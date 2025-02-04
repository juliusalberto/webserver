# HTTP Server Implementation Project

## Overview
This project implements a subset of the HTTP/1.1 protocol. The server (is planned to) handle basic HTTP requests, supports concurrent connections, and serves static files including HTML, JPEG, and PNG formats.

## Current Implementation Status

### Completed Components:
- [x] Basic server initialization and socket setup
- [x] Initial request parsing structure
- [] Complete request parsing implementation
- [] Response generation
- [] Response sending
- [] Connection timeout handling
- [] Threading/concurrency support
- [] Connection: close header handling
- [] Error response handling (400, 403, 404)
- [] Memory management and cleanup

## Building and Running

### Prerequisites
- POSIX-compliant system
- C/C++ compiler with C++11 support
- pthread library
- make

### Build Instructions
```bash
make
```

### Test Instructions
```bash
make test
```

### Running the Server
Can't run it yet lol


## Implementation Details
I have only implemented the HTTP parsing, and I did it using manual character parsing in C (which is a pain in the ass). 

## Testing
Currently implementing test cases for:
- Basic HTTP requests

## Attributes
The network utilities code (network_utils.c and network_utils.h) is adapted from the Computer Systems: A Programmer's Perspective (CS:APP) textbook.





