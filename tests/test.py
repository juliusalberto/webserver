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
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            s.connect((self.HOST, self.PORT))
            
            test_request = (
                "GET /tests/resources/test_send.txt HTTP/1.1\r\n"
                "Host: www.example.com\r\n"
                "Connection: keep-alive\r\n"
                "\r\n"
            )
            s.sendall(test_request.encode())
            
            # Read response status line
            f = s.makefile('rb')
            response_line = f.readline().decode()
            print(response_line)
            protocol, status_code, status_text = response_line.split(' ', 2)
            status_text = status_text.strip()
            
            self.assertEqual(protocol, "HTTP/1.1")
            self.assertEqual(status_code, "200")
            self.assertEqual(status_text, "OK")

            header = ""
            
            content_length = None
            while True:
                line = f.readline().decode().strip()
                print(line)
                if not line:  
                    break
                    
                if ':' in line:
                    key, value = line.split(':', 1)
                    if key.lower() == 'content-length':
                        content_length = int(value.strip())
            
            print(header)
            self.assertIsNotNone(content_length, "No Content-Length header found")
            
            # Read response body
            content = s.recv(content_length)
            
            # Compare with expected content
            with open(self.test_file, 'rb') as f:
                expected_content = f.read()
                
            self.assertEqual(len(expected_content), content_length)
            self.assertEqual(content, expected_content)

            print(f"content: {content}")

if __name__ == '__main__':
    unittest.main()