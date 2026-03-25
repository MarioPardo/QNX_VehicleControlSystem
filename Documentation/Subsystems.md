# QNX Control System Subsystems

This document describes the subsystems within the QNX vehicle control system, their roles, and message flows between components.

---

## 1. Braking System (Priority 1)

### Overview
The braking system is the highest priority subsystem, responsible for safely decelerating the vehicle. The QNX control system receives brake pedal input from the dashboard, validates and processes the command, and sends appropriate braking instructions to the vehicle simulation.

### Component Roles

#### Dashboard
- Monitors brake pedal position
- Sends brake input percentage to QNX control system
- Displays brake system alerts to the user

#### QNX Control System
- Receives brake input from dashboard
- Validates braking commands based on current vehicle state
- Calculates safe braking levels
- Monitors brake system health status
- Generates alerts for brake system failures
- Ensures real-time response for critical braking scenarios

#### Vehicle Simulation
- Applies commanded braking force to vehicle dynamics
- Monitors brake system health
- Reports brake system status back to QNX

### Message Flows

**Dashboard → QNX**
```json
BrakingInput*: {
    Percentage: 0-100
}
```

**QNX → Vehicle Simulation**
```json
BrakingCommand: {
    Percentage: 0-100
}
```

**Vehicle Simulation → QNX**
```json
BrakingSystemStatus*: {
    Status: "HEALTHY" | "DEGRADED" | "FAILED",
    Temperature: float
}
```

**QNX → Dashboard**
```json
VehicleAlert: {
    System: "BRAKING",
    Severity: "WARNING" | "CRITICAL",
    Message: string
}
```

---

## 2. Throttle System (Priority 2)

### Overview
The throttle system controls vehicle acceleration. It processes accelerator pedal input and manages engine power output while monitoring system health and performance.

### Component Roles

#### Dashboard
- Monitors accelerator pedal position
- Sends throttle input percentage to QNX control system
- Displays throttle system alerts and status

#### QNX Control System
- Receives throttle input from dashboard
- Performs control logic and calculations for optimal throttle response
- Validates throttle commands against vehicle state and safety constraints
- Monitors throttle system health
- Coordinates with braking system (braking takes priority)

#### Vehicle Simulation
- Applies commanded throttle level to engine
- Simulates engine response and power delivery
- Monitors throttle system health
- Reports system status back to QNX

### Message Flows

**Dashboard → QNX**
```json
ThrottleInput*: {
    Percentage: 0-100
}
```

**QNX → Vehicle Simulation**
```json
ThrottleCommand: {
    Percentage: 0-100
}
```

**Vehicle Simulation → QNX**
```json
ThrottleSystemStatus*: {
    Status: "HEALTHY" | "DEGRADED" | "FAILED",
    ActualThrottle: 0-100,
    RPM: int,
    Temperature: float
}
```

**QNX → Dashboard**
```json
VehicleAlert: {
    System: "THROTTLE",
    Severity: "WARNING" | "CRITICAL",
    Message: string
}
```

---

## 3. Steering System (Priority 3)

### Overview
The steering system manages vehicle direction control. It processes steering input and ensures responsive and safe steering behavior while monitoring system health.

### Component Roles

#### Dashboard
- Monitors steering wheel position and angle
- Sends steering input to QNX control system
- Displays steering system alerts and status

#### QNX Control System
- Receives steering input from dashboard
- Validates steering commands based on vehicle speed and state
- Calculates appropriate steering response
- Monitors steering system health
- Limits excessive steering inputs at high speeds for safety

#### Vehicle Simulation
- Applies commanded steering angle to vehicle dynamics
- Simulates steering response and vehicle handling
- Monitors steering system health
- Reports system status back to QNX

### Message Flows

**Dashboard → QNX**
```json
SteeringInput*: {
    Angle: -540 to 540,
    Rate: float
}
```

**QNX → Vehicle Simulation**
```json
SteeringCommand: {
    Angle: -540 to 540
}
```

**Vehicle Simulation → QNX**
```json
SteeringSystemStatus*: {
    Status: "HEALTHY" | "DEGRADED" | "FAILED",
    ActualAngle: -540 to 540
}
```

**QNX → Dashboard**
```json
VehicleAlert: {
    System: "STEERING",
    Severity: "WARNING" | "CRITICAL",
    Message: string
}
```

---

## Timing Constraints

The QNX Control System ensures that vehicle systems send health reports at some intervals
Messages with a * beside them are those responsible for keeping these deadlines in this direction of communication

 - for example: ``` ThrottleCommand* ```  is the message responsible for telling the QNX control system that the pedals are working gine

1. **Priority 1 (Braking)**: Highest priority - every 10ms.
2. **Priority 2 (Throttle)**: Medium priority - every 30ms
3. **Priority 3 (Steering)**: Standard priority - every 60ms

For each communication path that has a required timing, the receiving HealthMonitor process is what will ensure that
subsystems are running as expected. 


