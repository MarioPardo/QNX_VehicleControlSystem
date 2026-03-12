import socket
import json
import time
import random

HOST = "127.0.0.1"
PORT = 5000

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.bind((HOST, PORT))

print("QNX ECU UDP server running...")

# Vehicle state
speed = 0.0
throttle = 0
brake = 0
steering = 0

safe_mode = False
snow_mode = False  

client_addr = None
last_heartbeat = time.time()

# RTOS task timers
last_brake_task = time.time()
last_throttle_task = time.time()
last_steering_task = time.time()

LOG_FILE = "vehicle_log.txt"


def log_event(msg):
    with open(LOG_FILE, "a") as f:
        f.write(f"{time.time()} : {msg}\n")


while True:

    # RECEIVE UDP MESSAGES
   
    try:
        sock.settimeout(0.001)
        data, addr = sock.recvfrom(1024)

        # Simulate packet loss (10%)
        if random.random() < 0.1:
            continue

        client_addr = addr
        message = json.loads(data.decode())

        msg_type = message["type"]

        if msg_type == "ThrottleInput":
            throttle = message["data"]["Percentage"]

        elif msg_type == "BrakingInput":
            brake = message["data"]["Percentage"]

        elif msg_type == "SteeringInput":
            steering = message["data"]["Angle"]

        elif msg_type == "SnowModeToggle":  
            snow_mode = message["data"]["Enabled"]

        elif msg_type == "Heartbeat":
            last_heartbeat = time.time()

    except socket.timeout:
        pass

    now = time.time()

    
    # PRIORITY 1: BRAKE TASK
    # runs every 10 ms
    
    if now - last_brake_task > 0.01:

        if brake > 0:
            throttle = 0   # Brake overrides throttle

        speed -= (brake / 100) * 12

        last_brake_task = now

    
    # PRIORITY 2: THROTTLE TASK
    # runs every 30 ms
   
    if now - last_throttle_task > 0.03:

        if snow_mode:
            speed += (throttle / 100) * 3   # slower acceleration in snow
        else:
            speed += (throttle / 100) * 6

        last_throttle_task = now

   
    # PRIORITY 3: STEERING TASK
    # runs every 60 ms
    
    if now - last_steering_task > 0.06:

        # steering safety limit
        if speed > 80:
            steering = steering * 0.5

        last_steering_task = now


    speed -= 0.2

    if speed < 0:
        speed = 0

    if speed > 120:
        speed = 120

    # Snow mode speed cap
    if snow_mode and speed > 50:
        speed = 50

    # HEALTH MONITOR

    if time.time() - last_heartbeat > 3:
        safe_mode = True
        throttle = 0

    if safe_mode and speed > 30:
        speed = 30

    # SENSOR STATUS
    wheel_ok = True
    if int(time.time()) % 12 == 0:
        wheel_ok = False

    # TELEMETRY MESSAGE
    
    telemetry = {
        "type": "VehicleTelemetry",
        "data": {
            "Speed": round(speed, 1),
            "Throttle": throttle,
            "Brake": brake,
            "SteeringAngle": steering,
            "SafeMode": safe_mode,
            "SnowMode": snow_mode 
        },
        "timestamp": time.time()
    }

    if client_addr:
        sock.sendto(json.dumps(telemetry).encode(), client_addr)

    log_event(
        f"speed={speed} throttle={throttle} brake={brake} snow_mode={snow_mode}"
    )

    time.sleep(0.01)