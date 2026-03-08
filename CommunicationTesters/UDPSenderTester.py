import socket
import json
import time


class QNXSender:
    def __init__(self):
        self.to_vehicle_sim_messages = [
            {"SteeringCommand": {"Angle": -90}}
        ]
        self.to_vehicle_sim_frequencies = [60] #how often (ms) we want each message sent
        
        self.to_dashboard_messages = []
        self.to_dashboard_frequencies = []


class DashboardTester:
    def __init__(self):
        self.messages = []
        self.frequencies = []


class VehicleSimSender:
    def __init__(self):
        self.messages = []
        self.frequencies = []


def send_messages(message, frequency_ms, host, port):
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    interval = frequency_ms / 1000.0
    
    print(f"\n[UDP Sender] Sending to {host}:{port}")
    print(f"[UDP Sender] Message: {json.dumps(message)}")
    print(f"[UDP Sender] Frequency: {frequency_ms}ms")
    print("[UDP Sender] Press Ctrl+C to stop\n")
    
    try:
        while True:
            message_json = json.dumps(message)
            message_bytes = message_json.encode('utf-8')
            sock.sendto(message_bytes, (host, port))
            print(f"[{time.strftime('%H:%M:%S')}] Message sent")
            time.sleep(interval)
            
    except KeyboardInterrupt:
        print("\n[UDP Sender] Stopped")
    finally:
        sock.close()


def main():
    print("=" * 50)
    print("UDP Sender Tester")
    print("=" * 50)
    
    print("\nSelect emulator type:")
    print("1) QNX")
    print("2) Dashboard")
    print("3) Vehicle Simulation")
    emulator_choice = input("Choice: ").strip()
    
    messages = []
    frequencies = []
    
    if emulator_choice == "1":
        qnx = QNXSender()
        print("\nSelect destination:")
        print("1) ToVehicleSim")
        print("2) ToDashboard")
        direction_choice = input("Choice: ").strip()
        
        if direction_choice == "1":
            messages = qnx.to_vehicle_sim_messages
            frequencies = qnx.to_vehicle_sim_frequencies
        elif direction_choice == "2":
            messages = qnx.to_dashboard_messages
            frequencies = qnx.to_dashboard_frequencies
        else:
            print("Invalid direction selection")
            return
            
    elif emulator_choice == "2":
        dashboard = DashboardTester()
        messages = dashboard.messages
        frequencies = dashboard.frequencies
        
    elif emulator_choice == "3":
        vehicle_sim = VehicleSimSender()
        messages = vehicle_sim.messages
        frequencies = vehicle_sim.frequencies
        
    else:
        print("Invalid emulator selection")
        return
    
    if not messages:
        print("\nNo messages available for this selection")
        return
    
    print("\nAvailable messages:")
    for i, msg in enumerate(messages):
        print(f"{i}) {json.dumps(msg)} - {frequencies[i]}ms")
    
    msg_choice = input("\nSelect message ID: ").strip()
    msg_id = int(msg_choice)
    
    if msg_id < 0 or msg_id >= len(messages):
        print("Invalid message ID")
        return
    
    host_input = input("\nEnter target IP address (default 127.0.0.1): ").strip()
    host = host_input if host_input else "127.0.0.1"
    
    port_input = input("Enter target port (default 5000): ").strip()
    port = int(port_input) if port_input else 5000
    
    send_messages(messages[msg_id], frequencies[msg_id], host, port)


if __name__ == "__main__":
    main()
