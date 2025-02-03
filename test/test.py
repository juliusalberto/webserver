#!/usr/bin/env python3
import socket
import time
import unittest
import subprocess
import os
import signal
from typing import Tuple, Optional

class HTTPTestCase(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        # Configuration
        cls.PORT = 8080
        cls.HOST = "localhost"
        cls.DOC_ROOT = "/tmp/http_test"
        
        # Create test files
        os.makedirs(cls.DOC_ROOT, exist_ok=True)
        with open(f"{cls.DOC_ROOT}/index.html", "w") as f:
            f.write("<html><body>Test Index</body></html>")
        
        # Start server
        cls.server_process = subprocess.Popen(
            ["./httpd", str(cls.PORT), cls.DOC_ROOT],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE
        )
        time.sleep(1)  # Give server time to start

    @classmethod
    def tearDownClass(cls):
        # Cleanup
        cls.server_process.terminate()
        cls.server_process.wait()
        
    def send_request(self, request: str, expect_response: bool = True) -> Optional[Tuple[str, str]]:
        """Send a request and return headers and body separately"""
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.connect((self.HOST, self.PORT))
        sock.send(request.encode())
        
        if not expect_response:
            sock.close()
            return None
            
        response = b""
        while True:
            data = sock.recv(4096)
            if not data:
                break
            response += data
        
        sock.close()
        
        # Split response into headers and body
        response = response.decode()
        headers, _, body = response.partition("\r\n\r\n")
        return headers, body

    def test_basic_get(self):
        """Test basic GET request"""
        request = (
            "GET /index.html HTTP/1.1\r\n"
            f"Host: {self.HOST}\r\n"
            "\r\n"
        )
        headers, body = self.send_request(request)
        
        # Verify response
        self.assertIn("HTTP/1.1 200 OK", headers)
        self.assertIn("Content-Type: text/html", headers)
        self.assertIn("Content-Length:", headers)
        self.assertIn("Test Index", body)

    def test_404_not_found(self):
        """Test 404 response for non-existent file"""
        request = (
            "GET /nonexistent.html HTTP/1.1\r\n"
            f"Host: {self.HOST}\r\n"
            "\r\n"
        )
        headers, _ = self.send_request(request)
        self.assertIn("HTTP/1.1 404", headers)

    def test_400_bad_request(self):
        """Test 400 response for malformed request"""
        request = "GET /index.html\r\n\r\n"  # Missing HTTP version
        headers, _ = self.send_request(request)
        self.assertIn("HTTP/1.1 400", headers)

    def test_no_host_header(self):
        """Test that missing Host header returns 400"""
        request = "GET /index.html HTTP/1.1\r\n\r\n"
        headers, _ = self.send_request(request)
        self.assertIn("HTTP/1.1 400", headers)

    def test_connection_close(self):
        """Test Connection: close header handling"""
        request = (
            "GET /index.html HTTP/1.1\r\n"
            f"Host: {self.HOST}\r\n"
            "Connection: close\r\n"
            "\r\n"
        )
        headers, _ = self.send_request(request)
        self.assertIn("Connection: close", headers.lower())

    def test_pipelining(self):
        """Test handling of pipelined requests"""
        pipelined_request = (
            "GET /index.html HTTP/1.1\r\n"
            f"Host: {self.HOST}\r\n"
            "\r\n"
            "GET /index.html HTTP/1.1\r\n"
            f"Host: {self.HOST}\r\n"
            "Connection: close\r\n"
            "\r\n"
        )
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.connect((self.HOST, self.PORT))
        sock.send(pipelined_request.encode())
        
        # Should receive two complete responses
        response = sock.recv(8192).decode()
        sock.close()
        
        self.assertEqual(response.count("HTTP/1.1 200 OK"), 2)

    def test_timeout(self):
        """Test 5-second timeout"""
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.connect((self.HOST, self.PORT))
        
        # Send partial request
        sock.send(b"GET /index.html HTTP/1.1\r\n")
        
        # Wait for timeout
        time.sleep(6)
        
        # Should receive 400 response
        response = sock.recv(4096).decode()
        sock.close()
        
        self.assertIn("HTTP/1.1 400", response)

if __name__ == "__main__":
    unittest.main()