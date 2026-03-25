"""
Webots Vehicle Controller - UDP Controlled.
Receives VehicleControls messages via UDP and applies them to the vehicle.

Expected message format:
  {"VehicleControls": {"Brake": {"brakeLevel": 0.5}}}
  {"VehicleControls": {"Drive": {"ThrottleLevel": 0.5, "Steering": 0.0, "toggleGear": "D"}}}
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


def validate_brake(data):
    """Validate brake message. Returns brakeLevel float or None."""
    if not isinstance(data, dict):
        return None
    brake_level = data.get("brakeLevel")
    if not isinstance(brake_level, (int, float)):
        print("Invalid brakeLevel: not a number")
        return None
    return clamp(float(brake_level), 0.0, 1.0)


def validate_drive(data):
    """Validate drive message. Returns (throttle, steering, gear) or None."""
    if not isinstance(data, dict):
        return None

    throttle = data.get("ThrottleLevel")
    steering = data.get("Steering")
    gear = data.get("toggleGear")

    if not isinstance(throttle, (int, float)):
        print("Invalid ThrottleLevel: not a number")
        return None
    if not isinstance(steering, (int, float)):
        print("Invalid Steering: not a number")
        return None
    if gear not in ('R', 'D'):
        print(f"Invalid toggleGear: must be 'R' or 'D', got '{gear}'")
        return None

    throttle = clamp(float(throttle), 0.0, 1.0)
    steering = clamp(float(steering), -1.0, 1.0)
    return throttle, steering, gear


def parse_vehicle_controls(message):
    """Parse a VehicleControls message. Returns (msg_type, data) or (None, None)."""
    if not isinstance(message, dict):
        return None, None

    controls = message.get("VehicleControls")
    if not isinstance(controls, dict):
        return None, None

    if "Brake" in controls:
        brake_level = validate_brake(controls["Brake"])
        if brake_level is not None:
            return "Brake", brake_level

    if "Drive" in controls:
        result = validate_drive(controls["Drive"])
        if result is not None:
            return "Drive", result

    return None, None


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
            send_host='127.0.0.1', send_port=5000, recv_port=5001
        )

        self.steering_subsystem = SteeringSubsystem(self.driver)
        self.braking_subsystem = BrakingSubsystem(self.driver)
        self.throttle_subsystem = ThrottleSubsystem(self.driver)

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

        msg_type, data = parse_vehicle_controls(message)
        if msg_type is None:
            print(f"Invalid message received: {message}")
            return

        if msg_type == "Brake":
            print(f"UDP RECEIVED [Brake]: brakeLevel={data}")
            self.apply_brake(data)
        elif msg_type == "Drive":
            throttle, steering, gear = data
            print(f"UDP RECEIVED [Drive]: throttle={throttle}, steering={steering}, gear={gear}")
            self.apply_drive(throttle, steering, gear)

    def get_speed_kmh(self):
        """Get current vehicle speed in km/h as int."""
        speed = self.driver.getCurrentSpeed()
        if math.isnan(speed):
            return 0
        return int(speed)

    def send_brake_telemetry(self):
        brake_temp = int(self.braking_subsystem.temperature)
        message = {"VehicleData": {"Brake": {"brakeTemp": brake_temp}}}
        self.udp_comm.send_json_message(message)

    def send_drive_telemetry(self):
        speed = self.get_speed_kmh()
        message = {"VehicleData": {"Drive": {"speed": speed}}}
        self.udp_comm.send_json_message(message)

    def send_telemetry(self):
        """Send vehicle telemetry data every tick."""
        self.send_brake_telemetry()
        self.send_drive_telemetry()

    def run(self):
        while self.driver.step() != -1:
            self.process_udp()
            self.send_telemetry()


if __name__ == '__main__':
    controller = VehicleManager()
    controller.run()
