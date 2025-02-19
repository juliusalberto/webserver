#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "../src/network_utils.h"
#include "../src/http_server.h"

#define CHECK_OR_DIE(expr, msg) \
   do { \
       if (!(expr)) { \
           perror(msg); \
           exit(1); \
       } \
   } while(0)

// Forward declarations for test functions
void test_parse_request(void);
void test_generate_response(void);
void cleanup(void);

static char* HOST = "localhost";
static char* PORT = "1025";

#define TEST_ASSERT(expr) do { \
    if (!(expr)) { \
        fprintf(stderr, "Assertion failed: %s, function %s, file %s, line %d.\n", \
                #expr, __func__, __FILE__, __LINE__); \
        cleanup(); \
        exit(EXIT_FAILURE); \
    } \
} while (0)

static char docroot[256];
static char test_file_path[256];
static char forbidden_file_path[256];

int main(void) {
    printf("Running HTTP parser tests...\n");

    char cwd[256];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("Current working directory: %s\n", cwd);
    } else {
        perror("getcwd failed");
    }

    snprintf(docroot, sizeof(docroot), "%s/tests/resources", cwd);
    snprintf(test_file_path, sizeof(test_file_path), "%s/tests/resources/test_c.txt", cwd);
    snprintf(forbidden_file_path, sizeof(forbidden_file_path), "%s/tests/resources/forbidden.txt", cwd);

    // Check the constructed paths
    printf("Docroot: %s\n", docroot);
    printf("Test file path: %s\n", test_file_path);
    printf("Forbidden file path: %s\n", forbidden_file_path);


    test_parse_request();
    test_generate_response();
    
    // Final cleanup (in case all tests pass)
    // cleanup();
    
    printf("All tests passed!\n");
    return 0;
}

void cleanup(void) {
    remove(test_file_path);
    remove(forbidden_file_path);
    rmdir(docroot);
}

void test_parse_request(void) {
    http_request_t request;
    char test_request[] = 
        "GET /index.html HTTP/1.1\r\n"
        "Host: www.example.com\r\n"
        "Connection: keep-alive\r\n"
        "\r\n";
    
    // Test 1: Basic GET request
    TEST_ASSERT(parse_request(test_request, &request) == 0);
    TEST_ASSERT(strcmp(request.method, "GET") == 0);
    TEST_ASSERT(strcmp(request.uri, "/index.html") == 0);
    TEST_ASSERT(strcmp(request.version, "HTTP/1.1") == 0);
    TEST_ASSERT(strcmp(request.host, "www.example.com") == 0);

    memset(&request, 0, sizeof(http_request_t));
    
    // Test 2: Missing Host header (should fail)
    char bad_request[] = 
        "GET /index.html HTTP/1.1\r\n"
        "Connection: keep-alive\r\n"
        "\r\n";
    TEST_ASSERT(parse_request(bad_request, &request) == -1);
    memset(&request, 0, sizeof(http_request_t));

    // Test 3: Request with body
    char post_request[] = 
        "POST /submit HTTP/1.1\r\n"
        "Host: www.example.com\r\n"
        "Content-Length: 11\r\n"
        "\r\n"
        "hello world";
    TEST_ASSERT(parse_request(post_request, &request) == 0);
    TEST_ASSERT(strncmp(request.body, "hello world", 11) == 0);
}

void test_generate_response(void) {
    http_request_t request;
    http_response_t response;
    char docroot_local[256];
    snprintf(docroot_local, sizeof(docroot_local), "%s", docroot);

    // Create test environment: make sure the docroot exists
    if (mkdir(docroot, 0755) != 0) {
        if (errno != EEXIST) {  // Ignore error if directory already exists
            perror("mkdir");
            exit(1);
        }
    }
    
    // -----------------------------------------------------
    // Test 1: 200 OK for test.txt
    // -----------------------------------------------------
    const char *doc_text = "Hello world!";
    FILE* fp = fopen(test_file_path, "w");
    printf("test file path: %s\n", test_file_path);
    if (fp == NULL) {
        perror("fopen test.txt");
        exit(1);
    }
    fprintf(fp, "%s", doc_text);
    fflush(fp);
    fclose(fp);

    // Get file stats for test.txt to compare Last-Modified header etc.
    struct stat file_stat;
    if (stat(test_file_path, &file_stat) != 0) {
        perror("stat test.txt");
        exit(1);
    }
    char time_str[100];
    struct tm* tm_info = gmtime(&file_stat.st_mtime);
    strftime(time_str, sizeof(time_str), "%a, %d %b %Y %H:%M:%S GMT", tm_info);

    char test_request[] = 
        "GET /test_c.txt HTTP/1.1\r\n"
        "Host: www.example.com\r\n"
        "Connection: keep-alive\r\n"
        "\r\n";

    memset(&request, 0, sizeof(request));
    TEST_ASSERT(parse_request(test_request, &request) == 0);
    generate_response(&request, &response, docroot);

    TEST_ASSERT(response.status_code == 200);
    TEST_ASSERT(strcmp(response.status_text, "OK") == 0);
    TEST_ASSERT(strcmp(response.content_type, "application/octet-stream") == 0);
    TEST_ASSERT(strcmp(response.time_str, time_str) == 0);
    TEST_ASSERT(file_stat.st_size == response.content_length);
    TEST_ASSERT(strcmp(response.content, doc_text) == 0);

    // -----------------------------------------------------
    // Test 2: 403 Forbidden for a file with no read rights
    // -----------------------------------------------------
    const char *forbidden_text = "This file should be forbidden";
    
    FILE* fp2 = fopen(forbidden_file_path, "w");
    if (fp2 == NULL) {
        perror("fopen forbidden.txt");
        exit(1);
    }
    fprintf(fp2, "%s", forbidden_text);
    fflush(fp2);
    fclose(fp2);
    
    // Remove all permissions from forbidden.txt
    if (chmod(forbidden_file_path, 0000) != 0) {
        perror("chmod forbidden.txt");
        exit(1);
    }
    
    char forbidden_request[] =
        "GET /forbidden.txt HTTP/1.1\r\n"
        "Host: www.example.com\r\n"
        "Connection: keep-alive\r\n"
        "\r\n";
    
    memset(&request, 0, sizeof(request));
    TEST_ASSERT(parse_request(forbidden_request, &request) == 0);
    http_response_t forbidden_response;
    generate_response(&request, &forbidden_response, docroot);
    TEST_ASSERT(forbidden_response.status_code == 403);
    TEST_ASSERT(strcmp(forbidden_response.status_text, "Forbidden") == 0);

    // -----------------------------------------------------
    // Test 3: 404 File Not Found for non-existent file
    // -----------------------------------------------------
    char non_existent_request[] =
        "GET /blabla.txt HTTP/1.1\r\n"
        "Host: www.example.com\r\n"
        "Connection: keep-alive\r\n"
        "\r\n";

    memset(&request, 0, sizeof(request));
    TEST_ASSERT(parse_request(non_existent_request, &request) == 0);
    http_response_t non_existent_response;
    generate_response(&request, &non_existent_response, docroot);
    TEST_ASSERT(non_existent_response.status_code == 404);
    TEST_ASSERT(strcmp(non_existent_response.status_text, "Not Found") == 0);

    // Test 4: Directory traversal attempt

    cleanup();
}


