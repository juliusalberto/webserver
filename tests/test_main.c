#include <stdio.h>
#include <assert.h>
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
    
    // Test 2: Missing Host header (should fail)
    char bad_request[] = 
        "GET /index.html HTTP/1.1\r\n"
        "Connection: keep-alive\r\n"
        "\r\n";
    assert(parse_request(bad_request, &request) == -1);

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