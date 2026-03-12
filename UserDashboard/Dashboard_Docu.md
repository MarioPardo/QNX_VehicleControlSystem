***

Currently the vehicle controller is replaced by a Fake ECU program that generates real time telemetry using JSON messages over a TCP connection. Later this component will be replaced by the QNX control system running on a Raspberry Pi.

** System Architecture

Dashboard (Python GUI) ⇄ TCP Socket ⇄ Vehicle Controller (Fake ECU / Later QNX)

The dashboard acts as a client and the controller acts as a server.

The controller periodically sends telemetry: vehicle speed, sensor status, warning messages


**Libraries and Frameworks:

PySide6 (Qt)
Python socket library
JSON
Threading

**Requirements:

Python 3.10 

**Install dependencies:

python3 -m pip install PySide6

**How to Run:

Start the ECU (server): python3 fake_ecu.py
Start the Dashboard (client): python3 python_dashboard.py
