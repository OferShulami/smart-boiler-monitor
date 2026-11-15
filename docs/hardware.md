# Hardware notes

## Power

- **2S2P, not 1S.** With a single cell (~3.7 V) the S09 sits right at its buck/boost changeover and the 3.3 V rail collapsed during WiFi transmit (brownouts). With 2S (6.2–8.4 V) the S09 always runs in buck mode.
- **Feed 3.3 V into the 3V3 pin**, not the 5V pin. The S09 output replaces the on-board regulator.
- **TX power is reduced to 11 dBm** to lower the current spikes during transmit.
- Verify the setup with `tools/power_test_esp32s3`: target is `Brownouts: 0` and a minimum 3V3 above ~3000 mV.
- **Biggest power drain while sleeping: the Power LEDs** (2–5 mA each). Remove them on the light sensor (after calibrating), and on the ESP32 board (Power LED only, not the RGB LED). Target total sleep current: < 2 mA.

## Pins

| GPIO | Use | Why this pin |
|---|---|---|
| 4 | Light sensor DO | RTC GPIO (0–21 on the S3), required for ext0 wakeup |
| 5 | Battery divider (optional, `BATTERY_MONITOR`) | ADC1 — ADC2 can't be used together with WiFi |
| 6 | 3V3 rail divider (power test only) | ADC1 |

**Avoid on the N16R8 module:** 35–37 (Octal PSRAM), 0 / 3 / 45 / 46 (strapping pins), 19 / 20 (USB).

## Light sensor

- LM393 modules pull **DO LOW when light is detected**, so `LIGHT_ON_LEVEL = LOW`.
- Before sleeping, the code arms ext0 wakeup on the level *opposite* to the current state, so any change wakes it immediately.
- The light must be stable for 1.5 s (`STABLE_MS`) before a state is accepted, which filters flicker.

## Battery measurement (currently off)

- The ESP32-S3 ADC reads up to ~3.1 V (11 dB attenuation) and the pins are **not 5 V tolerant**. A full 2S pack is 8.4 V, so a divider with a ratio of ≥ 3 is needed.
- A permanently connected divider drains current all the time: 3 × 1 kΩ ≈ 2.8 mA (too much), 3 × 10 kΩ ≈ 0.25 mA, 100 kΩ-range resistors are much better.
- **Never switch the bottom (GND side) of the divider** with the ADC pin above the switch: when the switch is open, the pin sees the full battery voltage and the ESP32 gets damaged.

## Software notes

- `time()` keeps running in deep sleep (RTC), so on-time is measured without NTP.
- `RTC_DATA_ATTR` variables survive deep sleep but reset on power loss or RESET.
- **State is saved only after a successful send.** If sending fails, the change is detected again on the next wake (retry after 2 minutes). Keep this pattern.
- The power test stores its brownout/crash counters in NVS (`Preferences`), because RTC memory may not survive a brownout.
