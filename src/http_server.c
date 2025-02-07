/* http_server.c */
#include "http_server.h"
#include "network_utils.h"

bool read_request(rio_t* rp, char* dest, int client_fd);
int init_server(int port);
int parse_request(const char *raw_request, http_request_t *request);
int generate_response(const http_request_t *request, http_response_t *response, 
                     const char *docroot);
int send_response(int client_fd, const http_response_t *response, 
                 const char *content);

int init_server(int port) {
    int server_fd;
    struct sockaddr_in address;
    
    // Create socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
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
    if (bind(server_fd, (struct sockaddr*) &address, sizeof(address)) < 0) {
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
    char* method = strtok(first_line, " ");
    char* uri = strtok(NULL, " "); 
    char* version = strtok(NULL, "\r");  // now points to "HTTP/1.1"

    strncpy(request->method, method, sizeof(request->method) - 1);
    strncpy(request->uri, uri, MAX_URI_LENGTH - 1);
    strncpy(request->version, version, sizeof(request->version) - 1);

    char* line_start = end_of_line + 2;
    char curr_line[MAXLINE];
    int content_length = 0;
    while (true) {
        end_of_line = strstr(line_start, "\r\n");
        line_length = end_of_line - line_start;

        if (!end_of_line) {
            return -1;
        }

        if (line_start == end_of_line) {
            // we're already at the end of header
            line_start = end_of_line + 2;
            break;
        }

        // extract the string
        strncpy(curr_line, line_start, line_length);
        curr_line[line_length] = '\0';

        char* key = strtok(curr_line, ":");

        if (key) {
            char* value = key + strlen(key) + 1;
            // strip dangling spaces
            while (*value == ' ') {
                value++;
            }

            if (strcasecmp(key, "Host") == 0) {
                // get the host
                strcpy(request->host, value);
            } else if (strcasecmp(key, "Content-Length") == 0) {
                content_length = atoi(value);
            } else if (strcasecmp(key, "Connection") == 0) {
                if (strcmp(value, "close") == 0) {
                    request->connection_close = true;
                } 
            }
        }

        line_start = end_of_line + 2;
    }

    if (!request->host[0] || request->host == NULL) {
        return -1;
    }

    // if we arrive here it means that we're already in the body
    if (content_length > sizeof(request->body)) {
        content_length = sizeof(request->body) - 1;

        printf("Body too large!");
    }

    memcpy(request->body, line_start, content_length);
    
    return 0;
}

// TODO: Implement generate_response()
int generate_response(const http_request_t *request, http_response_t *response, 
                     const char *docroot) {
    // This is where you generate the appropriate response
    // based on the request and document root
    // TODO: Implement this function
    char new_filename[MAX_URI_LENGTH];
    const char* filename = request->uri;
    if (strcmp(filename, "/") == 0) {
        snprintf(new_filename, MAX_URI_LENGTH, "/index.html");
        filename = new_filename;
    }

    char combined_path[MAX_URI_LENGTH];
    snprintf(combined_path, MAX_URI_LENGTH, "%s%s", docroot, filename);

    char* real_path = realpath(combined_path, NULL);
    if (real_path == NULL) {
        response->status_code = 404;
        strcpy(response->status_text, "Not Found");
        return -1;
    }
    if (strlen(real_path) > MAX_URI_LENGTH) {
        free(real_path);
        return -1;
    }

    if (strncmp(real_path, docroot, strlen(docroot)) != 0) {
        response->status_code = 404;
        strcpy(response->status_text, "Not Found");
        free(real_path);
        return -1;
    }

    FILE* fp = fopen(real_path, "r"); // open file

    if (fp == NULL) {
        if (errno == ENOENT) {
            // File does not exist
            response->status_code = 404;
            strcpy(response->status_text, "Not Found");
        } else if (errno == EACCES) {
            // File exists but permission is denied
            response->status_code = 403;
            strcpy(response->status_text, "Forbidden");
        } else {
            // Other error conditions
            response->status_code = 500;
            strcpy(response->status_text, "Internal Server Error");
        }
        return -1;
    }

    // get file stat to get its last modified + size

    struct stat file_stat;
    if (stat(real_path, &file_stat) == -1) {
        fclose(fp);
        return -1;
    }

    if (!(file_stat.st_mode & S_IROTH)) {
        response->status_code = 403;
        strcpy(response->status_text, "Forbidden");
        free(real_path);
        return -1;
    }

    response->content_length = file_stat.st_size;
    strftime(response->time_str, 100, "%a, %d %b %Y %H:%M:%S GMT", gmtime(&file_stat.st_mtime));
    
    // get content type
    char* ext = strrchr(filename, '.');
    if (ext != NULL) {
        if (strcasecmp(ext, ".html") == 0) {
            strcpy(response->content_type, "text/html");
        } else if (strcasecmp(ext, ".jpg") == 0) {
            strcpy(response->content_type, "image/jpeg");
        } else if (strcasecmp(ext, ".png") == 0) {
            strcpy(response->content_type, "image/png");
        } else {
            strcpy(response->content_type, "application/octet-stream");
        }
    } else {
        strcpy(response->content_type, "application/octet-stream");
    }

    // read content
    response->content = malloc(file_stat.st_size);
    if (response->content == NULL) {
        response->status_code = 500;  // Internal Server Error
        strcpy(response->status_text, "Internal Server Error");
        free(real_path);
        fclose(fp);
        return -1;
    }

    if (fread(response->content, 1, file_stat.st_size, fp) != file_stat.st_size) {
        free(response->content);
        free(real_path);
        fclose(fp);
        return -1;
    }

    // check connection close
    if (request->connection_close) {
        response->connection_close = true;
    }

    response->status_code = 200;
    strcpy(response->status_text, "OK");

    free(real_path);
    fclose(fp);
    return 0;
}

// TODO: Implement send_response()
int send_response(int client_fd, const http_response_t *response, 
                 const char *content) {
    // This is where you send the response back to the client
    // TODO: Implement this function
    
    return 0;
}

#ifndef TESTING
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
        // At this point we're already creating the socket, binding the socket, and listening for 
        // connections
        int client_fd = accept(server_fd, (struct sockaddr*) &client_addr, &client_len);
        if (client_fd < 0) {
            perror("accept failed");
            continue;
        }
        
        // TODO: Set socket timeout
        
        // TODO: Read request
        char raw_request[MAX_REQUEST_SIZE];
        // TODO: Implement reading from client_fd into raw_request
        // remember that we need to read the whole request (can be multiple )
        rio_readinitb(&rio, client_fd);
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
#endif

bool read_request(rio_t* rp, char* dest, int client_fd) {
    ssize_t curr_size;
    size_t total = 0;
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
            // end of headers
            break;
        }
    }

    // If we're here, it means that we're already at the body
    char* content_length_str = strstr(dest, "Content-Length: ");
    int content_length = 0;
    if (content_length_str) {
        content_length = atoi(content_length_str + strlen("Content-Length: "));
    }

    // get the body
    if (rio_readnb(rp, dest, content_length) != content_length) {
        return false;
    }

    total += content_length;
    return true;
}