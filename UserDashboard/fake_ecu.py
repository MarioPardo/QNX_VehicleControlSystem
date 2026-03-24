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
gear = "D"  # NEW

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

        elif msg_type == "GearChange":   # NEW
            gear = message["data"]["Gear"]
            print(f"[ECU] Gear set to {gear}")

        elif msg_type == "Heartbeat":
            last_heartbeat = time.time()

    except socket.timeout:
        pass

    now = time.time()

    # PRIORITY 1: BRAKE TASK
    if now - last_brake_task > 0.01:

        if brake > 0:
            throttle = 0

        # Brake always reduces magnitude of speed
        if speed > 0:
            speed -= (brake / 100) * 12
        elif speed < 0:
            speed += (brake / 100) * 12

        last_brake_task = now

    # PRIORITY 2: THROTTLE TASK
    if now - last_throttle_task > 0.03:

        accel = (throttle / 100) * (3 if snow_mode else 6)

        if gear == "D":
            speed += accel
        elif gear == "R":
            speed -= accel  # reverse direction

        last_throttle_task = now

    # PRIORITY 3: STEERING TASK
    if now - last_steering_task > 0.06:

        if abs(speed) > 80:
            steering = steering * 0.5

        last_steering_task = now

    # Friction
    if speed > 0:
        speed -= 0.2
    elif speed < 0:
        speed += 0.2

    # Clamp speed
    if speed > 120:
        speed = 120
    if speed < -40:  # reverse limit
        speed = -40

    # Snow mode cap
    if snow_mode:
        if speed > 50:
            speed = 50
        if speed < -20:
            speed = -20

    # HEALTH MONITOR
    if time.time() - last_heartbeat > 3:
        safe_mode = True
        throttle = 0

    if safe_mode and abs(speed) > 30:
        speed = 30 if speed > 0 else -30

    # SENSOR STATUS
    wheel_ok = True
    if int(time.time()) % 12 == 0:
        wheel_ok = False

    # TELEMETRY
    telemetry = {
        "type": "VehicleTelemetry",
        "data": {
            "Speed": round(speed, 1),
            "Throttle": throttle,
            "Brake": brake,
            "SteeringAngle": steering,
            "SafeMode": safe_mode,
            "SnowMode": snow_mode,
            "Gear": gear   # NEW
        },
        "timestamp": time.time()
    }

    if client_addr:
        sock.sendto(json.dumps(telemetry).encode(), client_addr)

    log_event(
        f"speed={speed} throttle={throttle} brake={brake} gear={gear} snow_mode={snow_mode}"
    )

    time.sleep(0.01)
