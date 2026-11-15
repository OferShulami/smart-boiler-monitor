# Before making the ESP32-S3 version public

Tick each item when done. When everything is ticked, follow "Publish day" below.

## Checklist

- [x] **0. Compile both sketches** (arduino-cli 1.6.0, esp32 core 3.3.12, `esp32:esp32:esp32s3`): no errors, no warnings.
- [ ] **1. Test on hardware.** Startup message, boiler ON / OFF messages, reminder (set `REMINDER_MIN` to 2 for the test, then back to 30), daily alive message (set `ALIVE_H` low for the test).
- [ ] **2. Power test on 2S2P** (`tools/power_test_esp32s3`): `Brownouts: 0`, minimum 3V3 above ~3000 mV.
- [ ] **3. Calibrate the light sensor** potentiometer against the boiler's indicator light; confirm `LIGHT_ON_LEVEL`.
- [ ] **4. Remove the Power LEDs** (sensor + ESP32 board) and measure sleep current with a multimeter in series with the battery (target < 2 mA).
- [ ] **5. Photos of the finished build** for the README (wiring diagram is done: `docs/wiring.svg`).
- [ ] **6. ntfy on both phones** subscribed to a new, random topic; high priority on the phone next to the boiler.
- [x] **7. Charging method decided:** take all cells out, charge them all equally, put them back (in README).
- [ ] **8. Final README pass:** remove the "work in progress" note, check every link and that `docs/wiring.svg` renders on GitHub.

## Publish day

1. Rename the repo to `smart-boiler-monitor` (GitHub redirects the old URL):
   ```
   gh repo rename smart-boiler-monitor -R OferShulami/boiler
   ```
2. Point the local repo at the new URL:
   ```
   git remote set-url origin https://github.com/OferShulami/smart-boiler-monitor.git
   ```
3. Set the description and topics:
   ```
   gh repo edit OferShulami/smart-boiler-monitor --description "Battery-powered ESP32-S3 monitor that watches a water boiler's indicator light and sends ntfy push notifications when it turns on, off, or stays on too long." --add-topic esp32 --add-topic esp32-s3 --add-topic arduino --add-topic iot --add-topic ntfy --add-topic deep-sleep --add-topic home-automation
   ```
4. Push the branch and open a PR to `main`; merge after review.
