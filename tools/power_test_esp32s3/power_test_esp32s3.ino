/*
  Power stress test — ESP32-S3 on battery + S09 converter
  --------------------------------------------------------
  Runs on battery only (no USB needed). Every cycle it:
    1. Boots (like a real wake from sleep)
    2. Measures the 3V3 rail while idle
    3. Connects WiFi and sends 3 ntfy messages (current spikes!)
    4. Sends a summary to your phone: lowest 3V3 voltage, previous reset reason,
       how many brownouts happened so far
    5. Deep sleeps TEST_CYCLE_SEC and repeats

  If the board browns out mid-send, the NEXT summary will say
  "prev reset: BROWNOUT" and the brownout counter goes up.

  Extra wiring for the test only:
    3V3 pin -> 1k -> GPIO6 -> 1k -> GND   (two equal resistors, ratio 2.0; ~1.65mA while connected)

  Setup: copy secrets.example.h to secrets.h (same folder) and fill in your values.
*/

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <Preferences.h>
#include "esp_system.h"
#include "esp_sleep.h"
#include "secrets.h"   // WIFI_SSID, WIFI_PASS, NTFY_TOPIC (same values as the main project)

// ================== SETTINGS ==================

#define RAIL_PIN        6
#define RAIL_RATIO      2.0f

// Round A/B: 19.5 dBm (max). Round C: try WIFI_POWER_11dBm or WIFI_POWER_8_5dBm
#define TX_POWER        WIFI_POWER_19_5dBm
#define TX_POWER_LABEL  "19.5dBm"

#define TEST_LABEL      "A - no capacitor"   // change per round so you can tell results apart
#define STRESS_MESSAGES 3
#define TEST_CYCLE_SEC  60
// ==============================================

volatile uint32_t railMin = 99999;
volatile uint32_t railMax = 0;

void railTask(void*) {
  for (;;) {
    uint32_t mv = analogReadMilliVolts(RAIL_PIN) * RAIL_RATIO;
    if (mv < railMin) railMin = mv;
    if (mv > railMax) railMax = mv;
    taskYIELD();
  }
}

void resetRail() { railMin = 99999; railMax = 0; }

const char* resetReasonText(esp_reset_reason_t r) {
  switch (r) {
    case ESP_RST_POWERON:   return "POWER ON";
    case ESP_RST_BROWNOUT:  return "BROWNOUT ⚠️";
    case ESP_RST_DEEPSLEEP: return "deep sleep (OK)";
    case ESP_RST_PANIC:     return "PANIC/crash ⚠️";
    case ESP_RST_INT_WDT:
    case ESP_RST_TASK_WDT:
    case ESP_RST_WDT:       return "WATCHDOG ⚠️";
    case ESP_RST_SW:        return "software reset";
    case ESP_RST_EXT:       return "RESET button";
    default:                return "other";
  }
}

bool sendNtfy(const String& title, const String& msg, int priority) {
  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  if (!http.begin(client, "https://ntfy.sh/")) return false;
  http.addHeader("Content-Type", "application/json");
  String body = String("{\"topic\":\"") + NTFY_TOPIC + "\",\"title\":\"" + title +
                "\",\"message\":\"" + msg + "\",\"tags\":[\"test_tube\"],\"priority\":" + priority + "}";
  int code = http.POST(body);
  http.end();
  return code == 200;
}

void setup() {
  Serial.begin(115200);

  // Count boots and brownouts in flash (survives brownout resets)
  esp_reset_reason_t reason = esp_reset_reason();
  Preferences prefs;
  prefs.begin("pwrtest", false);
  uint32_t boots     = prefs.getUInt("boots", 0) + 1;
  uint32_t brownouts = prefs.getUInt("brown", 0) + (reason == ESP_RST_BROWNOUT ? 1 : 0);
  uint32_t crashes   = prefs.getUInt("crash", 0) +
                       ((reason == ESP_RST_PANIC || reason == ESP_RST_WDT ||
                         reason == ESP_RST_INT_WDT || reason == ESP_RST_TASK_WDT) ? 1 : 0);
  prefs.putUInt("boots", boots);
  prefs.putUInt("brown", brownouts);
  prefs.putUInt("crash", crashes);
  prefs.end();

  analogSetPinAttenuation(RAIL_PIN, ADC_11db);
  xTaskCreatePinnedToCore(railTask, "rail", 2048, nullptr, 1, nullptr, 1);

  // 1) idle
  resetRail();
  delay(1500);
  uint32_t idleMin = railMin, idleMax = railMax;

  // 2) WiFi connect
  resetRail();
  uint32_t t0 = millis();
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  WiFi.setTxPower(TX_POWER);
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < 15000) delay(50);
  uint32_t connectMs = millis() - t0;
  bool wifiOk = WiFi.status() == WL_CONNECTED;
  uint32_t connectMin = railMin;

  // 3) stress: several HTTPS sends
  resetRail();
  int sentOk = 0;
  if (wifiOk) {
    for (int i = 0; i < STRESS_MESSAGES; i++) {
      if (sendNtfy("stress " + String(i + 1) + "/" + String(STRESS_MESSAGES), "load test", 1)) sentOk++;
    }
  }
  uint32_t sendMin = railMin;

  // 4) summary
  String msg = String("Test: ") + TEST_LABEL +
               "\\nTX power: " + TX_POWER_LABEL +
               "\\nPrev reset: " + resetReasonText(reason) +
               "\\nBoots: " + boots + " | Brownouts: " + brownouts + " | Crashes: " + crashes +
               "\\n3V3 idle: " + idleMin + "-" + idleMax + " mV" +
               "\\n3V3 min during WiFi connect: " + connectMin + " mV (" + connectMs + " ms)" +
               "\\n3V3 min during sending: " + sendMin + " mV" +
               "\\nStress msgs OK: " + sentOk + "/" + STRESS_MESSAGES;
  Serial.println(msg);
  if (wifiOk) sendNtfy("🧪 Power test result", msg, (brownouts > 0 ? 4 : 2));

  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  esp_sleep_enable_timer_wakeup((uint64_t)TEST_CYCLE_SEC * 1000000ULL);
  esp_deep_sleep_start();
}

void loop() {}
