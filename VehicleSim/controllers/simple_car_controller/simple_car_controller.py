"""
Webots Vehicle Controller - UDP Controlled.
Receives vehicle control messages via UDP and applies them to the vehicle.

Expected message format:
  {"subsys": "vehicle_sender", "data": {"throttle_level": 0.8, "brake_level": 0.0,
   "steering_level": 0.5, "snow_mode": 0, "toggleGear": "D"}}
"""

import math
from vehicle import Driver
from udp_communicator import UDPCommunicator

MAX_STEERING = 0.5


# --- Validation helpers ---

def clamp(value, min_val, max_val):
    return max(min(value, max_val), min_val)


def map_steering(steering_input):
    """Map steering input [-1, 1] to angle [-MAX_STEERING, MAX_STEERING]."""
    return steering_input * MAX_STEERING


def parse_vehicle_sender(message):
    """Parse a vehicle_sender message. Returns data dict or None."""
    if not isinstance(message, dict):
        return None
    if message.get("subsys") != "vehicle_sender":
        return None

    data = message.get("data")
    if not isinstance(data, dict):
        return None

    throttle = data.get("throttle_level")
    brake = data.get("brake_level")
    steering = data.get("steering_level")
    gear = data.get("toggleGear")

    if not isinstance(throttle, (int, float)):
        print("Invalid throttle_level")
        return None
    if not isinstance(brake, (int, float)):
        print("Invalid brake_level")
        return None
    if not isinstance(steering, (int, float)):
        print("Invalid steering_level")
        return None
    if gear not in ('R', 'D'):
        print(f"Invalid toggleGear: '{gear}'")
        return None

    return {
        "throttle": clamp(float(throttle), 0.0, 1.0),
        "brake":    clamp(float(brake), 0.0, 1.0),
        "steering": clamp(float(steering), -1.0, 1.0),
        "gear":     gear,
        "snow_mode": bool(data.get("snow_mode", 0)),
    }


# --- Subsystems ---

class SteeringSubsystem:
    def __init__(self, driver):
        self.driver = driver
        self.status = "HEALTHY"
        self.angle = 0.0

    def set_steering(self, angle):
        self.angle = angle
        self.driver.setSteeringAngle(angle)


class BrakingSubsystem:
    def __init__(self, driver):
        self.driver = driver
        self.status = "HEALTHY"
        self.intensity = 0.0
        self.temperature = 85.0

    def set_braking(self, intensity):
        self.intensity = intensity
        self.driver.setBrakeIntensity(intensity)


class ThrottleSubsystem:
    def __init__(self, driver):
        self.driver = driver
        self.status = "HEALTHY"
        self.value = 0.0
        self.rpm = 0
        self.temperature = 90.0

    def set_throttle(self, value):
        self.value = value
        self.driver.setThrottle(value)
        self.rpm = int(value * 6000)


# --- Vehicle Manager ---

class VehicleManager:
    def __init__(self):
        self.driver = Driver()
        self.time_step = int(self.driver.getBasicTimeStep())
        self.driver.setGear(1)

        self.udp_comm = UDPCommunicator(
            send_host='192.168.1.2', send_port=5000, recv_port=6001
        )

        self.steering_subsystem = SteeringSubsystem(self.driver)
        self.braking_subsystem = BrakingSubsystem(self.driver)
        self.throttle_subsystem = ThrottleSubsystem(self.driver)

        self._telem_every = max(1, 200 // self.time_step)  # steps per 200ms
        self._telem_counter = 0

    def apply_brake(self, brake_level):
        """Apply braking and cut throttle."""
        self.throttle_subsystem.set_throttle(0.0)
        self.braking_subsystem.set_braking(brake_level)

    def apply_drive(self, throttle, steering, gear):
        """Apply drive commands: throttle, mapped steering, gear. Releases brake."""
        gear_val = -1 if gear == 'R' else 1
        self.driver.setGear(gear_val)

        mapped_steering = map_steering(steering)
        self.steering_subsystem.set_steering(mapped_steering)
        self.throttle_subsystem.set_throttle(throttle)
        self.braking_subsystem.set_braking(0.0)

    def process_udp(self):
        """Check for incoming UDP messages and apply commands."""
        message = self.udp_comm.receive_message()
        if not message:
            return

        data = parse_vehicle_sender(message)
        if data is None:
            print(f"Invalid message received: {message}")
            return

        print(f"UDP RECEIVED: throttle={data['throttle']}, brake={data['brake']}, "
              f"steering={data['steering']}, gear={data['gear']}, snow_mode={data['snow_mode']}")

        if data["brake"] > 0.0:
            self.apply_brake(data["brake"])
        else:
            self.apply_drive(data["throttle"], data["steering"], data["gear"])

    def get_speed_kmh(self):
        """Get current vehicle speed in km/h as int."""
        speed = self.driver.getCurrentSpeed()
        if math.isnan(speed):
            return 0
        return int(speed)

    def send_brake_telemetry(self):
        brake_temp = int(self.braking_subsystem.temperature)
        message = {
            "origin": "VehicleData",
            "subsys": "Brake",
            "data": {"brakeTemp": brake_temp},
        }
        self.udp_comm.send_json_message(message)

    def send_drive_telemetry(self):
        speed = self.get_speed_kmh()
        message = {
            "origin": "VehicleData",
            "subsys": "Drive",
            "data": {"speed": speed},
        }
        print(f"[WEBOTS] sending drive telemetry: speed={speed}")
        self.udp_comm.send_json_message(message)

    def send_telemetry(self):
        """Send vehicle telemetry data every tick."""
        self.send_brake_telemetry()
        self.send_drive_telemetry()

    def run(self):
        while self.driver.step() != -1:
            self.process_udp()
            self._telem_counter += 1
            if self._telem_counter >= self._telem_every:
                self._telem_counter = 0
                self.send_telemetry()


if __name__ == '__main__':
    controller = VehicleManager()
    controller.run()
