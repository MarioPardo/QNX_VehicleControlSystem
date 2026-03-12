# Demo code for setup of UDP socket listener and sender
# This code is for demonstration purposes only and may not be suitable for production use.
import socket
import threading
import time

# Yet to work on UDP connection from the python module 

# listener code
def udp_listener():
    listener_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    listener_socket.bind(( "127.0.0.1", 5556)) # listen on all interfaces on port 5556
    
    
    print("UDP listener is up and running...")

    while True:
        data, addr = listener_socket.recvfrom(1024)
        print(f"Received message from {addr}: {data.decode()}")
        response = f"Hello from the UDP listener! You said: {data.decode()}"
        listener_socket.sendto(response.encode(), addr)
# sender code
def udp_sender():   
    sender_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sender_address = ('127.0.0.1', 5555) # Send to localhost on port 5556 // Resloves since the vm host is also resolcing to 127.0.0.1

    for i in range(10):
        message = f"Hello UDP listener! This is message {i+1}"
        sender_socket.sendto(message.encode(), sender_address)
        print(f"Sent: {message}")
        
        time.sleep(1)
# Run the listener and sender in separate threads
if __name__ == "__main__":
    listener_thread = threading.Thread(target=udp_listener, daemon=True)
    sender_thread = threading.Thread(target=udp_sender, daemon=True)

    listener_thread.start()
    time.sleep(1)  # Ensure the listener is up before the sender starts
    sender_thread.start()

    sender_thread.join()    
    print("UDP sender has finished sending messages.")
    