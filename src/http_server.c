/* http_server.c */
#include "http_server.h"
#include "network_utils.h"
#include <signal.h>

bool read_request(rio_t* rp, char* dest, int client_fd);
int init_server(char* port);
int parse_request(const char *raw_request, http_request_t *request);
int generate_response(const http_request_t *request, http_response_t *response, 
                     const char *docroot);
int send_response(int client_fd, const http_response_t *response);
int send_error_response(int client_fd, const http_response_t* response);
void init_shared_buffer(void); 
void init_thread(pthread_t* workers, int length);
void add_to_buffer(http_task_t* new_task);
void *consumer_thread(void *arg);
void cleanup_server(void);

// should probably refactor this
// put it in different 
sbuf_cond_t shared_buffer;
volatile sig_atomic_t keep_running = 1;

void handle_sigint(int sig) {
    (void)sig;
    exit(0);
}

int init_server(char* port) {
    int server_fd;
    struct addrinfo hints, *res;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    int status;
    if ((status = getaddrinfo(NULL, port, &hints, &res)) < 0) {
        perror("getaddrinfo");
        freeaddrinfo(res);
        return -1;
    }
    
    // Create socket file descriptor
    if ((server_fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) < 0) {
        perror("socket");
        freeaddrinfo(res);
        return -1;
    }
    
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt failed");
        freeaddrinfo(res);
        return -1;
    }
    
    // Bind socket to address
    if (bind(server_fd, res->ai_addr, res->ai_addrlen) < 0) {
        perror("bind failed");
        freeaddrinfo(res);
        return -1;
    }
    
    // Listen for connections
    if (listen(server_fd, SOMAXCONN) < 0) {
        perror("listen failed");
        freeaddrinfo(res);
        return -1;
    }
    
    freeaddrinfo(res);
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

    if (!request->host[0]) {
        return -1;
    }

    // if we arrive here it means that we're already in the body
    if ((size_t) content_length > sizeof(request->body)) {
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
int send_response(int client_fd, const http_response_t *response) {
    // This is where you send the response back to the client
    // TODO: Implement this function
    int n_bytes = 0;
    char buf[MAXBUF];
    size_t remaining = MAXBUF;
    
    n_bytes += snprintf(buf, remaining, "HTTP/1.1 %d %s\r\n", response->status_code, response->status_text);
    remaining -= n_bytes;
    n_bytes += snprintf(buf + n_bytes, remaining, "Server: TinyServer\r\n");
    remaining -= n_bytes;

    if (response->connection_close) {
        n_bytes += snprintf(buf + n_bytes, remaining, "Connection: close\r\n");
        remaining -= n_bytes;
    }
    if (response->status_code == 200) {
        n_bytes += snprintf(buf + n_bytes, remaining, "Last-Modified: %s\r\n", response->time_str);
        remaining -= n_bytes;
    }
   
    n_bytes += snprintf(buf + n_bytes, remaining, "Content-length: %lu\r\n", response->content_length);
    remaining -= n_bytes;
    n_bytes += snprintf(buf + n_bytes, remaining, "Content-type: %s\r\n", response->content_type);
    remaining -= n_bytes;
    n_bytes += snprintf(buf + n_bytes, remaining, "\r\n");
    remaining -= n_bytes;

    if (n_bytes > MAXBUF) {
        printf("Buffer overflow");
        return -1;
    }

    if (rio_writen(client_fd, buf, strlen(buf)) != n_bytes) {
        printf("Wrong header length being sent");
        return -1;
    }
    printf("Response headers:\n");
    printf("%s", buf);

    if (rio_writen(client_fd, response->content, response->content_length) != response->content_length) {
        printf("Wrong body length being sent");
        return -1;
    }
    free(response->content);

    return 0;
}

int send_error_response(int client_fd, const http_response_t* response) {
    char buf[MAXBUF];
    int n_bytes = 0;
    
    n_bytes += snprintf(buf, MAXBUF, "HTTP/1.1 %d %s\r\n", response->status_code, response->status_text);
    if (rio_writen(client_fd, buf, strlen(buf)) != n_bytes) {
        printf("Wrong header length being sent");
        return -1;
    }
    printf("Response headers:\n");
    printf("%s", buf);
    return 0;
}

#ifndef TESTING
int main(int argc, char *argv[]) {
    signal(SIGINT, handle_sigint);
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <port> <docroot>\n", argv[0]);
        return 1;
    }
    
    int port = atoi(argv[1]);
    char *docroot = argv[2];
    
    // Initialize server
    int server_fd = init_server(port);
    pthread_t workers[100];
    init_thread(workers, 100);
    init_shared_buffer();
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
        // At this point we're already creating the socket, binding the socket, and listening for 
        // connections
        int client_fd = accept(server_fd, (struct sockaddr*) &client_addr, &client_len);

        // should def break here and create a worker thread
        http_task_t* new_request = malloc(sizeof(http_task_t));
        new_request->client_fd = client_fd;
        new_request->client_addr = client_addr;
        new_request->docroot = docroot;
        add_to_buffer(new_request);

        // TODO: Free any allocated memory
    }
    
    cleanup_server();
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

/*
HTTP Thread Section
*/

void add_to_buffer(http_task_t* new_task) {
    pthread_mutex_lock(&shared_buffer.lock);

    while (shared_buffer.count == MAX_TASK) {
        pthread_cond_wait(&shared_buffer.not_full, &shared_buffer.lock);
    }

    shared_buffer.tasks[shared_buffer.rear] = new_task;
    shared_buffer.rear = (shared_buffer.rear + 1) % MAX_TASK;
    shared_buffer.count++;

    pthread_cond_signal(&shared_buffer.not_empty);
    pthread_mutex_unlock(&shared_buffer.lock);
} 

http_task_t* get_task_from_buffer(void) {
    pthread_mutex_lock(&shared_buffer.lock);
    while (shared_buffer.count == 0) {
        pthread_cond_wait(&shared_buffer.not_empty, &shared_buffer.lock);
    }

    http_task_t* returned_task = shared_buffer.tasks[shared_buffer.front];
    shared_buffer.front = (shared_buffer.front + 1) % MAX_TASK;
    shared_buffer.count--;


    pthread_cond_signal(&shared_buffer.not_full);
    pthread_mutex_unlock(&shared_buffer.lock);

    return returned_task;
}

void init_shared_buffer() {
  shared_buffer.count = 0;
  shared_buffer.front = 0;
  shared_buffer.rear = 0;
  pthread_cond_init(&shared_buffer.not_empty, NULL);
  pthread_cond_init(&shared_buffer.not_full, NULL);
  pthread_mutex_init(&shared_buffer.lock, NULL);
}

void *consumer_thread(void *arg) {
    pthread_detach(pthread_self());
    int client_fd;
    rio_t rio;
    char* docroot;

    while (true && keep_running == 1) {
        http_task_t* task = get_task_from_buffer();
        docroot = task->docroot;
        client_fd = task->client_fd;
        
        printf("Accepted client\n");
        if (client_fd < 0) {
            perror("accept failed");
            continue;
        }
        
        // TODO: Set socket timeout
        struct timeval timeout;
        timeout.tv_sec = 5;
        timeout.tv_usec = 0;
        setsockopt(client_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
        
        char raw_request[MAX_REQUEST_SIZE];
        rio_readinitb(&rio, client_fd);
        bool read_header_status = read_request(&rio, raw_request, client_fd);
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
        if (generate_response(&request, &response, docroot) < 0) {
            // TODO: Send appropriate error response
            send_error_response(client_fd, &response);
            close(client_fd);
            continue;
        }
        
        // Send response
        if (send_response(client_fd, &response) < 0) {
            // TODO: Handle send error
            close(client_fd);
            continue;
        }
        
        close(client_fd);
        free(task);
    }
}

void init_thread(pthread_t* workers, int length) {
    for (int i = 0; i < length; i++) {
        pthread_create(&workers[i], NULL, consumer_thread, NULL);
    }
}

void cleanup_server() {
    keep_running = 0;

    // wake up thread;
    pthread_mutex_lock(&shared_buffer.lock);
    pthread_cond_broadcast(&shared_buffer.not_empty);
    pthread_cond_broadcast(&shared_buffer.not_full);
    pthread_mutex_unlock(&shared_buffer.lock);

    sleep(1);

    // pthread_mutex_destroy()
}