import socket
import json
import time

HOST = "127.0.0.1"
PORT = 5000

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
sock.bind((HOST, PORT))

print("QNX ECU running...")

speed = 0.0
throttle = 0.0
brake = 0.0
steering = 0.0
gear = "D"

snow_mode = False
chaos_mode = "brake"

client_addr = None

while True:

    try:
        sock.settimeout(0.001)
        data, addr = sock.recvfrom(1024)
        client_addr = addr

        msg = json.loads(data.decode())

        if msg["type"] == "UserInput":

            subs = msg["data"]["Subsystem"]

            brake = subs["Brake"]["Level"]

            driving = subs["Driving"]
            throttle = driving["Throttle"]
            steering = driving["Steering"]
            gear = driving["Gear"]

            mode = subs["Mode"]
            snow_mode = mode["Snow"]
            chaos_mode = mode["Chaos"]

    except:
        pass

    # BRAKE
    if chaos_mode == "brake":
        pass
    else:
        if brake > 0:
            throttle = 0

        if speed > 0:
            speed -= brake * 12
        elif speed < 0:
            speed += brake * 12

    # THROTTLE
    accel = throttle * (3 if snow_mode else 6)

    if gear == "D":
        speed += accel
    else:
        speed -= accel

    # FRICTION
    if speed > 0:
        speed -= 0.2
    elif speed < 0:
        speed += 0.2

    speed = max(min(speed, 120), -40)

    if snow_mode:
        speed = max(min(speed, 50), -20)

    # WARNINGS
    warnings = []

    if chaos_mode == "brake":
        warnings.append("Brake system failure")

    if abs(speed) > 100:
        warnings.append("High speed")

    telemetry = {
        "type": "VehicleTelemetry",
        "data": {
            "Speed": round(speed, 1),
            "Mode": {
                "Snow": snow_mode,
                "Chaos": chaos_mode
            },
            "Warnings": warnings
        },
        "timestamp": time.time()
    }

    if client_addr:
        sock.sendto(json.dumps(telemetry).encode(), client_addr)

    time.sleep(0.01)
