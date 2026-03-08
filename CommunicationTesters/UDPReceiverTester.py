import socket
import json


def receive_json_messages(host, port):
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind((host, port))
    
    print(f"\n[UDP Receiver] Listening on {host}:{port}")
    print("[UDP Receiver] Waiting for messages... (Press Ctrl+C to stop)\n")
    
    try:
        while True:
            data, addr = sock.recvfrom(1024)
            message_json = data.decode('utf-8')
            message_dict = json.loads(message_json)
            
            print("-" * 50)
            print(f"Message received from {addr[0]}:{addr[1]}")
            print(json.dumps(message_dict, indent=2))
            print("-" * 50)
            print()
            
    except KeyboardInterrupt:
        print("\n[UDP Receiver] Stopped")
    except Exception as e:
        print(f"Error: {e}")
    finally:
        sock.close()

    
#Implement if/when we use structs for messaging
def receive_struct_messages(host, port):
    pass


def main():
    print("=" * 50)
    print("UDP Receiver Tester")
    print("=" * 50)
    
    host_input = input("\nEnter IP address (default 127.0.0.1): ").strip()
    host = host_input if host_input else "127.0.0.1"
    
    port_input = input("Enter port (default 5000): ").strip()
    port = int(port_input) if port_input else 5000
    
    print("\nSelect message format:")
    print("1) JSON")
    format_choice = input("Choice (default 1): ").strip()
    format_choice = format_choice if format_choice else "1"
    
    if format_choice == "1":
        receive_json_messages(host, port)
    else:
        print("Invalid format selection")


if __name__ == "__main__":
    main()
