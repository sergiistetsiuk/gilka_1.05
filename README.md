<a id="top"></a>

<h1 align="center">☢️ GILKA V1.05</h1>

<p align="center">
  <strong>A fan-made simulation of the «Гілка» (Gilka) detector</strong><br>
  Inspired by <em>S.T.A.L.K.E.R. 2: Heart of Chornobyl</em>
</p>

<p align="center">
  ESP32-S3 · OLED · Timer &amp; Clock · Weather · Remote Radiation Data · Ukraine Air Alerts
</p>

<p align="center">
  <a href="https://github.com/sergiistetsiuk/gilka_1.05/stargazers"><img src="https://img.shields.io/github/stars/sergiistetsiuk/gilka_1.05?style=for-the-badge&amp;logo=github" alt="GitHub stars"></a>
  <a href="https://github.com/sergiistetsiuk/gilka_1.05/forks"><img src="https://img.shields.io/github/forks/sergiistetsiuk/gilka_1.05?style=for-the-badge&amp;logo=github" alt="GitHub forks"></a>
  <a href="https://github.com/sergiistetsiuk/gilka_1.05/commits"><img src="https://img.shields.io/github/last-commit/sergiistetsiuk/gilka_1.05?style=for-the-badge&amp;logo=github" alt="Last commit"></a>
  <img src="https://img.shields.io/github/repo-size/sergiistetsiuk/gilka_1.05?style=for-the-badge&amp;logo=github" alt="Repository size">
</p>

<p align="center">
  <img src="https://img.shields.io/badge/ESP32--S3-E7352C?style=for-the-badge&amp;logo=espressif&amp;logoColor=white" alt="ESP32-S3">
  <img src="https://img.shields.io/badge/PlatformIO-F5822A?style=for-the-badge&amp;logo=platformio&amp;logoColor=white" alt="PlatformIO">
  <img src="https://img.shields.io/badge/Arduino-00878F?style=for-the-badge&amp;logo=arduino&amp;logoColor=white" alt="Arduino">
  <img src="https://img.shields.io/badge/C%2B%2B-00599C?style=for-the-badge&amp;logo=cplusplus&amp;logoColor=white" alt="C++">
</p>

<p align="center">
  <a href="#about-gilka">About</a> ·
  <a href="#features">Features</a> ·
  <a href="#hardware-and-wiring">Wiring</a> ·
  <a href="#build-and-usb-upload">Build &amp; Flash</a> ·
  <a href="#first-setup">Setup</a> ·
  <a href="#current-verification-status">Verification</a>
</p>

---

<a id="about-gilka"></a>

## 🎮 About GILKA

**GILKA V1.05** is a fan-made electronic simulation of the **«Гілка» (Gilka) detector** from **S.T.A.L.K.E.R. 2: Heart of Chornobyl**. It brings the idea of the fictional device into an ESP32-S3 project with physical controls, an OLED interface, sound, and useful everyday information functions.

This is an interpretation of the detector, not a claim of real artifact detection or a one-to-one reproduction of its in-game behavior. The documented firmware provides six sections: a timer, network-synchronized clock, Cherkasy weather, remote SaveEcoBot radiation measurements, regional air-alert status, and device settings.

The desktop/portable device uses a **128×64 OLED**, **rotary encoder**, and **MH-FMD buzzer**. The current target is an **ESP32-S3 Zero (ESP32-S3FH4R2, 4 MB flash)**, running **Arduino firmware built with PlatformIO**. Configuration is available on the OLED and through a local web page.

### From fictional detector to real electronics

| Aspect of the project         | GILKA implementation                                                          |
| ----------------------------- | ----------------------------------------------------------------------------- |
| Detector-inspired interface   | Six OLED sections and the **Гілка.ос / V1.05** startup screen                 |
| Physical interaction          | Rotary encoder with click, hold, and rotation gestures                        |
| Sound and light feedback      | Active-low buzzer, onboard RGB LED, and external GPIO8 indicator              |
| Radiation information         | Remote SaveEcoBot station records, with measurement time and freshness labels |
| Regional warnings             | Ukraine Alarm integration; unavailable data is not treated as an all-clear    |
| Everyday functions            | Stopwatch/countdown, NTP time, and weather                                    |
| Portable operation            | 1S battery voltage measurement and approximate charge indication              |
| Configuration and maintenance | Wi-Fi portal, saved settings, and a temporary authenticated OTA window        |

The **simulation** is the device concept and presentation. Environmental values are not fictional: DOSIMETER uses provider records and does not generate synthetic radiation readings, detector clicks, or flashes.

**Inspiration reference:** [Детектор «Гілка» — Ukrainian S.T.A.L.K.E.R. Wiki](https://stalker.fandom.com/uk/wiki/%D0%94%D0%B5%D1%82%D0%B5%D0%BA%D1%82%D0%BE%D1%80_%C2%AB%D0%93%D1%96%D0%BB%D0%BA%D0%B0%C2%BB).

> [!IMPORTANT]
> **This is not a local radiation sensor or an official alert device.** DOSIMETER displays remote station data; use official alert channels alongside GILKA. Implemented features and hardware-verified behavior are distinguished in [Current verification status](#current-verification-status).

> [!WARNING]
> **SaveEcoBot public access is blocked on the tested ESP32/network.** The project records an HTTP 403 browser challenge on **2026-09-19**. The authenticated API path still needs device verification. See [SaveEcoBot radiation data](#saveecobot-api-key) before relying on this section.

<details>
<summary><strong>📖 Full documentation index</strong></summary>

- [About GILKA](#about-gilka)
- [Features](#features)
- [Hardware and wiring](#hardware-and-wiring)
  - [OLED configuration](#oled-configuration)
  - [Startup screen](#startup-screen)
- [Local environment files](#local-environment-files)
- [Build and USB upload](#build-and-usb-upload)
- [First setup](#first-setup)
- [Encoder controls and settings](#encoder-controls)
- [Online services](#online-services)
- [SaveEcoBot radiation data](#saveecobot-api-key)
  - [City and search radius](#saveecobot-city-and-search-radius)
  - [Public JSON fallback](#public-json-fallback-without-a-key)
- [Ukraine Alarm v3 integration](#ukraine-alarm-v3-integration)
- [External indicator — GPIO8](#external-indicator-gpio8)
- [Battery indicator](#battery-indicator)
- [OTA updates](#ota-updates)
- [Tests and OLED diagnostics](#tests-and-oled-diagnostics)
- [Project structure](#project-structure)
- [Current verification status](#current-verification-status)
- [Attribution and fan-project notice](#attribution-and-fan-project-notice)

</details>

<a id="features"></a>

## ✨ Features

| Section         | Function                                                              | Short click                                         | Rotate without holding               |
| --------------- | --------------------------------------------------------------------- | --------------------------------------------------- | ------------------------------------ |
| TIME & DATE     | NTP clock with Kyiv timezone                                          | Cycle 24-hour, 12-hour, and seconds formats         | —                                    |
| TACTICAL: TIMER | Stopwatch or countdown in 30-second steps (00:30–99:30)               | Start/pause; acknowledge completed alarm            | Adjust/reset duration while paused   |
| METEO: SECTOR   | OpenWeatherMap weather for Cherkasy                                   | Switch temperature/humidity and wind/pressure pages | —                                    |
| DOSIMETER       | SaveEcoBot measurements near the configured city (capture date shown) | Toggle approximate gamma uR/h                       | Toggle measurement / distance and ID |
| SURGE: ALERT    | Ukraine Alarm status for Cherkasy Oblast                              | Mute current siren                                  | —                                    |
| SYS: SETTINGS   | Sound, display, Wi-Fi, OTA, battery                                   | Open/save setting                                   | Navigate/change value                |

Timers continue across sections. Each elapsed minute produces a double beep; countdown completion produces a short beep once per second and flashing onboard red LED. The final alarm replaces the last minute's double beep. Sound settings can suppress these sounds.

Weather and alerts require user-provided API credentials. The dosimeter displays remote SaveEcoBot records; it is not a local radiation sensor. Battery voltage is measured on GPIO9 through a 100 kΩ / 100 kΩ divider for a 1S, 6000 mAh Li-ion battery.

<a id="hardware-and-wiring"></a>

## 🔌 Hardware and wiring

Pin numbers below are GPIO numbers, not physical header positions.

| Connection                      | ESP32-S3 Zero                              |
| ------------------------------- | ------------------------------------------ |
| OLED VCC                        | **3V3**                                    |
| OLED SDA                        | GPIO1                                      |
| OLED SCL                        | GPIO2                                      |
| OLED GND                        | GND                                        |
| Encoder A / CLK                 | GPIO5                                      |
| Encoder B / DT                  | GPIO4                                      |
| Encoder button / SW             | GPIO6                                      |
| Encoder common / GND            | GND                                        |
| MH-FMD buzzer I/O               | GPIO7                                      |
| MH-FMD buzzer VCC               | **3V3**                                    |
| MH-FMD buzzer GND               | GND                                        |
| External status LED anode (+)   | GPIO8 through a 330 Ω series resistor      |
| External status LED cathode (−) | GND                                        |
| Battery divider midpoint        | GPIO9 (100 kΩ to battery +, 100 kΩ to GND) |
| Onboard RGB LED                 | GPIO21; no external LED required           |

Use an OLED module compatible with 3.3V power and I²C logic, and connect its VCC to 3V3. Encoder inputs use internal pull-ups; contacts switch to ground. All connected modules share ground.

The tested MH-FMD module is **active-low**: GPIO7 HIGH is silent, LOW enables sound. Operation was confirmed with the tested module powered from 3.3V. The earlier 5V supply caused continuous sound with direct GPIO control; retain the working 3.3V wiring for this build.

A 21700 charging/protection circuit is outside the firmware's scope; do not connect a cell directly to an ADC pin.

<a id="oled-configuration"></a>

### OLED configuration

- SSD1306, 128×64 framebuffer; probes I²C addresses `0x3C` and `0x3D`.
- I²C clock: 100 kHz. Rotation: 180° (`2`). COM configuration: `0x12`.
- This particular panel showed missing alternating logical rows during testing. The firmware repeats each glyph row vertically with `setTextSize(1, 2)` as a readability workaround.
- Three text rows occupy the blue region; the separator is at y=47 and the yellow status row starts at y=48.

This workaround is specific to the tested display and does not establish that the underlying panel fault is repaired.

<a id="startup-screen"></a>

### Startup screen

At startup, the OLED displays **Гілка.ос** across the blue region and **V1.05** in the yellow region for two seconds. The entire background is lit with dark lettering. Normal section screens follow with their usual colors. The title uses custom Cyrillic pixel glyphs and retains the display row workaround.

<a id="local-environment-files"></a>

## 🔐 Local environment files

[.env.example](.env.example) is the committed template; `.env` is the private local copy and is ignored by Git. Secret fields start empty. On another checkout, create it with:

```sh
cp .env.example .env
chmod 600 .env
```

### Firmware defaults and saved settings

PlatformIO reads `.env` automatically before building and embeds `WIFI_SSID`, `WIFI_PASSWORD`, `OPENWEATHERMAP_API_KEY`, `UKRAINEALARM_API_KEY`, `SAVEECOBOT_API_KEY`, and `DEVICE_TIMEZONE` as firmware defaults. Rebuild and upload after editing them. A missing `.env` builds with empty credentials and the default Kyiv timezone.

**Existing NVS settings take priority.** A new device uses the embedded defaults; an already configured device keeps its saved values. Use the web page to change saved settings. Clearing an API key through the web page stores an empty override, so a compiled key does not unexpectedly return. Firmware defaults are not automatically copied into NVS.

### File format and credential handling

The loader accepts `KEY=value`, quoted values, comments, and optional `export`; it does not execute shell commands or expand `$VARIABLES`. UTF-8 values are supported for Wi-Fi. Quote passwords containing spaces or `#`. Invalid values stop the build without printing secrets.

Generated defaults are kept inside ignored `.pio` build output, not command-line compiler flags. Credentials are embedded in firmware binaries, which must also remain private. `PIO_ENV`, `UPLOAD_PORT`, `MONITOR_BAUD`, `DEVICE_URL`, and `OTA_*` remain local command helpers, not firmware defaults. OTA continues to generate its temporary password on the device.

### Local command helpers

To use the local build/upload variables from a trusted `.env`:

```sh
set -a
source .env
set +a
pio run -e "$PIO_ENV" -t upload --upload-port "$UPLOAD_PORT"
pio device monitor --port "$UPLOAD_PORT" --baud "$MONITOR_BAUD"
```

Use shell quotes around values containing spaces or special characters; single quotes preserve literal `$` characters. Do not commit `.env` or put real credentials in `.env.example`. OTA passwords are temporary and must match the password currently displayed by the device.

The `.env` loader tests can be run with `python3 test/env_config_test.py`.

<a id="build-and-usb-upload"></a>

## 🔨 Build and USB upload

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

<a id="first-setup"></a>

## 📡 First setup

1. Power the board. If saved Wi-Fi cannot be reached during startup, it opens **GILKA-SETUP**.
2. Connect to that access point using **gilka1234**.
3. Open **http://192.168.4.1** if the captive portal does not appear, and save your Wi-Fi credentials.
4. On the same home network, open **http://gilka.local**, or use the IP shown in **SYS: SETTINGS → WI-FI INFO**.
5. In the web page's Sections form, enter the OpenWeatherMap key and Ukraine Alarm token. Blank fields keep existing keys; the remove checkboxes clear them.

Wi-Fi and device preferences persist in ESP32 NVS. The firmware retries a lost connection and opens the setup AP after approximately 30 seconds offline. You can also open it manually through **WI-FI CONFIG**.

The local portal uses HTTP: configure credentials on a trusted network. API requests use HTTPS with certificate verification; see [TLS trust roots](docs/TLS_ROOTS.md).

<a id="encoder-controls"></a>

## 🎛️ Encoder controls

| Gesture                                   | Action                                                                  |
| ----------------------------------------- | ----------------------------------------------------------------------- |
| **Hold + rotate**                         | Highlight a section                                                     |
| **Release**                               | Open the highlighted section                                            |
| **Short click**                           | Perform the section's action, or open/save a setting                    |
| **Rotate**                                | Navigate a list or change the current value                             |
| **Hold without rotating for 1.2 seconds** | Save/back within settings; open section selection from a section screen |

After the display turns off, the first interaction only wakes it. Release and interact again to navigate.

<a id="settings"></a>

### Settings

| Setting      | Behavior                                                   |
| ------------ | ---------------------------------------------------------- |
| SOUND        | MUTE / LOW / MAX / CLICKS ONLY; LOW is the initial default |
| DISPLAY      | Normal/dim contrast and dim/off timeouts                   |
| WI-FI CONFIG | Open the setup access point and captive portal             |
| OTA UPDATE   | Arm a temporary authenticated upload window                |
| BATTERY      | 1S battery voltage and approximate percentage via GPIO9    |
| WI-FI INFO   | Show network or active setup details                       |

The buzzer is silent while idle. LOW/MAX change pulse duration, **not true volume**. MUTE suppresses all sounds; CLICKS ONLY keeps encoder feedback while suppressing timer and alert sounds. Visual alarms remain enabled.

Initial display settings are contrast 120, dim contrast 20, dim after 30 seconds, and off after 300 seconds. Saved preferences override these defaults. A zero timeout disables that transition; the setup AP keeps the screen awake.

<a id="online-services"></a>

## 🌐 Online services

- **Time:** NTP with a default Kyiv POSIX timezone of `EET-2EEST,M3.5.0/3,M10.5.0/4`, editable on the web page.
- **Weather:** Cherkasy city (`Cherkasy,UA`), refreshed every 10 minutes, with 30-second error retries. Obtain an [OpenWeatherMap API key](https://openweathermap.org/api).
- **Radiation:** Remote SaveEcoBot station data for the configured city and radius; see [SaveEcoBot radiation data](#saveecobot-api-key) for authentication, freshness, and the documented public-access limitation.
- **Air alerts:** Cherkasy Oblast, resolved by region name; change index checked every 60 seconds. Obtain an [Ukraine Alarm token](https://api.ukrainealarm.com/).

The firmware marks unavailable or stale data explicitly. A received active alert stays latched until a fresh no-alert response; muting stops its siren. An unknown state never means clear. Use official alert channels alongside this device.

Weather/alert requests run in a background task so network timeouts do not block encoder handling. Buzzer pulses end through a dedicated timer rather than waiting for the main loop.

<a id="saveecobot-api-key"></a>

## ☢️ SaveEcoBot radiation data

> [!WARNING]
> **Device limitation verified 2026-09-19:** although desktop retrieval of the public JSON succeeds, SaveEcoBot returns an HTTP 403 Cloudflare browser challenge to this ESP32. Firmware shows `Public blocked 403` and retries after ten minutes. The public fallback is therefore not currently usable on this device/network. Obtain an official API key for the authenticated path; authenticated device access still needs verification.

### API key and displayed measurements

Set `SAVEECOBOT_API_KEY="your-key"` in `.env`, then rebuild/upload. Alternatively save the key on the device web page. Saved NVS `ecoKey` takes precedence; clearing it selects public JSON fallback. The old Safecast key is not reused.

DOSIMETER selects the nearest station within the configured radius of the selected city and displays gamma dose rate in uSv/h, UTC measurement time, and distance/station ID details. ARCHIVE marks provider-old or >24-hour measurements; STALE marks cached readings after failed/offline refreshes. No simulated radiation sounds are generated.

Get access through the [SaveEcoBot API page](https://www.saveecobot.com/en/static/api). See [implementation and tests](docs/SECTIONS.md#saveecobot-dosimeter).

<a id="saveecobot-city-and-search-radius"></a>

### SaveEcoBot city and search radius

```dotenv
SAVEECOBOT_STATION="Cherkasy, Cherkasy Oblast"
SAVEECOBOT_RADIUS_KM=25
```

`SAVEECOBOT_STATION` is a Ukrainian city name, not a sensor ID. Ukrainian or English names are accepted by [Open-Meteo geocoding](https://open-meteo.com/en/docs/geocoding-api), based on GeoNames. Add an oblast after a comma for namesakes; ambiguous or missing results stop station lookup with a configuration message. Only Ukrainian populated places are accepted. No API credentials are sent to the geocoder.

`SAVEECOBOT_RADIUS_KM` is the maximum distance from the resolved city centre, in kilometres (greater than 0, at most 500; decimals supported). Firmware selects the nearest SaveEcoBot station inside that circle; it does not average all stations. No station within the radius means no data. Coordinates and selection are cached until reboot/configuration change. Rebuild/upload after editing these values. `SAVEECOBOT_STATION_ID` has been replaced and is no longer used.

<a id="public-json-fallback-without-a-key"></a>

### Public JSON fallback without a key

When the effective API key (NVS first, then `.env`) is empty, the worker reads `SAVEECOBOT_PUBLIC_JSON_URL` instead of calling the authenticated API:

```dotenv
SAVEECOBOT_API_KEY=""
SAVEECOBOT_PUBLIC_JSON_URL="https://www.saveecobot.com/en/station/22800.json"
```

This default is the public radiation station on vul. Hoholia, Cherkasy. It is a specific fallback station, not automatic public station discovery. Its coordinates must be inside `SAVEECOBOT_RADIUS_KM` of the configured city; if the city changes, select another public station URL. No national map download occurs on the device. Requests occur every ten minutes, with errors retried after one minute. A nonempty but rejected API key does not silently fall back.

Only public `last_data[].phenomenon == gamma` is accepted, in nSv/h, divided by 1000 for uSv/h. The project verification record reports a check of the station response against the map's `gamma_t` UNIX timestamp: `2026-09-19 06:12:00` corresponds to `1789798320` UTC. Preserve source time and `is_old`; never substitute fetch time. OLED footer shows PUBLIC for this source and retains ARCHIVE/STALE labels. Missing/malformed/out-of-radius gamma data produces an error or explicitly stale cached reading, never generated values.

The actual public response is in `test/fixtures/saveecobot_public_22800.json`. Data attribution: [SaveEcoBot](https://www.saveecobot.com/). Public JSON usage is described on its [API page](https://www.saveecobot.com/en/static/api).

<a id="ukraine-alarm-v3-integration"></a>

## 🚨 Ukraine Alarm v3 integration

The supplied `swagger.json` describes [Ukraine Alarm / Stfalcon](https://api.ukrainealarm.com/), not the previous provider. Configure:

```dotenv
UKRAINEALARM_API_KEY=""
UKRAINEALARM_REGION_NAME="Черкаська область"
```

The token is sent verbatim in `Authorization`, with no Bearer prefix. Old Alerts.in.ua credentials are not reused. The web page stores this provider's key in separate NVS `uaAlarmKey`, which takes priority over the compiled `.env` default. Blank form input preserves it; Remove clears it. Rebuild/upload after changing `.env`.

### Region selection, polling, and freshness

The worker resolves the matching State ID from `/api/v3/regions`, retaining only state names/types/IDs while parsing. It checks `/api/v3/alerts/status` every 60 seconds, loads `/api/v3/alerts/{regionId}` initially or when `lastActionIndex` changes, and also refreshes full state every five minutes. A successful unchanged-index check refreshes connection freshness. Data is stale after 180 seconds without verification, immediately after a request error, or while offline.

### Threat states and muting

AIR, ARTILLERY, URBAN_FIGHTS, CHEMICAL, NUCLEAR and CUSTOM activate the existing buzzer/LED alarm with an appropriate OLED label. INFO displays an informational status without the siren. An explicit empty activeAlerts array for the selected region clears the alarm; null/missing/unknown data does not. A latched threat survives communication errors and invalid responses. Encoder click mutes sound; LED continues until a confirmed clear state. Existing sound mode settings apply. This device supplements official alerts.

### Parser tests

Run the parser tests:

```sh
clang++ -std=c++11 -Wall -Wextra -Werror -Iinclude \
  -I.pio/libdeps/esp32-s3-zero/ArduinoJson/src \
  test/ukraine_alarm_test.cpp -o /tmp/gilka-alert-test
/tmp/gilka-alert-test
python3 test/env_config_test.py
```

<a id="ukraine-alarm-sos-signalling"></a>

### SOS signalling

The Ukraine Alarm v3 integration notes specify that sound and both LEDs repeat synchronized SOS (three short, three long, three short). Dot: 200 ms; dash: 600 ms; symbol pause: 200 ms; letter pause: 600 ms; repeat pause: 1400 ms. Each new alert starts at the first dot. Mute silences the buzzer while LED SOS continues. Timer signalling is unchanged.

> [!NOTE]
> The general GPIO8 indicator notes also describe a 350 ms on / 350 ms off pattern for a latched air alert. These descriptions differ; see the [indicator timing note](#indicator-timing-confirmation) and confirm the active behavior on the device.

<a id="external-indicator-gpio8"></a>

## 💡 External indicator (GPIO8)

Connect GPIO8 → 330 Ω resistor → LED anode (+), and LED cathode (−) → GND. HIGH turns the LED on; it is initialized off at boot.

| State                          | Documented indication                                                                                                                                                                               |
| ------------------------------ | --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| Running countdown or stopwatch | A short flash each elapsed second, nominally **150 ms**, including while another section is open. Pausing stops the heartbeat.                                                                      |
| Timer completion               | The general indicator notes specify **350 ms on / 350 ms off**, with priority over ordinary indications. Muting sound does not clear visual alerts.                                                 |
| Latched air alert              | **Two patterns are documented:** the general indicator notes say **350 ms on / 350 ms off**; the Ukraine Alarm v3 notes specify synchronized **SOS** on both LEDs. See the confirmation note below. |
| DOSIMETER                      | Remote measurements only; no synthetic detector clicks or flashes.                                                                                                                                  |
| Otherwise                      | Off. The existing onboard RGB alarm indication remains available.                                                                                                                                   |

Patterns use the main loop without added delays; display or other loop work may affect exact flash timing. The added GPIO8 LED's physical wiring and brightness need device confirmation.

<a id="indicator-timing-confirmation"></a>

> [!WARNING]
> **Air-alert signal timing needs confirmation.** The source documentation contains both the 350/350 ms description and the [Ukraine Alarm v3 SOS description](#ukraine-alarm-sos-signalling). The active air-alert pattern needs firmware/device verification. Confirm the actual behavior and record the result in [VERIFICATION.md](VERIFICATION.md). Muting is documented as silencing the buzzer without clearing the visual alert.

<a id="battery-indicator"></a>

## 🔋 Battery indicator

The confirmed battery is **1S Li-ion, 6000 mAh**, up to 4.2 V. Connect battery + → 100 kΩ → GPIO9 → 100 kΩ → battery − / common GND. Equal resistors halve the voltage: firmware multiplies the ADC millivolts by two.

A three-segment battery icon replaces GILKA in the lower-left status bar. The approximate rising thresholds are listed below; below the first threshold the outline is empty. A 30 mV hysteresis and averaging stabilize the display. Invalid readings show `?`, not a full battery. The startup title/version remain unchanged.

| Rising battery voltage threshold | Icon           |
| -------------------------------- | -------------- |
| Below **3.40 V**                 | Empty outline  |
| **3.40 V**                       | One segment    |
| **3.70 V**                       | Two segments   |
| **3.95 V**                       | Three segments |

These are rising thresholds, not fixed instantaneous ranges; the documented hysteresis affects transitions. Invalid readings show `?`.

SYS: SETTINGS → BATTERY shows measured voltage, rated 6000 mAh capacity, and an approximate percentage (linear 3.2–4.2 V estimate). This is not coulomb counting, a charging-status detector, or a runtime estimate. Load and chemistry affect the relationship between voltage and charge.

Readings use `analogReadMilliVolts`, 16-sample averaging, and a once-per-second update. A 100 nF ceramic capacitor between GPIO9 and GND can reduce ADC noise, as described in [Espressif ADC guidance](https://docs.espressif.com/projects/esp-idf/en/v4.4.7/esp32s3/api-reference/peripherals/adc.html). Compare the displayed voltage with a multimeter before relying on the charge estimate.

<a id="ota-updates"></a>

## 🔄 OTA updates

While connected to Wi-Fi, select **SYS: SETTINGS → OTA UPDATE**. The OLED shows the device IP and a temporary password for a 120-second upload window. Build the normal firmware and, from the same LAN, run:

```sh
~/.platformio/penv/bin/python \
  ~/.platformio/packages/framework-arduinoespressif32/tools/espota.py \
  -i DEVICE_IP -p 3232 -a DISPLAYED_PASSWORD \
  -f .pio/build/esp32-s3-zero/firmware.bin
```

Replace the IP/password placeholders with the OLED values. Leaving the screen or letting the window expire closes the listener. USB upload remains available. An actual OTA transfer has not yet been verified on this device.

<a id="tests-and-oled-diagnostics"></a>

## 🧪 Tests and OLED diagnostics

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

<a id="project-structure"></a>

## 📁 Project structure

| Path                        | Purpose                                                                                          |
| --------------------------- | ------------------------------------------------------------------------------------------------ |
| `src/main.cpp`              | Hardware startup, Wi-Fi/captive portal, web server, encoder, OLED rendering and power management |
| `include/Sections.h`        | Section screens, interaction, settings, alarms, and OTA                                          |
| `include/SectionLogic.h`    | Portable timer and freshness logic                                                               |
| `include/SectionServices.h` | Background weather and air-alert API clients                                                     |
| `include/EncoderBuzzer.h`   | Active-low buzzer control and pulse timer                                                        |
| `include/OLEDSettings.h`    | Display setting validation and normalization                                                     |
| `include/ServiceTrust.h`    | Public CA certificates for outbound HTTPS                                                        |
| `src/oled_diagnostic*.cpp`  | Standalone display diagnostics                                                                   |
| `test/`                     | Host-side tests                                                                                  |
| `docs/SECTIONS.md`          | Detailed controls, optional sensor configuration, and service behavior                           |
| `VERIFICATION.md`           | Verification results and troubleshooting history; latest status is at the top                    |

Build configuration is in [platformio.ini](platformio.ini); local configuration starts from [.env.example](.env.example). HTTPS certificate details are in [docs/TLS_ROOTS.md](docs/TLS_ROOTS.md).

<a id="current-verification-status"></a>

## 📋 Current verification status

The six-section firmware compiled and uploaded successfully. Host tests passed, and the boot log confirmed OLED initialization, encoder initialization, HTTP server startup, and a Wi-Fi connection.

The status below summarizes the recorded project checks. It does **not** mean that every implemented feature has been validated on the device; consult [VERIFICATION.md](VERIFICATION.md) for the latest record.

| Area                               | Recorded status                                                                                                                             |
| ---------------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------- |
| Firmware build and USB upload      | Successful for the six-section firmware                                                                                                     |
| Host logic tests                   | Passed in the recorded checks                                                                                                               |
| Boot initialization                | OLED, encoder, HTTP server startup, and Wi-Fi connection confirmed by the boot log                                                          |
| Physical section gestures          | Still need device-level confirmation                                                                                                        |
| Onboard RGB LED color              | Still needs device-level confirmation                                                                                                       |
| External GPIO8 LED                 | Physical wiring, brightness, and the conflicting air-alert pattern descriptions need device confirmation                                    |
| Live keyed API responses           | Still need device-level confirmation                                                                                                        |
| SaveEcoBot public JSON             | Desktop retrieval succeeds, but the ESP32 receives a Cloudflare HTTP 403 browser challenge on the tested network; documented **2026-09-19** |
| SaveEcoBot authenticated retrieval | Requires an access key; authenticated device access still needs verification                                                                |
| Device web page                    | The computer could not reach it during the last recorded check; live web behavior remains unverified                                        |
| OTA transfer                       | Implemented upload window, but an actual transfer has not yet been verified on this device                                                  |
| Battery readings                   | Voltage calibration needs comparison with a multimeter; percentage remains an approximation                                                 |
| Radiation readings                 | Remote SaveEcoBot records, not measurements made by a local sensor                                                                          |

The display's row-repetition workaround improves readability on the tested panel but does not establish that its underlying fault is repaired. See [OLED configuration](#oled-configuration).

See [detailed section documentation](docs/SECTIONS.md) and the latest [verification record](VERIFICATION.md) before extending the firmware.

<a id="attribution-and-fan-project-notice"></a>

## 📚 Attribution and fan-project notice

### Data and technical references

| Reference                                                                                                                  | Role in this project                                       |
| -------------------------------------------------------------------------------------------------------------------------- | ---------------------------------------------------------- |
| [SaveEcoBot](https://www.saveecobot.com/) · [API access](https://www.saveecobot.com/en/static/api)                         | Remote radiation station measurements and data attribution |
| [OpenWeatherMap](https://openweathermap.org/api)                                                                           | Weather API                                                |
| [Ukraine Alarm / Stfalcon](https://api.ukrainealarm.com/)                                                                  | Regional alert API                                         |
| [Open-Meteo geocoding](https://open-meteo.com/en/docs/geocoding-api)                                                       | City lookup based on GeoNames                              |
| [Espressif ADC guidance](https://docs.espressif.com/projects/esp-idf/en/v4.4.7/esp32s3/api-reference/peripherals/adc.html) | ADC noise-reduction reference used in the battery notes    |

### Fan-project notice

GILKA is an **independent fan-made project**, not an official GSC Game World product. No affiliation or endorsement is claimed. References to S.T.A.L.K.E.R. and the original detector identify the inspiration; this project does not claim ownership of the original game's names or designs.

---

<p align="center">
  <strong>GILKA V1.05</strong> · A piece of the Zone, built with real electronics.<br>
  <a href="#top">Back to top ↑</a>
</p>
