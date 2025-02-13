import requests
import time
from concurrent.futures import ThreadPoolExecutor
import statistics

def make_request(i):
    start_time = time.time()
    try:
        response = requests.get('http://localhost:1025/test.html')
        end_time = time.time()
        return end_time - start_time, response.status_code
    except requests.exceptions.RequestException as e:
        print(f"Request {i} failed: {e}")
        return None

def test_concurrent_requests(num_requests, max_workers):
    times = []
    with ThreadPoolExecutor(max_workers=max_workers) as executor:
        futures = [executor.submit(make_request, i) for i in range(num_requests)]
        for f in futures:
            result = f.result()
            if result:
                times.append(result[0])
    
    if times:
        print(f"Average response time: {statistics.mean(times):.3f}s")
        print(f"Max response time: {max(times):.3f}s")
        print(f"Min response time: {min(times):.3f}s")
        print(f"Successful requests: {len(times)}/{num_requests}")

if __name__ == "__main__":
    for workers in [10, 50, 100]:
        print(f"\nTesting with {workers} concurrent requests:")
        test_concurrent_requests(workers, workers)