#!/usr/bin/env python3
"""
Simple UDP server to receive messages from RTEMS STM32 Nucleo board

Para probar desde la MAC, use el siguiente comando en la terminal:
echo "Test message from Mac" | python3 -c "import socket, sys; s=socket.socket(socket.AF_INET, socket.SOCK_DGRAM); s.sendto(sys.stdin.buffer.read(), ('localhost', 5000))"

ifconfig | grep "inet " | grep -v 127.0.0.1

"""
import socket
import datetime

# Configuration
UDP_IP = "0.0.0.0"  # Listen on all interfaces
UDP_PORT = 5000     # Must match UDP_SERVER_PORT in your RTEMS application

def main():
    # Create UDP socket
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind((UDP_IP, UDP_PORT))
    
    print("=" * 60)
    print("UDP Server for RTEMS STM32 Nucleo Board")
    print("=" * 60)
    print(f"Listening on {UDP_IP}:{UDP_PORT}")
    print("Waiting for messages from the board...")
    print("Press Ctrl+C to exit")
    print("=" * 60)
    print()
    
    message_count = 0
    
    try:
        while True:
            # Receive data (buffer size 1024 bytes)
            data, addr = sock.recvfrom(1024)
            
            message_count += 1
            timestamp = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S.%f")[:-3]
            
            # Decode message
            try:
                message = data.decode('utf-8')
            except UnicodeDecodeError:
                message = data.decode('utf-8', errors='replace')
            
            # Log to console
            print(f"[{timestamp}] [{addr[0]}:{addr[1]}] Msg #{message_count}")
            print(f"  {message}", end='')
            
            # Add newline if message doesn't end with one
            if not message.endswith('\n'):
                print()
            
    except KeyboardInterrupt:
        print("\n" + "=" * 60)
        print(f"Server stopped. Total messages received: {message_count}")
        print("=" * 60)
    finally:
        sock.close()

if __name__ == "__main__":
    main()
