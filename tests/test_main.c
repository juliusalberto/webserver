#include <stdio.h>
#include <assert.h>
#include <sys/stat.h>
#include "../src/http_server.h"

// Declare all test functions
void test_parse_request(void);


int main(void) {
    printf("Running HTTP parser tests...\n");

    test_parse_request();
    // ... other test calls
    
    printf("All tests passed!\n");
    return 0;
}

void test_parse_request() {
    http_request_t request;
    char test_request[] = 
        "GET /index.html HTTP/1.1\r\n"
        "Host: www.example.com\r\n"
        "Connection: keep-alive\r\n"
        "\r\n";
    
    // Test 1: Basic GET request
    assert(parse_request(test_request, &request) == 0);
    assert(strcmp(request.method, "GET") == 0);
    assert(strcmp(request.uri, "/index.html") == 0);
    assert(strcmp(request.version, "HTTP/1.1") == 0);
    assert(strcmp(request.host, "www.example.com") == 0);

    memset(&request, 0, sizeof(http_request_t));
    
    // Test 2: Missing Host header (should fail)
    char bad_request[] = 
        "GET /index.html HTTP/1.1\r\n"
        "Connection: keep-alive\r\n"
        "\r\n";
    assert(parse_request(bad_request, &request) == -1);
    memset(&request, 0, sizeof(http_request_t));

    // Test 3: Request with body
    char post_request[] = 
        "POST /submit HTTP/1.1\r\n"
        "Host: www.example.com\r\n"
        "Content-Length: 11\r\n"
        "\r\n"
        "hello world";
    assert(parse_request(post_request, &request) == 0);
    assert(strncmp(request.body, "hello world", 11) == 0);
}

void test_generate_response() {
    http_request_t request;
    http_response_t response;
    char docroot[] = "/tmp/test_www";  // Test document root
    
    // Create test environment
    mkdir(docroot, 0755);
    // Create test files with different permissions
    FILE* fp = fopen("/tmp/test_www/test.txt", "w");
    fprintf(fp, "Hello world!");

    struct stat file_stat;
    stat("tmp/test_www/test.txt", &file_stat);
    time_t last_mod = file_stat.st_mtime;
    char time_str[100];
    struct tm* tm_info = gmtime(&file_stat.st_mtime);
    strftime(time_str, 100, "%a, %d %b %Y %H:%M:%S GMT", tm_info);

    // Test 1: 200 OK for existing readable file
    // Verify:
    // - Status code is 200
    // - Content-Type is correct
    // - Content-Length matches file size
    // - Last-Modified header present and correct
    // - Server header present

    char test_request[] = 
    "GET /index.html HTTP/1.1\r\n"
    "Host: www.example.com\r\n"
    "Connection: keep-alive\r\n"
    "\r\n";

    parse_request(test_request, &request);
    generate_response(&request, &response, &docroot);

    // create a text file with the text "Hello World"

    assert(response.status_code == 200);
    assert(strcmp(response.content_type, "application/octet-stream") == 0);
    assert(strcmp(response.time_str, time_str) == 0);
    assert(strcmp(response.status_text, "OK") == 0);
    assert(file_stat.st_size == response.content_length);

    // Test 2: 403 Forbidden
    // Create file with no read permissions
    // Verify proper 403 response

    http_response_t forbidden_response;
    chmod("/tmp/test_www/test.txt", 0000);
    generate_response(&request, &forbidden_response, &docroot);
    assert(response.status_code == 403);
    assert(strcmp(response.status_text, "Forbidden") == 0);

    // Test 3: 404 Not Found
    // Request non-existent file
    // Verify proper 404 response

    // Test 4: Directory traversal attempt
    // Try to access file outside docroot
    // Should return 404

    // Cleanup
    // Remove test files and directory
}