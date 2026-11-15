# Before making the ESP32-S3 version public

Tick each item when done. When everything is ticked: push the `esp32s3` branch, open a PR to `main`, and merge.

- [x] **0. Compile both sketches** (arduino-cli 1.6.0, esp32 core 3.3.12, `esp32:esp32:esp32s3`): no errors, no warnings.
- [ ] **1. Test on hardware.** Startup message, boiler ON / OFF messages, reminder (set `REMINDER_MIN` to 2 for the test, then back to 60), daily alive message (set `ALIVE_H` low for the test).
- [ ] **2. Power test on 2S2P** (`tools/power_test_esp32s3`): `Brownouts: 0`, minimum 3V3 above ~3000 mV.
- [ ] **3. Calibrate the light sensor** potentiometer against the boiler's indicator light; confirm `LIGHT_ON_LEVEL`.
- [ ] **4. Remove the Power LEDs** (sensor + ESP32 board) and measure sleep current with a multimeter in series with the battery (target < 2 mA).
- [ ] **5. Photos and a new schematic** for the README (replace the "coming soon" line).
- [ ] **6. ntfy on both phones** subscribed to a new, random topic; high priority on the phone next to the boiler.
- [ ] **7. Charging method decided:** external 18650 charger, balanced series pairs. Update the README if it changes.
- [ ] **8. Final README pass:** remove the "work in progress" note, check every link.
