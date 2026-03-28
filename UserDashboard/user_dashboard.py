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

import pyqtgraph as pg

IP = "192.168.1.1"


class Dashboard(QWidget):

    packet_received = Signal(str)

    def __init__(self):
        super().__init__()

        self.setWindowTitle("QNX Vehicle Dashboard")
        self.setGeometry(200, 200, 520, 450)

        self.setFocusPolicy(Qt.StrongFocus)
        self.packet_received.connect(self.process_packet)

        # UDP socket
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.sock.bind(("192.168.1.1", 6000)) #listening
        self.server_address = ("192.168.1.2", 5000) #sending

        self.last_packet_time = time.time()

        # STATE (normalized)
        self.throttle = 0.0
        self.brake = 0.0
        self.steering = 0.0
        self.snow_mode = False
        self.gear = "D"

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

        # Gear display
        self.gear_label = QLabel("Gear: D")
        self.gear_label.setAlignment(Qt.AlignCenter)
        self.gear_label.setStyleSheet("font-size: 18px; color: #facc15;")

        # STATUS
        self.health_label = QLabel("QNX Control System CONNECTED")
        self.health_label.setAlignment(Qt.AlignCenter)
        self.health_label.setStyleSheet("color:green")

        # WARNING
        self.warning_label = QLabel("No warnings")
        self.warning_label.setAlignment(Qt.AlignCenter)
        self.warning_label.setStyleSheet("""
            font-size: 18px;
            padding: 8px;
            border-radius: 8px;
            background-color: #16a34a;
            color: white;
        """)

        # BUTTONS
        self.throttle_btn = QPushButton("Throttle")
        self.throttle_btn.pressed.connect(self.throttle_on)
        self.throttle_btn.released.connect(self.throttle_off)

        self.brake_btn = QPushButton("Brake")
        self.brake_btn.pressed.connect(self.brake_on)
        self.brake_btn.released.connect(self.brake_off)

        self.snow_btn = QPushButton("Snow Mode OFF")
        self.snow_btn.clicked.connect(self.toggle_snow_mode)

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

        # DATA FOR PLOTS
        self.packet_times = []
        self.time_data = []
        self.freq_data = []
        self.start_time = time.time()

        # PLOTS
        self.plot_widget = pg.GraphicsLayoutWidget()

        # FREQUENCY PLOT
        self.freq_plot = self.plot_widget.addPlot(title="Telemetry Frequency (Hz)")
        self.freq_plot.setLabel('left', 'Packets/sec', units='Hz')
        self.freq_plot.setLabel('bottom', 'Time', units='s')
        self.freq_plot.showGrid(x=True, y=True)
        self.freq_curve = self.freq_plot.plot(pen='y')

        self.plot_timer = QTimer()
        self.plot_timer.timeout.connect(self.update_plots)
        self.plot_timer.start(100)

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

        # CHAOS ROW
        chaos_layout = QHBoxLayout()
        chaos_label = QLabel("CHAOS")
        chaos_label.setStyleSheet("font-size: 16px; font-weight: bold; color: red;")
        chaos_layout.addWidget(chaos_label)
        chaos_subsystems = [
            ("Braking",        "braking_system"),
            ("Driving",        "driving_system"),
            ("Telemetry",      "telemetry_system"),
            ("Vehicle Sender", "vehicle_sender"),
            ("Client",         "client"),
        ]
        self.chaos_btns = {}
        for display_name, subsys_name in chaos_subsystems:
            btn = QPushButton(display_name)
            btn.setStyleSheet(button_style)
            btn.clicked.connect(lambda checked, s=subsys_name: self.trigger_chaos(s))
            chaos_layout.addWidget(btn)
            self.chaos_btns[subsys_name] = (btn, display_name, button_style)

        main_layout.addWidget(self.health_label)
        main_layout.addWidget(self.warning_label)
        main_layout.addLayout(speed_layout)
        main_layout.addWidget(self.speed_unit)
        main_layout.addWidget(self.gear_label)
        main_layout.addLayout(control_layout)
        main_layout.addLayout(chaos_layout)

        main_layout.addWidget(QLabel("Steering"))
        main_layout.addWidget(self.steering_slider)

        main_layout.addWidget(self.plot_widget)

        self.setLayout(main_layout)

        # Dark theme
        self.setStyleSheet("""
            QWidget {
                background-color: #0f172a;
                color: white;
                font-family: Arial;
            }
        """)

        self.start_network()

        self.control_timer = QTimer()
        self.control_timer.timeout.connect(self.send_controls)
        self.control_timer.start(200)

        self.health_timer = QTimer()
        self.health_timer.timeout.connect(self.check_connection)
        self.health_timer.start(1000)

        

    # KEYBOARD
    def keyPressEvent(self, event):
        if event.isAutoRepeat():
            return

        if event.key() == Qt.Key_W:
            self.throttle_on()
        elif event.key() == Qt.Key_Space:
            self.brake_on()
        elif event.key() == Qt.Key_R:
            self.toggle_gear()

    def keyReleaseEvent(self, event):
        if event.isAutoRepeat():
            return

        if event.key() == Qt.Key_W:
            self.throttle_off()
        elif event.key() == Qt.Key_Space:
            self.brake_off()

    # NETWORK
    def start_network(self):
        threading.Thread(target=self.network_listener, daemon=True).start()

    def network_listener(self):
        while True:
            try:
                data, _ = self.sock.recvfrom(1024)
                self.packet_received.emit(data.decode())
            except:
                continue

    # SEND CONTROLS (3s chaos)

    def send_controls(self):

        # BRAKE MESSAGE
        brake_msg = {
            "origin": "UserInput",
            "subsys": "Brake",
            "data": {
                "brake": {
                    "level": self.brake
                }
            }
        }

        # DRIVING MESSAGE
        driving_msg = {
            "origin": "UserInput",
            "subsys": "Drive",
            "data": {
                "steering": {
                    "angle": self.steering
                },
                "throttle": {
                    "level": self.throttle
                },
                "gear": self.gear,
                "snow": self.snow_mode
            }
        }

        # SEND
        self.sock.sendto(json.dumps(brake_msg).encode(), self.server_address)
        self.sock.sendto(json.dumps(driving_msg).encode(), self.server_address)



    
    # PROCESS TELEMETRY
    def process_packet(self, json_data):
        message = json.loads(json_data)

        if message["type"] == "VehicleTelemetry":

            data = message["data"]

            self.speed_label.setText(str(abs(int(data["Speed"]))))

            warnings = data.get("Warnings", [])
            if warnings:
                self.warning_label.setText("\n".join(warnings))
                self.warning_label.setStyleSheet("background-color:#dc2626; color:white;")
            else:
                self.warning_label.setText("No warnings")
                self.warning_label.setStyleSheet("background-color:#16a34a; color:white;")

            self.last_packet_time = time.time()

            now = time.time()

            # track packets (last 1 second)
            self.packet_times.append(now)
            self.packet_times = [t for t in self.packet_times if now - t <= 1]

            freq = len(self.packet_times)
            current_time = now - self.start_time

            self.time_data.append(current_time)
            self.freq_data.append(freq)

            MAX = 200
            self.time_data = self.time_data[-MAX:]
            self.freq_data = self.freq_data[-MAX:]

    def check_connection(self):
        if time.time() - self.last_packet_time > 3:
            self.health_label.setText("QNX Control System DISCONNECTED")
            self.health_label.setStyleSheet("color:red")
        else:
            self.health_label.setText("QNX Control System CONNECTED")
            self.health_label.setStyleSheet("color:green")

    # INPUTS
    def throttle_on(self):
        self.throttle = 1.0

    def throttle_off(self):
        self.throttle = 0.0

    def brake_on(self):
        self.brake = 1.0

    def brake_off(self):
        self.brake = 0.0

    def update_steering(self, value):
        self.steering = value / 540.0

    def toggle_gear(self):
        self.gear = "R" if self.gear == "D" else "D"
        self.gear_label.setText(f"Gear: {self.gear}")

    def toggle_snow_mode(self):
        self.snow_mode = not self.snow_mode
        self.snow_btn.setText("Snow Mode ON" if self.snow_mode else "Snow Mode OFF")

    # CHAOS TRIGGER — fire and forget, one packet; button stays lit for 3s
    def trigger_chaos(self, subsys):
        print(f"[CHAOS] Injecting chaos into {subsys}")
        msg = {"origin": "UserInput", "subsys": "Mode", "data": {"chaos": subsys}}
        self.sock.sendto(json.dumps(msg).encode(), self.server_address)

        btn, display_name, normal_style = self.chaos_btns[subsys]
        btn.setText(f"{display_name} — CHAOS ACTIVE")
        btn.setStyleSheet("font-size: 16px; padding: 8px; border-radius: 8px; background-color: #dc2626; color: white;")
        QTimer.singleShot(3000, lambda: (btn.setText(display_name), btn.setStyleSheet(normal_style)))

    def update_plots(self):
        self.freq_curve.setData(self.time_data, self.freq_data)

app = QApplication(sys.argv)
window = Dashboard()
window.show()
sys.exit(app.exec())
