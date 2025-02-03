/* http_server.c */
#include "http_server.h"
#include "network_utils.h"
#include <stdbool.h>

int init_server(int port) {
    int server_fd;
    struct sockaddr_in address;
    
    // Create socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket failed");
        return -1;
    }
    
    // Set socket options to reuse address and port
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt failed");
        return -1;
    }
    
    // Setup server address structure
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);
    
    // Bind socket to address
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        return -1;
    }
    
    // Listen for connections
    if (listen(server_fd, SOMAXCONN) < 0) {
        perror("listen failed");
        return -1;
    }
    
    return server_fd;
}

// TODO: Implement parse_request()
int parse_request(const char *raw_request, http_request_t *request) {
    // This is a basic example. You need to implement proper parsing
    if (raw_request == NULL || request == NULL) {
        return -1;
    }

    char* end_of_line = strstr(raw_request, "\r\n");
    if (!end_of_line) {
        return -1;
    }

    size_t line_length = end_of_line - raw_request;
    char first_line[MAXLINE];

    strncpy(first_line, raw_request, line_length);
    first_line[line_length] = '\0';

    // now we have the first line as a proper string
    char* token = strtok(first_line, ' ');
    strncpy(request->method, token, 15);
    token = strtok(NULL, ' ');
    strncpy(request->uri, token, MAX_URI_LENGTH - 1);
    token = strtok(NULL, ' ');
    strncpy(request->version, token, 15);
    
    return 0;
}

// TODO: Implement generate_response()
int generate_response(const http_request_t *request, http_response_t *response, 
                     const char *docroot) {
    // This is where you generate the appropriate response
    // based on the request and document root
    // TODO: Implement this function
    
    return 0;
}

// TODO: Implement send_response()
int send_response(int client_fd, const http_response_t *response, 
                 const char *content) {
    // This is where you send the response back to the client
    // TODO: Implement this function
    
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <port> <docroot>\n", argv[0]);
        return 1;
    }
    
    int port = atoi(argv[1]);
    char *docroot = argv[2];
    
    // Initialize server
    int server_fd = init_server(port);
    if (server_fd < 0) {
        fprintf(stderr, "Failed to initialize server\n");
        return 1;
    }
    
    printf("Server listening on port %d...\n", port);
    
    // Main server loop
    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        rio_t rio;
        
        // Accept client connection
        int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, 
                             &client_len);
        if (client_fd < 0) {
            perror("accept failed");
            continue;
        }
        
        // TODO: Set socket timeout
        
        // TODO: Read request
        char raw_request[MAX_REQUEST_SIZE];
        // TODO: Implement reading from client_fd into raw_request
        // remember that we need to read the whole request (can be multiple )
        bool read_header_status = read_request(&rio, &raw_request, client_fd);
        if (!read_header_status) {
            // this is when the request size is too large
            continue;
        }


        // Parse request
        http_request_t request;
        if (parse_request(raw_request, &request) < 0) {
            // TODO: Send 400 Bad Request
            close(client_fd);
            continue;
        }
        
        // Generate response
        http_response_t response;
        char *content = NULL;
        if (generate_response(&request, &response, docroot) < 0) {
            // TODO: Send appropriate error response
            close(client_fd);
            continue;
        }
        
        // Send response
        if (send_response(client_fd, &response, content) < 0) {
            // TODO: Handle send error
        }
        
        // TODO: Handle connection: close header
        
        close(client_fd);
        // TODO: Free any allocated memory
    }
    
    close(server_fd);
    return 0;
}

bool read_request(rio_t* rp, char* dest, int client_fd) {
    ssize_t curr_size;
    size_t total;
    char line[MAXLINE];

    while ((curr_size = rio_readlineb(rp, line, MAXLINE)) > 0) {
        if (curr_size + total >= MAX_REQUEST_SIZE) {
            fprintf(stderr, "File too large!");
            close(client_fd);
            return false;
        }

        memcpy(dest + total, line, curr_size);
        // the line is already copied to destination
        total += curr_size;

        // check for end of line
        // the end of http header is supposed to be \r\n
        if (curr_size == 2 && line[0] == '\r' && line[1] == '\n') {
            return true;
        }
    }

    return false;
}