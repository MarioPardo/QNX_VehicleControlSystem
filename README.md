# QNX Vehicle Control System

We implement a QNX-based vehicle control system that uses RTOS principles to ensure that critical components stay healthy and active under varying levels of stress on the system.

[Video Demo Link](https://drive.google.com/file/d/1uYeQUPKilvpn_ePVozu5UvaWOICw7Qpc/view?usp=sharing)
---

## Project Structure

The project is split into three main components:

### UserDashboard
A Python-based GUI that runs on the host PC. It displays live telemetry data received from the QNX system (speed, steering, system health, etc.) and allows the user to send control commands back to QNX. Communication is done over UDP using JSON payloads.

### QNX (QNX_files)
The core real-time control system, built to run on a QNX target platform (e.g., Raspberry Pi or a QNX VM). It manages multiple processes — including driving, braking, telemetry, and a watchdog — coordinating between them using QNX IPC. It receives vehicle state from Webots and sends telemetry to the dashboard, while also accepting control input.

This is the **core** of the system, with the main player being the watchdog process and architecture

### VehicleSim (Webots)
A Webots-based vehicle simulation that runs on the host PC. It simulates the physical vehicle (movement, physics, environment) and communicates vehicle state to the QNX system over UDP. QNX sends back control signals that Webots applies to the simulated vehicle.

---

## CommunicationTesters

The `CommunicationTesters/` directory contains standalone Python scripts for mocking and testing the UDP communication layer without needing the full system running:

- **UDPSenderTester.py** — Sends mock JSON data packets, simulating what Webots or the dashboard would transmit.
- **UDPReceiverTester.py** — Listens for incoming JSON packets and prints them, simulating the receiving end of QNX or the dashboard.

These are useful for verifying IP/port configuration and JSON payload structure in isolation.

---

## How to Run

### Prerequisites
- A computer capable of running **QNX Momentics IDE** and **Python 3**
- **Webots** installed on the host PC
- A QNX target platform (e.g., Raspberry Pi) or QNX VM for running the QNX system

### Steps

**1. Set Up IP Addresses**

Configure the IP addresses and ports across all three components so they can communicate over UDP:
- **Webots**: update the UDP Communicator controller settings
- **UserDashboard**: update the IP/port constants in the dashboard script
- **QNX**: update `QNX_files/src/includes/defs.h` with the correct addresses

All three must agree on which addresses and ports they send/receive on.

**2. Compile QNX for the Correct Platform**

Open the project in QNX Momentics and build for your target platform. For our project we used a **Raspberry Pi** as the target. Select the appropriate CPU architecture in the build configuration before compiling.

**3. Start the System**

The three components can be started in any order:

- **UserDashboard** — run `user_dashboard.py` on the host PC
- **QNX** — deploy and run the compiled binary on the target platform (Raspberry Pi or QNX VM)
- **Webots** — open the world file and run the simulation on the host PC

> **Note:** The Dashboard and Webots simulation both run on a regular host PC. The QNX control system runs on a separate target platform, such as a Raspberry Pi or a QNX-compatible virtual machine.
