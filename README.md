# Smart Boiler Status Monitor 🚿🔥

An Arduino-based IoT solution designed to monitor the operational status of a water boiler using light detection and send real-time notifications to a smartphone via Bluetooth.

## 🌟 Overview
This project addresses a common household inconvenience: forgetting the boiler is on or being unsure if the water is ready. By monitoring the boiler's physical indicator light with an LDR (Light Dependent Resistor), the system detects state changes and alerts the user immediately through the **Dabble** app.

## 🚀 Key Features
* **Event-Driven Notifications:** The system tracks state changes using `previousLdrState` logic. This ensures that a notification is sent **only** when the boiler is actually toggled ON or OFF, preventing notification spam.
* **Real-Time Smartphone Alerts:** Integration with the Dabble Notification module for push alerts with custom titles, descriptions, and emojis.
* **Live System Monitoring:** Includes a serial terminal interface accessible via the Dabble app for real-time debugging and status logs.
* **Energy Awareness:** Provides a simple way to monitor energy consumption remotely by tracking boiler activity.

## 🛠 Hardware Configuration
* **Microcontroller:** Arduino Uno / Nano (or any Dabble-compatible board).
* **Sensor:** LDR Sensor connected to **Digital Pin 2**.
* **Connectivity:** HC-05/HC-06 Bluetooth Module.
    * **TX** -> Pin 3 (RX)
    * **RX** -> Pin 4 (TX)
* **Logic Note:** In this configuration, a `LOW` signal represents light detection (Boiler ON), while a `HIGH` signal represents darkness (Boiler OFF).

## 💻 Software & Libraries
1. **Dabble Library:** Required for communication between Arduino and the smartphone app.
2. **Enabled Modules:**
   * `INCLUDE_TERMINAL_MODULE`
   * `INCLUDE_NOTIFICATION_MODULE`

## 🧠 Core Logic Snippet
The system's efficiency relies on detecting the **transition** between states rather than continuous polling:

```cpp
if (currentLdrState != previousLdrState) {
    if (currentLdrState == HIGH) {
        // Transition to Dark -> Boiler Light is OFF
        Notification.setDescription("The BOILER is OFF 📴");
        Notification.notify();
    } else {
        // Transition to Light -> Boiler Light is ON
        Notification.setDescription("The BOILER is ON 🔥");
        Notification.notify();
    }
    // Update state to prevent repetitive triggers
    previousLdrState = currentLdrState;
}
