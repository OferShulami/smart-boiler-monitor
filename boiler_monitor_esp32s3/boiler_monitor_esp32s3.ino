/*
  Smart Boiler Status Monitor 🚿🔥 — ESP32-S3 edition
  ----------------------------------------------------
  - Sleeps in deep sleep all the time.
  - Wakes instantly when the boiler indicator light changes (light module DO -> ext0 wakeup).
  - Sends push notifications to any number of phones via ntfy.sh (Android + iPhone app, free).
  - Reminder every REMINDER_MIN minutes while the boiler stays ON.
  - Daily "alive" message: if it stops arriving, the battery is empty.
  - Optional battery measurement (BATTERY_MONITOR, off by default — needs a voltage divider).

  Wiring (see docs/hardware.md):
    Batteries 2S2P (+/-)   -> S09 converter IN
    S09 3.3V OUT           -> 3V3 pin, GND -> GND   (NOT the 5V pin)
    Light module VCC / GND -> 3V3 / GND
    Light module DO        -> GPIO4   (RTC GPIO, used for wakeup)

  Setup: copy secrets.example.h to secrets.h (same folder) and fill in your values.
  Board: "ESP32S3 Dev Module" (Arduino-ESP32 core 3.x)
*/

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <time.h>
#include "esp_sleep.h"
#include "driver/rtc_io.h"
#include "secrets.h"   // WIFI_SSID, WIFI_PASS, NTFY_TOPIC

// ================== USER SETTINGS ==================
#define LIGHT_PIN        GPIO_NUM_4
#define LIGHT_ON_LEVEL   LOW      // LM393 light modules pull DO LOW when light is detected. Flip to HIGH if yours is opposite.

// Lower TX power = smaller current spikes = fewer brownouts.
// Options: WIFI_POWER_19_5dBm (max), WIFI_POWER_15dBm, WIFI_POWER_11dBm, WIFI_POWER_8_5dBm
#define TX_POWER         WIFI_POWER_11dBm

// Timing
#define REMINDER_MIN     30       // reminder interval while boiler is ON
#define ALIVE_H          24       // "alive" message interval (and the longest sleep)
#define RETRY_MIN        2        // retry interval if WiFi / sending failed
#define STABLE_MS        1500     // light must be stable this long (filters flicker)
#define WIFI_TIMEOUT_MS  15000

// ---- Battery measurement (optional) ----
// 0 = off (current hardware: no suitable resistors).
// 1 = on: needs a divider BAT+ -> R1 -> GPIO5 -> R2 -> GND, with the pin voltage below ~3.1V.
//     Note: a permanently connected divider drains the battery all the time — use large resistors.
#define BATTERY_MONITOR  0
#if BATTERY_MONITOR
#define BATT_PIN           5        // ADC1 (works together with WiFi)
#define CELLS_IN_SERIES    2        // 2 = 2S2P
#define DIVIDER_RATIO      3.0f     // (R1 + R2) / R2 — calibrate against a multimeter
#define LOW_PER_CELL       3.50f    // send "low battery" once
#define RECOVER_PER_CELL   3.80f    // after charging above this, the low alert can fire again
#define CRITICAL_PER_CELL  3.10f    // send last message and shut down to protect the cells
#endif
// ===================================================

// Survives deep sleep (cleared only on power loss / reset)
RTC_DATA_ATTR int8_t lastState    = -1;   // -1 unknown, 0 OFF, 1 ON
RTC_DATA_ATTR time_t onSince      = 0;
RTC_DATA_ATTR time_t lastReminder = 0;
RTC_DATA_ATTR time_t lastAlive    = 0;
#if BATTERY_MONITOR
RTC_DATA_ATTR bool   lowBattSent  = false;
#endif

// ---------- helpers ----------
int readLightStable() {
  int s = (digitalRead(LIGHT_PIN) == LIGHT_ON_LEVEL);
  uint32_t stableStart = millis(), start = millis();
  while (millis() - stableStart < STABLE_MS && millis() - start < 10000) {
    int now = (digitalRead(LIGHT_PIN) == LIGHT_ON_LEVEL);
    if (now != s) { s = now; stableStart = millis(); }
    delay(20);
  }
  return s;
}

#if BATTERY_MONITOR
float readBatteryVolts() {
  uint32_t sum = 0;
  for (int i = 0; i < 16; i++) { sum += analogReadMilliVolts(BATT_PIN); delay(2); }
  return (sum / 16.0f) / 1000.0f * DIVIDER_RATIO;
}

// Rough Li-ion state of charge from resting voltage per cell
int batteryPercent(float vCell) {
  const float v[] = {3.10, 3.30, 3.40, 3.50, 3.60, 3.70, 3.80, 3.90, 4.00, 4.10, 4.20};
  const int   p[] = {   0,    3,    6,   12,   22,   38,   52,   65,   78,   90,  100};
  if (vCell <= v[0])  return 0;
  if (vCell >= v[10]) return 100;
  for (int i = 1; i < 11; i++) {
    if (vCell < v[i]) return p[i-1] + (int)((vCell - v[i-1]) / (v[i] - v[i-1]) * (p[i] - p[i-1]));
  }
  return 100;
}
#endif

bool connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  WiFi.setTxPower(TX_POWER);
  uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < WIFI_TIMEOUT_MS) delay(100);
  return WiFi.status() == WL_CONNECTED;
}

// tags: comma-separated quoted list, e.g. "\"fire\""
bool sendNtfy(const String& title, const String& msg, const char* tags, int priority) {
  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  if (!http.begin(client, "https://ntfy.sh/")) return false;
  http.addHeader("Content-Type", "application/json");
  String body = String("{\"topic\":\"") + NTFY_TOPIC +
                "\",\"title\":\"" + title +
                "\",\"message\":\"" + msg +
                "\",\"tags\":[" + tags + "]" +
                ",\"priority\":" + priority + "}";
  int code = http.POST(body);
  http.end();
  Serial.printf("ntfy -> %d\n", code);
  return code == 200;
}

String minutesText(time_t seconds) {
  long m = seconds / 60;
  if (m < 60) return String(m) + " דקות";
  return String(m / 60) + " שעות ו-" + String(m % 60) + " דקות";
}

// ---------- main ----------
void setup() {
  Serial.begin(115200);
  delay(50);

  rtc_gpio_deinit(LIGHT_PIN);          // return the pin from RTC mode to normal GPIO
  pinMode(LIGHT_PIN, INPUT);

  const int    state = readLightStable();
  const time_t now   = time(nullptr);  // RTC clock keeps running during deep sleep

#if BATTERY_MONITOR
  analogSetPinAttenuation(BATT_PIN, ADC_11db);
  const float vBatt = readBatteryVolts();
  const float vCell = vBatt / CELLS_IN_SERIES;
  if (vCell > RECOVER_PER_CELL) lowBattSent = false;
  const bool critical = vCell < CRITICAL_PER_CELL;
  const bool lowDue   = !critical && !lowBattSent && vCell < LOW_PER_CELL;
  const String battTxt = " | סוללה: " + String(vBatt, 2) + "V (~" + batteryPercent(vCell) + "%)";
#else
  const bool critical = false;
  const bool lowDue   = false;
  const String battTxt = "";
#endif

  Serial.printf("wake cause=%d state=%d last=%d\n", esp_sleep_get_wakeup_cause(), state, lastState);

  const bool firstBoot   = (lastState < 0);
  const bool changed     = !firstBoot && state != lastState;
  const bool reminderDue = !firstBoot && !changed && state == 1 &&
                           (now - lastReminder) >= (REMINDER_MIN * 60 - 30);
  const bool aliveDue    = !firstBoot && (now - lastAlive) >= (ALIVE_H * 3600L - 60);

  const bool needSend = firstBoot || changed || reminderDue || aliveDue || lowDue || critical;
  bool failed = false;

  if (needSend) {
    if (!connectWiFi()) {
      failed = true;
    } else {
      if (firstBoot) {
        bool ok = sendNtfy("מוניטור הדוד הופעל",
                           String("מצב נוכחי: ") + (state ? "דלוק 🔥" : "כבוי") + battTxt,
                           "\"electric_plug\"", 3);
        if (ok) { lastState = state; onSince = now; lastReminder = now; lastAlive = now; } else failed = true;
      }

      if (changed) {
        bool ok;
        if (state) {
          ok = sendNtfy("הדוד נדלק 🔥", "הדוד התחיל לחמם מים", "\"fire\"", 4);
          if (ok) { lastState = 1; onSince = now; lastReminder = now; }
        } else {
          ok = sendNtfy("הדוד כבה 📴", "הדוד היה דלוק " + minutesText(now - onSince), "\"white_check_mark\"", 3);
          if (ok) { lastState = 0; }
        }
        if (!ok) failed = true;   // lastState not updated -> the change is detected again on retry
      }

      if (reminderDue) {
        bool ok = sendNtfy("⏰ הדוד עדיין דלוק", "הדוד דלוק כבר " + minutesText(now - onSince) + ". לכבות?",
                           "\"alarm_clock\"", 5);
        if (ok) lastReminder = now; else failed = true;
      }

      if (aliveDue) {
        bool ok = sendNtfy("✅ המוניטור פעיל",
                           String("מצב הדוד: ") + (state ? "דלוק" : "כבוי") + battTxt +
                           ". אם ההודעה הזו לא מגיעה יום שלם, צריך להחליף סוללות.",
                           "\"heartbeat\"", 2);
        if (ok) lastAlive = now; else failed = true;
      }

#if BATTERY_MONITOR
      if (lowDue) {
        bool ok = sendNtfy("🔋 סוללה חלשה", "מתח הסוללה" + battTxt + ". כדאי להטעין בקרוב.", "\"battery\"", 4);
        if (ok) lowBattSent = true; else failed = true;
      }

      if (critical) {
        sendNtfy("🪫 הסוללה ריקה", "מתח" + battTxt + ". המערכת נכבית להגנה על הסוללות. יש להטעין וללחוץ RESET.",
                 "\"warning\"", 5);
      }
#endif
    }
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
  }

  // ---------- go to sleep ----------
  if (critical) {
    Serial.println("critical battery -> sleeping forever");
    esp_deep_sleep_start();             // no wake sources: only RESET / power cycle wakes it
  }

  const time_t after = time(nullptr);
  long sleepSec = ALIVE_H * 3600L - (long)(after - lastAlive);      // until the next alive message
  if (lastState == 1) {
    long remaining = REMINDER_MIN * 60L - (long)(after - lastReminder);
    if (remaining < sleepSec) sleepSec = remaining;
  }
  if (sleepSec < 60) sleepSec = 60;
  if (failed && sleepSec > RETRY_MIN * 60L) sleepSec = RETRY_MIN * 60L;

  // Wake when the light changes away from the CURRENT physical state
  const int wakeLevel = state ? !LIGHT_ON_LEVEL : LIGHT_ON_LEVEL;
  rtc_gpio_pullup_dis(LIGHT_PIN);       // the module has its own pull-up
  rtc_gpio_pulldown_dis(LIGHT_PIN);
  esp_sleep_enable_ext0_wakeup(LIGHT_PIN, wakeLevel);
  esp_sleep_enable_timer_wakeup((uint64_t)sleepSec * 1000000ULL);

  Serial.printf("sleep: %lds, wake on level %d\n", sleepSec, wakeLevel);
  Serial.flush();
  esp_deep_sleep_start();
}

void loop() {}  // never reached
