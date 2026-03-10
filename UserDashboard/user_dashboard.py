import sys
import json
import socket
import threading
import time

from PySide6.QtWidgets import (
    QApplication, QWidget, QLabel,
    QVBoxLayout, QHBoxLayout,
    QPushButton, QSlider
)
from PySide6.QtCore import Qt, Signal, QTimer


class Dashboard(QWidget):

    packet_received = Signal(str)

    def __init__(self):
        super().__init__()

        self.setWindowTitle("QNX Vehicle Dashboard")
        self.setGeometry(200, 200, 520, 450)

        self.packet_received.connect(self.process_packet)

        # UDP socket
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.sock.bind(("127.0.0.1", 6000))
        self.server_address = ("127.0.0.1", 5000)

        self.last_packet_time = time.time()

        # Vehicle state
        self.throttle = 0
        self.brake = 0
        self.steering = 0
        self.snow_mode = False

        # SPEED DISPLAY 
        self.speed_label = QLabel("0")
        self.speed_label.setAlignment(Qt.AlignCenter)
        self.speed_label.setStyleSheet("""
            font-size: 72px;
            font-weight: bold;
            color: #38bdf8;
        """)

        self.speed_unit = QLabel("km/h")
        self.speed_unit.setAlignment(Qt.AlignCenter)
        self.speed_unit.setStyleSheet("font-size: 20px; color: #94a3b8;")

        # QNX Control System STATUS 
        self.health_label = QLabel("QNX Control System CONNECTED")
        self.health_label.setAlignment(Qt.AlignCenter)
        self.health_label.setStyleSheet("color:green")
        

        # WARNING SECTION 
        self.warning_label = QLabel("No warnings")
        self.warning_label.setAlignment(Qt.AlignCenter)
        self.warning_label.setStyleSheet("""
            font-size: 18px;
            padding: 8px;
            border-radius: 8px;
            background-color: #16a34a;
            color: white;
        """)

        # CONTROL BUTTONS 
        self.throttle_btn = QPushButton("Throttle")
        self.throttle_btn.pressed.connect(self.throttle_on)
        self.throttle_btn.released.connect(self.throttle_off)

        self.brake_btn = QPushButton("Brake")
        self.brake_btn.pressed.connect(self.brake_on)
        self.brake_btn.released.connect(self.brake_off)
        

        # Snow mode toggle
        self.snow_btn = QPushButton("Snow Mode OFF")
        self.snow_btn.clicked.connect(self.toggle_snow_mode)

        # Button styling
        button_style = """
        QPushButton {
            font-size: 16px;
            padding: 8px;
            border-radius: 8px;
            background-color: #1e293b;
            color: white;
        }
        QPushButton:hover {
            background-color: #334155;
        }
        QPushButton:pressed {
            background-color: #0ea5e9;
        }
        """

        self.throttle_btn.setStyleSheet(button_style)
        self.brake_btn.setStyleSheet(button_style)
        self.snow_btn.setStyleSheet(button_style)
        # Steering slider
        self.steering_slider = QSlider(Qt.Horizontal)
        self.steering_slider.setMinimum(-540)
        self.steering_slider.setMaximum(540)
        self.steering_slider.valueChanged.connect(self.update_steering)

        # LAYOUT 
        main_layout = QVBoxLayout()

        speed_layout = QHBoxLayout()
        speed_layout.addStretch()
        speed_layout.addWidget(self.speed_label)
        speed_layout.addStretch()

        control_layout = QHBoxLayout()
        control_layout.addWidget(self.throttle_btn)
        control_layout.addWidget(self.brake_btn)
        control_layout.addWidget(self.snow_btn)

        main_layout.addWidget(self.health_label)
        main_layout.addWidget(self.warning_label)
        main_layout.addLayout(speed_layout)
        main_layout.addWidget(self.speed_unit)
        main_layout.addLayout(control_layout)

        main_layout.addWidget(QLabel("Steering"))
        main_layout.addWidget(self.steering_slider)

        self.setLayout(main_layout)

        # Dark Theme 
        self.setStyleSheet("""
            QWidget {
                background-color: #0f172a;
                color: white;
                font-family: Arial;
            }
        """)

        # NETWORK
        self.start_network()

        # Control messages every 100 ms
        self.control_timer = QTimer()
        self.control_timer.timeout.connect(self.send_controls)
        self.control_timer.start(100)

        # Health monitor
        self.health_timer = QTimer()
        self.health_timer.timeout.connect(self.check_connection)
        self.health_timer.start(1000)

    # NETWORK LISTENER 
    def start_network(self):
        thread = threading.Thread(target=self.network_listener, daemon=True)
        thread.start()

    def network_listener(self):

        while True:
            data, _ = self.sock.recvfrom(1024)
            self.packet_received.emit(data.decode())

    # SEND CONTROL MESSAGES 
    def send_controls(self):

        throttle_msg = {
            "type": "ThrottleInput",
            "data": {"Percentage": self.throttle},
            "timestamp": time.time()
        }

        brake_msg = {
            "type": "BrakingInput",
            "data": {"Percentage": self.brake},
            "timestamp": time.time()
        }

        steering_msg = {
            "type": "SteeringInput",
            "data": {"Angle": self.steering, "Rate": 1.0},
            "timestamp": time.time()
        }

        heartbeat = {"type": "Heartbeat"}

        self.sock.sendto(json.dumps(throttle_msg).encode(), self.server_address)
        self.sock.sendto(json.dumps(brake_msg).encode(), self.server_address)
        self.sock.sendto(json.dumps(steering_msg).encode(), self.server_address)
        self.sock.sendto(json.dumps(heartbeat).encode(), self.server_address)

    # PROCESS TELEMETRY
    def process_packet(self, json_data):

        message = json.loads(json_data)

        if message["type"] == "VehicleTelemetry":

            data = message["data"]

            speed = data["Speed"]
            snow_mode = data["SnowMode"]
            safe_mode = data["SafeMode"]

            self.speed_label.setText(str(int(speed)))

            # Snow mode button update
            if snow_mode:
                self.snow_btn.setText("Snow Mode ON")
                self.snow_mode = True
            else:
                self.snow_btn.setText("Snow Mode OFF")
                self.snow_mode = False

            # Warning logic
            if safe_mode:
                self.warning_label.setText("WARNING: VEHICLE IN SAFE MODE")
            elif speed > 100:
                self.warning_label.setText("WARNING: HIGH SPEED")
            else:
                self.warning_label.setText("No warnings")

            self.last_packet_time = time.time()

    # HEALTH MONITOR 
    def check_connection(self):

        if time.time() - self.last_packet_time > 3:
            self.health_label.setText("QNX Control System DISCONNECTED")
            self.health_label.setStyleSheet("color:red")
            self.warning_label.setText("WARNING: QNX Control System CONNECTION LOST")
        else:
            self.health_label.setText("QNX Control System CONNECTED")
            self.health_label.setStyleSheet("color:green")

    # DRIVER INPUT
    def throttle_on(self):
        self.throttle = 100

    def throttle_off(self):
        self.throttle = 0

    def brake_on(self):
        self.brake = 100

    def brake_off(self):
        self.brake = 0

    def update_steering(self, value):
        self.steering = value

    # SNOW MODE
    def toggle_snow_mode(self):

        self.snow_mode = not self.snow_mode

        msg = {
            "type": "SnowModeToggle",
            "data": {"Enabled": self.snow_mode},
            "timestamp": time.time()
        }

        self.sock.sendto(json.dumps(msg).encode(), self.server_address)


app = QApplication(sys.argv)
window = Dashboard()
window.show()
sys.exit(app.exec())