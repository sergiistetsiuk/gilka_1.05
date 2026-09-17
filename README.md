# GILKA

GILKA is an ESP32-S3 desktop/portable device with a 128×64 OLED, rotary encoder, and buzzer. Its six sections provide a timer, network-synchronized clock, Cherkasy weather, a radiation simulation, regional air-alert status, and device settings.

The firmware uses Arduino with PlatformIO. The current target is an **ESP32-S3 Zero (ESP32-S3FH4R2, 4 MB flash)**. Configuration is available on the OLED and through a local web page.

## Features

| Section | Function | Short click | Rotate without holding |
| --- | --- | --- | --- |
| TACTICAL: TIMER | Stopwatch or 1/5/15/25-minute countdown | Start/pause; acknowledge completed alarm | Select/reset preset while paused |
| TIME & DATE | NTP clock with Kyiv timezone | Cycle 24-hour, 12-hour, and seconds formats | — |
| METEO: SECTOR | OpenWeatherMap weather for Cherkasy | Switch temperature/humidity and wind/pressure pages | — |
| DOSIMETER | Clearly labeled radiation **simulation** | Toggle uSv/h and approximate gamma uR/h | Change simulated dose |
| SURGE: ALERT | Alerts.in.ua status for Cherkasy Oblast | Mute current siren | — |
| SYS: SETTINGS | Sound, radar hold, display, Wi-Fi, OTA, battery | Open/save setting | Navigate/change value |

Timers continue across sections. Each elapsed minute produces a double beep; countdown completion produces a continuous alarm and flashing onboard red LED. The final alarm replaces the last minute's double beep. Sound settings can suppress these sounds.

Weather and alerts require user-provided API credentials. The dosimeter does **not** measure radiation or read SaveEcoBot. Radar and battery readings remain unconfigured until their wiring is specified.

## Hardware and wiring

Pin numbers below are GPIO numbers, not physical header positions.

| Connection | ESP32-S3 Zero |
| --- | --- |
| OLED SDA | GPIO1 |
| OLED SCL | GPIO2 |
| OLED GND | GND |
| Encoder A / CLK | GPIO5 |
| Encoder B / DT | GPIO4 |
| Encoder button / SW | GPIO6 |
| Encoder common / GND | GND |
| MH-FMD buzzer I/O | GPIO7 |
| MH-FMD buzzer VCC | **3V3** |
| MH-FMD buzzer GND | GND |
| Onboard RGB LED | GPIO21; no external LED required |

Use an OLED module compatible with 3.3V power and I²C logic, and connect its VCC to 3V3. Encoder inputs use internal pull-ups; contacts switch to ground. All connected modules share ground.

The tested MH-FMD module is **active-low**: GPIO7 HIGH is silent, LOW enables sound. The user confirmed operation with the module powered from 3.3V. The earlier 5V supply caused continuous sound with direct GPIO control; retain the working 3.3V wiring for this build.

Optional RCWL-0516 and battery-divider pins are not assigned. A 21700 charging/protection circuit is outside the firmware's scope; do not connect a cell directly to an ADC pin.

### OLED configuration

- SSD1306, 128×64 framebuffer; probes I²C addresses `0x3C` and `0x3D`.
- I²C clock: 100 kHz. Rotation: 180° (`2`). COM configuration: `0x12`.
- This particular panel showed missing alternating logical rows during testing. The firmware repeats each glyph row vertically with `setTextSize(1, 2)` as a readability workaround.
- Three text rows occupy the blue region; the separator is at y=47 and the yellow status row starts at y=48.

This workaround is specific to the tested display and does not establish that the underlying panel fault is repaired.

## Build and USB upload

Install PlatformIO Core or the PlatformIO extension for VS Code, open this project, and run commands from its root. PlatformIO installs the dependencies listed in [platformio.ini](platformio.ini).

```sh
# Compile normal firmware.
pio run -e esp32-s3-zero

# Find the connected board.
pio device list

# Upload; replace the port with your board's port.
pio run -e esp32-s3-zero -t upload --upload-port /dev/cu.usbmodem2101

# Observe the boot log.
pio device monitor --port /dev/cu.usbmodem2101 --baud 115200
```

If `pio` is not on PATH, the local installation used for this project is `~/.platformio/penv/bin/pio`. Close any serial monitor holding the port before uploading.

The PlatformIO board definition is `esp32-s3-devkitm-1`, with explicit **4 MB flash and partition overrides** for the actual Zero board. Keep those overrides; the generic board definition's displayed flash capacity is not this device's capacity.

## First setup

1. Power the board. If saved Wi-Fi cannot be reached during startup, it opens **GILKA-SETUP**.
2. Connect to that access point using **gilka1234**.
3. Open **http://192.168.4.1** if the captive portal does not appear, and save your Wi-Fi credentials.
4. On the same home network, open **http://gilka.local**, or use the IP shown in **SYS: SETTINGS → WI-FI INFO**.
5. In the web page's Sections form, enter the OpenWeatherMap key and Alerts.in.ua token. Blank fields keep existing keys; the remove checkboxes clear them.

Wi-Fi and device preferences persist in ESP32 NVS. The firmware retries a lost connection and opens the setup AP after approximately 30 seconds offline. You can also open it manually through **WI-FI CONFIG**.

The local portal uses HTTP: configure credentials on a trusted network. API requests use HTTPS with certificate verification; see [TLS trust roots](docs/TLS_ROOTS.md).

## Encoder controls

- **Hold + rotate:** highlight a section.
- **Release:** open the highlighted section.
- **Short click:** perform the section's action, or open/save a setting.
- **Rotate:** navigate a list or change the current value.
- **Hold without rotating for 1.2 seconds:** save/back within settings; open section selection from a section screen.

After the display turns off, the first interaction only wakes it. Release and interact again to navigate.

### Settings

| Setting | Behavior |
| --- | --- |
| SOUND | MUTE / LOW / MAX / CLICKS ONLY; LOW is the initial default |
| RADAR SENS | Software motion hold time, 5–120 seconds; requires a configured sensor |
| DISPLAY | Normal/dim contrast and dim/off timeouts |
| WI-FI CONFIG | Open the setup access point and captive portal |
| OTA UPDATE | Arm a temporary authenticated upload window |
| BATTERY | Voltage and approximate percentage when an ADC divider is configured |
| WI-FI INFO | Show network or active setup details |

The buzzer is silent while idle. LOW/MAX change pulse duration, **not true volume**. MUTE suppresses all sounds; CLICKS ONLY keeps encoder feedback while suppressing timer, alert, and simulated Geiger sounds. Visual alarms remain enabled.

Initial display settings are contrast 120, dim contrast 20, dim after 30 seconds, and off after 300 seconds. Saved preferences override these defaults. A zero timeout disables that transition; the setup AP keeps the screen awake.

## Online services

- **Time:** NTP with a default Kyiv POSIX timezone of `EET-2EEST,M3.5.0/3,M10.5.0/4`, editable on the web page.
- **Weather:** Cherkasy city (`Cherkasy,UA`), refreshed every 10 minutes, with 30-second error retries. Obtain an [OpenWeatherMap API key](https://openweathermap.org/api).
- **Air alerts:** Cherkasy Oblast, region `24`, polled every 30 seconds. Obtain an [Alerts.in.ua token](https://devs.alerts.in.ua/).

The firmware marks unavailable or stale data explicitly. A received active alert stays latched until a fresh no-alert response; muting stops its siren. An unknown state never means clear. Use official alert channels alongside this device.

Weather/alert requests run in a background task so network timeouts do not block encoder handling. Buzzer pulses end through a dedicated timer rather than waiting for the main loop.

## OTA updates

While connected to Wi-Fi, select **SYS: SETTINGS → OTA UPDATE**. The OLED shows the device IP and a temporary password for a 120-second upload window. Build the normal firmware and, from the same LAN, run:

```sh
~/.platformio/penv/bin/python \
  ~/.platformio/packages/framework-arduinoespressif32/tools/espota.py \
  -i DEVICE_IP -p 3232 -a DISPLAYED_PASSWORD \
  -f .pio/build/esp32-s3-zero/firmware.bin
```

Replace the IP/password placeholders with the OLED values. Leaving the screen or letting the window expire closes the listener. USB upload remains available. An actual OTA transfer has not yet been verified on this device.

## Tests and OLED diagnostics

Run the portable logic tests with a host C++ compiler:

```sh
clang++ -std=c++11 -Wall -Wextra -Werror -I include \
  test/oled_settings_test.cpp -o /tmp/gilka-oled-test
/tmp/gilka-oled-test

clang++ -std=c++11 -Wall -Wextra -Werror -I include \
  test/section_logic_test.cpp -o /tmp/gilka-section-test
/tmp/gilka-section-test
```

The tests cover settings validation, countdown pause/resume/completion, stopwatch minute events, millisecond rollover, freshness checks, and section index wrapping.

A separate OLED diagnostic replaces the running application temporarily, without modifying saved Wi-Fi/OLED preferences:

```sh
pio run -e oled-diagnostic -t upload --upload-port /dev/cu.usbmodem2101
```

Restore the full application afterwards:

```sh
pio run -e esp32-s3-zero -t upload --upload-port /dev/cu.usbmodem2101
```

`oled-diagnostic-32` is an experimental native 32-row test retained for investigation; it produced a black screen on the tested panel and is not the normal firmware.

## Project structure

| Path | Purpose |
| --- | --- |
| `src/main.cpp` | Hardware startup, Wi-Fi/captive portal, web server, encoder, OLED rendering and power management |
| `include/Sections.h` | Section screens, interaction, settings, alarms, and OTA |
| `include/SectionLogic.h` | Portable timer and freshness logic |
| `include/SectionServices.h` | Background weather and air-alert API clients |
| `include/EncoderBuzzer.h` | Active-low buzzer control and pulse timer |
| `include/OLEDSettings.h` | Display setting validation and normalization |
| `include/ServiceTrust.h` | Public CA certificates for outbound HTTPS |
| `src/oled_diagnostic*.cpp` | Standalone display diagnostics |
| `test/` | Host-side tests |
| `docs/SECTIONS.md` | Detailed controls, optional sensor configuration, and service behavior |
| `VERIFICATION.md` | Verification results and troubleshooting history; latest status is at the top |

## Current verification status

The six-section firmware compiled and uploaded successfully. Host tests passed, and the boot log confirmed OLED initialization, encoder initialization, HTTP server startup, and a Wi-Fi connection.

Physical section gestures, onboard LED color, live keyed API responses, and OTA transfer still need device-level confirmation. The computer could not reach the device's web page during the last check, so live web behavior remains unverified. Radar and battery require confirmed wiring; radiation readings remain simulated.

See [detailed section documentation](docs/SECTIONS.md) and the latest [verification record](VERIFICATION.md) before extending the firmware.
