# Smart Boiler Status Monitor 🚿🔥

This Arduino-based IoT project monitors the operational status of a boiler using an LDR (Light Dependent Resistor) and sends real-time notifications to a smartphone via the **Dabble** app.

## Overview
The system detects the state of the boiler's indicator light. [cite_start]It is designed to be "event-driven," meaning it tracks the state changes to avoid flooding the phone with repetitive notifications[cite: 4]. [cite_start]Notifications are sent only when the boiler is turned ON or OFF[cite: 7, 9].

## Features
* [cite_start]**Real-time Alerts:** Sends push notifications with custom titles and descriptions[cite: 5, 6, 8].
* [cite_start]**State Management:** Uses a `previousLdrState` logic to ensure notifications are sent only upon a change in status[cite: 4, 10].
* [cite_start]**Serial Debugging:** Includes a terminal module to view logs via the Dabble app[cite: 1, 4].
* [cite_start]**Visual Emojis:** Uses icons in notifications (🔥 for ON, 📴 for OFF) for better user experience[cite: 6, 8].

## Hardware Configuration
* **Microcontroller:** Arduino (Compatible with Dabble).
* [cite_start]**Sensor:** LDR Sensor connected to **Digital Pin 2**[cite: 2].
* [cite_start]**Connectivity:** Bluetooth module (HC-05/HC-06) connected to **Pins 3 (RX) and 4 (TX)**[cite: 3].



## Software Requirements
1. [cite_start]**Dabble Library:** Must be installed in the Arduino IDE[cite: 2].
2. **Modules Enabled:**
   * [cite_start]`INCLUDE_TERMINAL_MODULE` [cite: 1]
   * [cite_start]`INCLUDE_NOTIFICATION_MODULE` [cite: 1]

## How it Works
1. [cite_start]The system initializes serial and Bluetooth communication at **9600 baud**[cite: 3].
2. [cite_start]The `loop()` function checks the LDR state every **500ms**[cite: 10].
3. [cite_start]If the state changes from `LOW` to `HIGH` (or vice versa), the `Notification` module is triggered[cite: 4].
4. [cite_start]The smartphone receives a notification with the title "Boiler Status" and the current operational state[cite: 5, 8].

## Code Snippet (Logic Example)
```cpp
if (currentLdrState != previousLdrState) {
    if (currentLdrState == HIGH) {
        Notification.setDescription("The BOILER is OFF 📴");
        Notification.notify();
    } else {
        Notification.setDescription("The BOILER is ON 🔥");
        Notification.notify();
    }
    previousLdrState = currentLdrState;
}
