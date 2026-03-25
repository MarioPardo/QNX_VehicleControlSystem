import socket
import json
import time
import threading


class QNXSender:
    def __init__(self):
        self.to_vehicle_sim_messages = [
            {"VehicleControls": {"Drive": {"ThrottleLevel": 0.5, "Steering": 0.0, "toggleGear": "D"}}},
            {"VehicleControls": {"Brake": {"brakeLevel": 0.8}}},
        ]
        self.to_vehicle_sim_frequencies = [500, 500]

        self.to_dashboard_messages = []
        self.to_dashboard_frequencies = []


class DashboardTester:
    def __init__(self):
        self.messages = []
        self.frequencies = []


class VehicleSimSender:
    def __init__(self):
        self.messages = [
            {"SteeringSystemStatus": {"Status": "HEALTHY", "ActualAngle": 0.0}},
            {"BrakingSystemStatus": {"Status": "HEALTHY", "Temperature": 85.0}},
            {"ThrottleSystemStatus": {"Status": "HEALTHY", "ActualThrottle": 50.0, "RPM": 3000, "Temperature": 90.0}}
        ]
        self.frequencies = [60, 10, 30]


def send_single(message, frequency_ms, host, port):
    """Send a single message repeatedly at a given frequency."""
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    interval = frequency_ms / 1000.0

    print(f"  -> {json.dumps(message)} every {frequency_ms}ms")

    try:
        while True:
            data = json.dumps(message).encode('utf-8')
            sock.sendto(data, (host, port))
            time.sleep(interval)
    except KeyboardInterrupt:
        pass
    finally:
        sock.close()


def send_multiple(selected, messages, frequencies, host, port):
    """Send multiple messages concurrently, each on its own thread."""
    print(f"\n[UDP Sender] Sending to {host}:{port}")
    print(f"[UDP Sender] Sending {len(selected)} message(s):")
    for idx in selected:
        print(f"  [{idx}] {json.dumps(messages[idx])} every {frequencies[idx]}ms")
    print("[UDP Sender] Press Ctrl+C to stop\n")

    threads = []
    for idx in selected:
        t = threading.Thread(
            target=send_single,
            args=(messages[idx], frequencies[idx], host, port),
            daemon=True
        )
        threads.append(t)
        t.start()

    try:
        while True:
            time.sleep(1)
    except KeyboardInterrupt:
        print("\n[UDP Sender] Stopped")


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

    msg_choice = input("\nSelect message IDs (comma-separated, or 'all'): ").strip()

    if msg_choice.lower() == "all":
        selected = list(range(len(messages)))
    else:
        selected = [int(x.strip()) for x in msg_choice.split(",")]

    for idx in selected:
        if idx < 0 or idx >= len(messages):
            print(f"Invalid message ID: {idx}")
            return

    host_input = input("\nEnter target IP address (default 127.0.0.1): ").strip()
    host = host_input if host_input else "127.0.0.1"

    port_input = input("Enter target port (default 5001): ").strip()
    port = int(port_input) if port_input else 5001

    send_multiple(selected, messages, frequencies, host, port)


if __name__ == "__main__":
    main()
