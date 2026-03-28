"""
UDP Communication Module for Vehicle Control System.
Handles two-way UDP communication (intake and output channels).
"""

import socket
import json


class UDPCommunicator:
    """
    Generic UDP communication handler.
    Provides methods for sending and receiving JSON messages.
    """
    
    def __init__(self, send_host='192.168.1.2', send_port=5000, recv_port=6001):
        """
        Initialize UDP communicator with separate send and receive channels.
        
        Args:
            send_host (str): Target host address for outgoing messages
            send_port (int): Target port for outgoing messages
            recv_port (int): Local port for incoming messages
        """
        self.send_host = send_host
        self.send_port = send_port
        self.recv_port = recv_port
        
        # Create output socket for sending messages
        self.send_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        
        # Create input socket for receiving messages
        self.recv_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.recv_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.recv_socket.bind(('', self.recv_port))
        self.recv_socket.setblocking(False)  # Non-blocking mode for integration with main loop
        
        print(f"UDP Communicator initialized:")
        print(f"  Sending to {self.send_host}:{self.send_port}")
        print(f"  Listening on port {self.recv_port}")
    
    def send_json_message(self, message_dict):
        """
        Send a JSON message via UDP.
        
        Args:
            message_dict (dict): Message to send (will be converted to JSON)
        
        Returns:
            bool: True if message sent successfully, False otherwise
        """
        try:
            message_json = json.dumps(message_dict)
            message_bytes = message_json.encode('utf-8')
            self.send_socket.sendto(message_bytes, (self.send_host, self.send_port))
            return True
        except Exception as e:
            print(f"Error sending UDP message: {e}")
            return False
    
    def receive_message(self):
        """
        Attempt to receive a message from the input channel (non-blocking).
        
        Returns:
            dict or None: Received message as dict, or None if no message available
        """
        try:
            data, addr = self.recv_socket.recvfrom(1024)  # Buffer size of 1024 bytes
            print(f"[UDP RAW] from {addr}: {data}")
            message_json = data.decode('utf-8')
            message_dict = json.loads(message_json)
            return message_dict
        except socket.error:
            # No data available (non-blocking mode)
            return None
        except json.JSONDecodeError as e:
            print(f"Error decoding received JSON: {e}")
            return None
        except Exception as e:
            print(f"Error receiving UDP message: {e}")
            return None
    
    def close(self):
        """
        Close both UDP sockets.
        """
        self.send_socket.close()
        self.recv_socket.close()
        print("UDP Communicator closed")
