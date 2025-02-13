# HTTP Server Implementation Project

## Overview
This project implements a subset of the HTTP/1.1 protocol. The server (is planned to) handle basic HTTP requests, supports concurrent connections, and serves static files including HTML, JPEG, and PNG formats.

## Screenshot
![Hello world screenshot](assets/webserver-ss.png)

## Current Implementation Status

### Completed Components:
- [x] Basic server initialization and socket setup
- [x] Initial request parsing structure
- [x] Complete request parsing implementation
- [x] Response generation
- [x] Response sending
- [ ] Connection timeout handling
- [x] Threading/concurrency support
- [ ] Connection: close header handling
- [x] Error response handling (400, 403, 404)
- [x] Memory management and cleanup

## Building and Running

### Prerequisites
- POSIX-compliant system
- C/C++ compiler with C++11 support
- pthread library
- make
- If you want to run http test, Poetry (in mac, you can install it by using `brew install poetry`)

### Build Instructions
```bash
make
```

### Test Instructions
```bash
make test
```

If you want to run the python test, do:
```bash
make clean && make

# must have poetry first
# port for testing is set to port 1025
python tests/test.py
poetry install
poetry run python test_concurrency.py 
```

### Running the Server
```bash
# Run (specify port and document root)
./httpd 8080 /path/to/docs
```


## Implementation Details
- Thread synchronization using mutex and condition variables (thanks CSAPP)
- Robust request parsing with buffer management (credit to network_utils)
- Socket timeouts for stale connections
- Resource cleanup and error recovery
- Thread pool with 100 workers

## Testing
Used both C and Python test suites:

- C tests for unit testing (request parsing, response generation)
- Python tests for integration testing (actual HTTP requests)
- Memory checks using AddressSanitizer

### Test files:
1. `tests/test_main.c`: Unit tests
2. `tests/test.py`: Integration tests

## Attributes
The network utilities code (network_utils.c and network_utils.h) is adapted from the Computer Systems: A Programmer's Perspective (CS:APP) textbook.





