/**
 * @file BoilerMonitor.ino
 * @author Ofer Shulami
 * @brief System for monitoring boiler status and battery levels using Dabble.
 * @version 1.1
 * @date 2025
 * * @details This program uses an LDR sensor to detect if the boiler light is on,
 * tracks the duration, and monitors battery voltage via an analog pin.
 * Notifications are sent to the Dabble mobile app.
 */

#define CUSTOM_SETTINGS
#define INCLUDE_TERMINAL_MODULE      
#define INCLUDE_NOTIFICATION_MODULE  
#include <Dabble.h>

/* --- Hardware Configuration --- */
/** @brief Digital pin connected to the LDR sensor (Active LOW) */
const uint8_t PIN_LDR = 2; 
/** @brief Analog pin for battery voltage divider */
const uint8_t PIN_BATTERY = A0; 

/* --- Timing & Threshold Constants --- */
/** @brief Maximum allowed runtime before alert (30 minutes) */
const uint32_t MAX_RUN_TIME_MS = 30UL * 60 * 1000; 
/** @brief Voltage threshold for low battery notification (Volts) */
const float VOLT_THRESHOLD_LOW = 7.2; 
/** @brief Voltage threshold to reset the battery alert (Volts) */
const float VOLT_THRESHOLD_OK  = 8.0;
/** @brief Delay between loop iterations (ms) */
const uint32_t LOOP_DELAY_MS = 3000;

/* --- Global State Variables --- */
int g_previousLdrState = -1;       ///< Stores the last LDR reading to detect changes
bool g_isBatteryAlertSent = false; ///< Flag to prevent spamming battery notifications
bool g_isLongRunAlertSent = false; ///< Flag to prevent spamming duration notifications
uint32_t g_boilerStartTime = 0;    ///< Timestamp (ms) when the boiler was turned on

/**
 * @brief Initializes serial communication, Dabble, and pin modes.
 */
void setup() {
  Serial.begin(9600);
  
  // Initialize Dabble with Bluetooth Baud rate and TX/RX pins
  Dabble.begin(9600, 3, 4); 
  
  pinMode(PIN_LDR, INPUT);
  
  // Initial check: if boiler is already on at startup
  if (digitalRead(PIN_LDR) == LOW) {
    g_boilerStartTime = millis();
    g_previousLdrState = LOW;
    Serial.println(F("Boiler detected as ON at startup."));
  }

  Serial.println(F("--- System Professional Initialization Complete ---"));
}

/**
 * @brief Main execution loop.
 * Processes inputs, monitors boiler status, and checks battery health.
 */
void loop() {
  // Sync with Dabble app
  Dabble.processInput(); 

  monitorBoilerStatus();
  monitorBatteryHealth();

  delay(LOOP_DELAY_MS); 
}

/**
 * @brief Reads LDR sensor and handles state changes and time alerts.
 */
void monitorBoilerStatus() {
  int currentLdrState = digitalRead(PIN_LDR);
  bool isBoilerOn = (currentLdrState == LOW);

  // 1. Detect State Change (ON/OFF)
  if (currentLdrState != g_previousLdrState) {
    if (isBoilerOn) {
      Notification.notifyPhone("The BOILER is ON 🔥");
      g_boilerStartTime = millis();  
      g_isLongRunAlertSent = false;    
    } else {
      Notification.notifyPhone("The BOILER is OFF 📴");
    }
    g_previousLdrState = currentLdrState;
  }

  // 2. Track Duration and Alert
  String statusDisplay = "OFF";
  if (isBoilerOn) {
    uint32_t elapsedMs = millis() - g_boilerStartTime;
    uint32_t minutesOn = elapsedMs / 60000;
    statusDisplay = "ON (" + String(minutesOn) + "m)";

    // Alert if running too long
    if (elapsedMs > MAX_RUN_TIME_MS && !g_isLongRunAlertSent) {
      Notification.notifyPhone("⚠️ Alert: Boiler ON for 30 min!");
      g_isLongRunAlertSent = true; 
    }
  }

  // 3. Log to Terminal (Status part)
  Terminal.print(F("Status: "));
  Terminal.print(statusDisplay);
}

/**
 * @brief Measures battery voltage and sends alerts if below threshold.
 */
void monitorBatteryHealth() {
  int rawValue = analogRead(PIN_BATTERY);
  
  // Calculation: (Raw * Vref / Res) * DividerRatio
  float voltage = rawValue * (5.0 / 1023.0) * 2; 
  
  // Map 7V-9V to 0-100%
  int batteryPercent = map(voltage * 100, 700, 900, 0, 100); 
  batteryPercent = constrain(batteryPercent, 0, 100);

  // 1. Update Terminal (Battery part)
  Terminal.print(F(" | Bat: "));
  Terminal.print(voltage, 1);
  Terminal.print(F("V ("));
  Terminal.print(batteryPercent);
  Terminal.println(F("%)"));

  // 2. Threshold Logic
  if (voltage < VOLT_THRESHOLD_LOW && !g_isBatteryAlertSent) {
     Notification.notifyPhone("Battery Low! Replace soon 🪫");
     g_isBatteryAlertSent = true; 
  }
  
  // Reset alert flag if battery is charged/replaced
  if (voltage > VOLT_THRESHOLD_OK) {
    g_isBatteryAlertSent = false;
  }
}