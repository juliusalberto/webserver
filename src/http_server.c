/* http_server.c */
#include "http_server.h"

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
    
    // Parse the request line
    // Example: GET /index.html HTTP/1.1
    // TODO: Implement proper parsing with error checking
    
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