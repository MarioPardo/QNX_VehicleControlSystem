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

udp_comm = None


class SteeringSubsystem:
    def __init__(self, driver):
        self.driver = driver
        self.status = "HEALTHY"
        self.angle = 0.0
        
    def setSteering(self, angle):
        self.angle = angle
        self.driver.setSteeringAngle(angle)
    
    def receiveMessage(self, message):
        pass
    
    def sendStatus(self):
        global udp_comm
        if udp_comm:
            steering_angle_deg = self.angle * 540
            message = {
                "SteeringSystemStatus": {
                    "Status": self.status,
                    "ActualAngle": steering_angle_deg
                }
            }
            udp_comm.send_json_message(message)
    
    def sendMessage(self):
        pass


class BrakingSubsystem:
    def __init__(self, driver):
        self.driver = driver
        self.status = "HEALTHY"
        self.intensity = 0.0
        
    def setBraking(self, intensity):
        self.intensity = intensity
        self.driver.setBrakeIntensity(intensity)
    
    def receiveMessage(self, message):
        pass
    
    def sendMessage(self):
        pass


class ThrottleSubsystem:
    def __init__(self, driver):
        self.driver = driver
        self.status = "HEALTHY"
        self.value = 0.0
        
    def setThrottle(self, value):
        self.value = value
        self.driver.setThrottle(value)
    
    def receiveMessage(self, message):
        pass
    
    def sendMessage(self):
        pass

# Constants to tune driving feel
THROTTLE_INCREMENT = 0.02    
MAX_STEERING = 0.5          
STEERING_INCREMENT = 0.02


class VehicleManager:
    def __init__(self):
        global udp_comm
        
        self.driver = Driver()
        self.time_step = int(self.driver.getBasicTimeStep())
        self.driver.setGear(1)
        
        self.throttle = 0.0
        self.brake = 0.0
        self.target_steering = 0.0
        self.headlights_on = False
        
        self.keyboard = self.driver.getKeyboard()
        self.keyboard.enable(self.time_step)
        
        self.prev_key = -1
        
        self.steering_subsystem = SteeringSubsystem(self.driver)
        self.braking_subsystem = BrakingSubsystem(self.driver)
        self.throttle_subsystem = ThrottleSubsystem(self.driver)
        
        udp_comm = UDPCommunicator(send_host='127.0.0.1', send_port=5000, recv_port=5001)
        
        self.last_udp_send_time = 0
        self.udp_send_interval = 5.0

    def get_keyboard_commands(self):
        key = self.keyboard.getKey()
        
        # Default transient states
        indicator = Driver.INDICATOR_OFF
        self.brake = 0.0
        
        if key == ord('W'):
            self.throttle += THROTTLE_INCREMENT
        elif key == ord('S'):
            self.throttle -= THROTTLE_INCREMENT * 2
            self.brake = 1.0
        elif key == ord('A'):
            self.target_steering -= STEERING_INCREMENT 
        elif key == ord('D'):
            self.target_steering += STEERING_INCREMENT 
        else:
            self.target_steering *= 0.9
            self.throttle *= 0.95
        
        if key == ord('R'):
            self.driver.setGear(-1)
            
        elif key == ord('F'):
            self.driver.setGear(1)

        if key == ord('Q'):
            indicator = Driver.INDICATOR_LEFT
        elif key == ord('E'):
            indicator = Driver.INDICATOR_RIGHT
            
        if key == ord('L') and self.prev_key != key:
            self.headlights_on = not self.headlights_on
            
        self.prev_key = key
        
        self.throttle = max(min(self.throttle, 1.0), 0.0)
        self.target_steering = max(min(self.target_steering, MAX_STEERING), -MAX_STEERING)
        self.brake = max(min(self.brake, 1.0), 0.0)

        if self.throttle < 0.01 and self.brake == 0.0:
            self.throttle = 0.0
            self.brake = 0.1
        
        return {
            'throttle': self.throttle,
            'steering': self.target_steering,
            'brake': self.brake,
            'indicator': indicator,
            'headlights': self.headlights_on
        }

    def apply_commands(self, commands):
        self.throttle_subsystem.setThrottle(commands['throttle'])
        self.steering_subsystem.setSteering(commands['steering'])
        self.braking_subsystem.setBraking(commands['brake'])
        self.driver.setDippedBeams(commands['headlights'])

    def run(self):
        while self.driver.step() != -1:
            cmds = self.get_keyboard_commands()
            self.apply_commands(cmds)

            print("Throttle:", self.throttle)
            print("BBrake:", self.brake)
            
            current_time = self.driver.getTime()
            if current_time - self.last_udp_send_time >= self.udp_send_interval:
                self.steering_subsystem.sendStatus()
                self.last_udp_send_time = current_time
            
            received = udp_comm.receive_message()
            if received:
                print(f"UDP RECEIVED: {received}")

if __name__ == '__main__':
    controller = VehicleManager()
    controller.run()