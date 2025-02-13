/* http_server.h */
#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <errno.h>
#include <stdbool.h>

/* Constants */
#define MAX_REQUEST_SIZE 8192
#define MAX_URI_LENGTH 2048
#define TIMEOUT_SECS 5
#define MAX_TASK 100
#define SERVER_NAME "TritonHTTP/1.0"


/* HTTP Request Structure */
typedef struct {
    char method[16];      // GET
    char uri[MAX_URI_LENGTH];     // /path/to/file
    char version[16];     // HTTP/1.1
    char host[256];       // Required header
    bool connection_close; // Connection: close header present?
    char body[4096]; //max 4 KB
    // TODO: Add more headers as needed
} http_request_t;

/* HTTP Response Structure */
typedef struct {
    int status_code;          // 200, 404, etc.
    char status_text[128];        // OK, Not Found, etc.
    char content_type[128];   // text/html, image/jpeg, etc.
    size_t content_length;    // Length of the body
    bool connection_close;     // Whether to close connection
    char time_str[100];          // Last Modified
    char* content;
    // TODO: Add more headers as needed
} http_response_t;


typedef struct http_task {
    int client_fd;
    struct sockaddr_in client_addr;
    char* docroot;
} http_task_t;

typedef struct shared_buffer {
    pthread_mutex_t lock;
    pthread_cond_t not_full, not_empty;
    int count;
    http_task_t* tasks[MAX_TASK];
    int front;
    int rear;
} sbuf_cond_t;

/* Function declarations */

/**
 * Initialize server socket
 * Returns: socket file descriptor or -1 on error
 */
int init_server(char* port);

/**
 * Parse raw HTTP request into http_request_t structure
 * Returns: 0 on success, -1 on error
 * TODO: Implement this function
 */
int parse_request(const char *raw_request, http_request_t *request);

/**
 * Generate HTTP response based on request
 * Returns: 0 on success, -1 on error
 * TODO: Implement this function
 */
int generate_response(const http_request_t *request, http_response_t *response, 
                     const char *docroot);

/**
 * Send HTTP response to client
 * Returns: 0 on success, -1 on error
 * TODO: Implement this function
 */
int send_response(int client_fd, const http_response_t *response);

#endif /* HTTP_SERVER_H */