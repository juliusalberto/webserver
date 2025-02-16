import unittest
import socket
import os
from pathlib import Path

class TestHTTPServer(unittest.TestCase):
    HOST = 'localhost'
    PORT = 1025
    
    def setUp(self):
        # Create test file and directory if needed
        self.test_dir = Path('tests/resources')
        self.test_dir.mkdir(parents=True, exist_ok=True)
        
        self.test_file = self.test_dir / 'test_send.txt'
        with open(self.test_file, 'w') as f:
            f.write("helloworld")

    def test_send_response_good(self):
        print("starting test_send_response_good")
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            s.settimeout(10) 
            s.connect((self.HOST, self.PORT))

            test_request = (
                "GET /tests/resources/test_send.txt HTTP/1.1\r\n"
                "Host: www.example.com\r\n"
                "Connection: close\r\n"
                "\r\n"
            )
            s.sendall(test_request.encode())
            
            f = s.makefile('rb')
            self._read_and_verify_response(f, "test_send.txt")

    def test_send_response_404(self):
        print("starting test_send_response_404")
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            s.settimeout(10) 
            s.connect((self.HOST, self.PORT))

            test_request = (
                "GET /nonexistent.txt HTTP/1.1\r\n"
                "Host: www.example.com\r\n"
                "Connection: close\r\n"
                "\r\n"
            )
            s.sendall(test_request.encode())
            
            f = s.makefile('rb')
            self._verify_404_response(f)

    def test_send_response_403(self):
        print("starting test_send_response_403")
        forbidden_file = self.test_dir / 'forbidden.txt'
        with open(forbidden_file, 'w') as f:
            f.write("secret")
        os.chmod(forbidden_file, 0o200)  # Write-only permissions
        
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            s.settimeout(10)
            s.connect((self.HOST, self.PORT))
            
            test_request = (
                f"GET /tests/resources/forbidden.txt HTTP/1.1\r\n"
                "Host: www.example.com\r\n"
                "Connection: close\r\n"
                "\r\n"
            )
            s.sendall(test_request.encode())
            
            f = s.makefile('rb')
            response_line = f.readline().decode()
            protocol, status_code, status_text = response_line.split(' ', 2)
            
            self.assertEqual(status_code, "403")
            self.assertEqual(status_text.strip(), "Forbidden")

    def test_keep_alive(self):
        print("starting test_keepalive")
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            s.settimeout(10)
            s.connect((self.HOST, self.PORT))
            
            test_request = (
                "GET /tests/resources/test_send.txt HTTP/1.1\r\n"
                "Host: www.example.com\r\n"
                "Connection: keep-alive\r\n"
                "\r\n"
            )
            s.sendall(test_request.encode())
            
            # Read first response
            f = s.makefile('rb')
            self._read_and_verify_response(f, "test_send.txt")
            
            # Second request on same connection
            s.sendall(test_request.encode())
            self._read_and_verify_response(f, "test_send.txt")

    def test_pipelining(self):
        print("starting test_pipelining")
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            s.settimeout(10)
            s.connect((self.HOST, self.PORT))
            
            # Send multiple requests without waiting for responses
            test_request = (
                "GET /tests/resources/test_send.txt HTTP/1.1\r\n"
                "Host: www.example.com\r\n"
                "Connection: keep-alive\r\n"
                "\r\n"
            )
            
            # Send 3 requests back to back
            s.sendall(test_request.encode() * 3)
            
            # Read all responses
            f = s.makefile('rb')
            for _ in range(3):
                self._read_and_verify_response(f, "test_send.txt")

    def _read_and_verify_response(self, f, filename):
        if filename is None:
            filename = self.test_file

        # Helper method to read and verify a response
        response_line = f.readline().decode()
        protocol, status_code, status_text = response_line.split(' ', 2)

        if status_code != "200":
            print(f"ERROR: File not found. Response: {response_line}")
            print(f"Current directory contents: {os.listdir('.')}") 
            exit(1)
        
        self.assertEqual(protocol, "HTTP/1.1")
        self.assertEqual(status_code, "200")
        
        content_length = None
        while True:
            line = f.readline().decode().strip()
            if not line:
                break
            if ':' in line:
                key, value = line.split(':', 1)
                if key.lower() == 'content-length':
                    content_length = int(value.strip())
        
        self.assertIsNotNone(content_length)
        content = f.read(content_length)
        
        expected_path = self.test_dir / filename
        with open(expected_path, 'rb') as file_obj:
            expected_content = file_obj.read()
        self.assertEqual(content, expected_content)

    def test_complex_pipelining(self):
        print("starting test_complex_pipelining")
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            s.settimeout(10)
            s.connect((self.HOST, self.PORT))
            
            # create some files w different sizes for better testing
            small_file = self.test_dir / 'small.txt'
            large_file = self.test_dir / 'large.txt'
            
            with open(small_file, 'w') as f:
                f.write("smol")
            with open(large_file, 'w') as f:
                f.write("b" * 8192)  # bigger than typical buffer size
            
            # multiple requests with different files + some 404s mixed in
            requests = [
                # valid file
                "GET /tests/resources/small.txt HTTP/1.1\r\n"
                "Host: www.example.com\r\n"
                "Connection: keep-alive\r\n\r\n",
                
                # nonexistent file
                "GET /nope.txt HTTP/1.1\r\n"
                "Host: www.example.com\r\n"
                "Connection: keep-alive\r\n\r\n",
                
                # large file to test buffering
                "GET /tests/resources/large.txt HTTP/1.1\r\n"
                "Host: www.example.com\r\n"
                "Connection: keep-alive\r\n\r\n",
                
                # another small file
                "GET /tests/resources/test_send.txt HTTP/1.1\r\n"
                "Host: www.example.com\r\n"
                "Connection: close\r\n\r\n"
            ]
            
            # send everything at once
            s.sendall(''.join(requests).encode())
            
            f = s.makefile('rb')
            
            # verify responses come back in order
            self._read_and_verify_response(f, "small.txt")
            self._verify_404_response(f)
            self._read_and_verify_response(f, "large.txt")
            self._read_and_verify_response(f, "test_send.txt")

    def _verify_404_response(self, f):
        response_line = f.readline().decode()
        protocol, status_code, status_text = response_line.split(' ', 2)
        self.assertEqual(status_code, "404")
        
        # skip headers
        while True:
            line = f.readline().decode().strip()
            if not line:
                break


if __name__ == '__main__':
    unittest.main()