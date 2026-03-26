Smart Boiler Status Monitor 🚿🔥
An Arduino-based IoT solution designed to monitor the operational status of a water boiler using light detection and send real-time notifications to a smartphone via Bluetooth.

🌟 Overview
This project solves a common household problem: forgetting the boiler is on (or wondering if the water is ready). By monitoring the boiler's physical indicator light with an LDR (Light Dependent Resistor), the system detects state changes and alerts the user immediately through the Dabble app.

🚀 Key Features
Event-Driven Notifications: The system tracks state changes using previousLdrState logic. This ensures that a notification is sent only when the boiler is actually toggled ON or OFF, preventing spam.

Real-Time Smartphone Alerts: Integration with the Dabble Notification module for push alerts with custom titles, descriptions, and emojis.

Live System Monitoring: Includes a serial terminal interface accessible via the Dabble app for real-time debugging and status logs.

Energy Awareness: Helps prevent unnecessary energy consumption by providing a remote status check.

🛠 Hardware Configuration
Microcontroller: Arduino Uno / Nano (or any Dabble-compatible board).

Sensor: LDR Sensor connected to Digital Pin 2.

Connectivity: HC-05/HC-06 Bluetooth Module.

TX -> Pin 3 (RX)

RX -> Pin 4 (TX)

Logic Note: In this configuration, a LOW signal represents light detection (Boiler ON), while a HIGH signal represents darkness (Boiler OFF).

💻 Software & Libraries
Dabble Library: Required for communication between Arduino and the smartphone app.

Enabled Modules:

INCLUDE_TERMINAL_MODULE

INCLUDE_NOTIFICATION_MODULE
