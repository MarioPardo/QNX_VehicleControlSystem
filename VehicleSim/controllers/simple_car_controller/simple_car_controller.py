"""
Webots Vehicle Controller using the 'driver' library.
Controls:
W: Accelerate (increase throttle)
S: Brake
A/D: Steer Left / Right
Q/E: Left / Right Blinkers
L: Toggle Headlights
"""

from vehicle import Driver
from udp_communicator import UDPCommunicator

# Constants to tune driving feel
THROTTLE_INCREMENT = 0.02    
MAX_STEERING = 0.5          
STEERING_INCREMENT = 0.02  
class VehicleManager:
    def __init__(self):
        # Initialize the Driver API
        self.driver = Driver()
        self.time_step = int(self.driver.getBasicTimeStep())
        self.driver.setGear(1)  # Shift into 1st gear (Drive)
        
        # State variables for continuous inputs
        self.throttle = 0.0
        self.brake = 0.0
        self.target_steering = 0.0
        self.headlights_on = False
        
        # Setup Keyboard
        self.keyboard = self.driver.getKeyboard()
        self.keyboard.enable(self.time_step)
        
        # Track the last key pressed to handle toggles properly
        self.prev_key = -1
        
        # Initialize UDP communication
        self.udp_comm = UDPCommunicator(send_host='127.0.0.1', send_port=5000, recv_port=5001)
        
        # Timer for periodic UDP messages (send every 5 seconds)
        self.last_udp_send_time = 0
        self.udp_send_interval = 5.0  # seconds

    def get_keyboard_commands(self):
        key = self.keyboard.getKey()
        
        # Default transient states
        indicator = Driver.INDICATOR_OFF
        self.brake = 0.0
        
        # W for throttle, S for brake, A/D for steering
        if key == ord('W'):
            self.throttle += THROTTLE_INCREMENT
        elif key == ord('S'):
            self.throttle -= THROTTLE_INCREMENT * 2  # Cut throttle quickly when braking
            self.brake = 1.0  # Max brake intensity
        elif key == ord('A'):
            self.target_steering -= STEERING_INCREMENT 
        elif key == ord('D'):
            self.target_steering += STEERING_INCREMENT 
        else:
            # Auto-center steering and decay throttle if no keys are pressed
            self.target_steering *= 0.9
            self.throttle *= 0.95  # Simulate engine braking / coasting
        
        # Shift to Reverse
        if key == ord('R'):
            self.driver.setGear(-1)
            
        # Shift back to Drive
        elif key == ord('F'):
            self.driver.setGear(1)

        # Q/E for Blinkers
        if key == ord('Q'):
            indicator = Driver.INDICATOR_LEFT
        elif key == ord('E'):
            indicator = Driver.INDICATOR_RIGHT
            
        # L for Headlights (Toggle)
        if key == ord('L') and self.prev_key != key:
            self.headlights_on = not self.headlights_on
            
        self.prev_key = key
        
        # Clamp values to logical limits
        self.throttle = max(min(self.throttle, 1.0), 0.0)
        self.target_steering = max(min(self.target_steering, MAX_STEERING), -MAX_STEERING)
        self.brake = max(min(self.brake, 1.0), 0.0)

        # PREVENT IDLE CREEP: Apply a light brake if we are off the gas
        if self.throttle < 0.01 and self.brake == 0.0:
            self.throttle = 0.0
            self.brake = 0.1 # Just enough brake to hold the car against idle RPM
        
        # Return standard command payload
        return {
            'throttle': self.throttle,
            'steering': self.target_steering,
            'brake': self.brake,
            'indicator': indicator,
            'headlights': self.headlights_on
        }

    def apply_commands(self, commands):
        self.driver.setThrottle(commands['throttle'])
        self.driver.setSteeringAngle(commands['steering'])
        self.driver.setBrakeIntensity(commands['brake'])
        # self.driver.setIndicator(commands['indicator'])#TODO look into blinkers later
        self.driver.setDippedBeams(commands['headlights'])

    def run(self):
        # The main simulation loop
        while self.driver.step() != -1:
            cmds = self.get_keyboard_commands()
            self.apply_commands(cmds)

            print("Throttle:", self.throttle)
            print("BBrake:", self.brake)
            
            # Send steering status via UDP every 5 seconds
            current_time = self.driver.getTime()
            if current_time - self.last_udp_send_time >= self.udp_send_interval:
                # Build the steering status message
                steering_angle_deg = self.target_steering * 540  # Convert normalized steering to degrees
                message = {
                    "SteeringSystemStatus": {
                        "Status": "HEALTHY",
                        "ActualAngle": steering_angle_deg
                    }
                }
                self.udp_comm.send_json_message(message)
                self.last_udp_send_time = current_time

if __name__ == '__main__':
    controller = VehicleManager()
    controller.run()