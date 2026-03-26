# Smart Boiler Status Monitor 🚿🔥

An Arduino-based IoT solution designed to monitor the operational status of a water boiler using light detection and send real-time notifications to a smartphone via Bluetooth. 

This project was born out of a practical need in a new apartment where the boiler control panel is located far from the living area, leading to unnecessary energy waste.

## 🌟 Key Features
* **Event-Driven Notifications:** Uses `previousLdrState` logic to ensure alerts are sent **only** on state changes (ON/OFF), preventing notification spam.
* **Intelligent Battery Monitoring:** Real-time tracking of the 9V power supply via a voltage divider to ensure system reliability.
* **Remote Debugging:** Integrated serial terminal via the Dabble app for live logs and system status.
* **Smartphone Integration:** Custom push notifications with emojis and status descriptions.

## 🔌 Hardware Configuration & Schematic
The system is powered by an **Arduino Nano** and utilizes a Bluetooth module (HC-05/06) for wireless communication.

### Schematic Diagram
![Circuit Schematic](schematic.jpeg)
*The circuit includes an LDR sensor for light detection and a voltage divider for battery safety.*

### Component List:
* **Microcontroller:** Arduino Nano.
* **Sensor:** LDR connected to **Analog Pin A0** (Pull-down with 10kΩ).
* **Power:** 9V Battery connected to **Vin**.
* **Battery Monitoring:** Voltage divider (2x 10kΩ resistors) connected to **Analog Pin A6**.
* **Communication:** HC-05/06 Bluetooth Module.

## 🧠 Technical Deep Dive

### 1. Battery Sensing (Voltage Divider)
Since the Arduino's analog pins can only handle up to 5V, we use two 10kΩ resistors in series to divide the 9V battery voltage by 2. This brings the input to a safe ~4.5V. 
The code then reconstructs the actual voltage using the formula: 
$$V_{actual} = (AnalogRead \times \frac{5.0}{1023}) \times 2$$

### 2. Light Detection (LDR)
Instead of simple digital HIGH/LOW, we use an analog threshold on pin **A0**. This allows the system to be calibrated for different ambient light levels or specific indicator light intensities.

## 💻 Software Snippet: Battery & Status Logic
```cpp
void loop() {
  Dabble.processInput();
  
  // 1. Check Boiler Status
  int ldrValue = analogRead(A0);
  bool currentStatus = (ldrValue > 500); // Adjustable threshold

  if (currentStatus != previousStatus) {
    Notification.setDescription(currentStatus ? "Boiler is ON 🔥" : "Boiler is OFF 📴");
    Notification.notify();
    previousStatus = currentStatus;
  }

  // 2. Battery Health Check
  float voltage = (analogRead(A6) * (5.0 / 1023.0)) * 2.0;
  if (voltage < 7.0) {
    Terminal.println("⚠️ Warning: Low Battery!");
  }
}
