# GILKA sections

Location: Cherkasy city, Ukraine. Air alerts cover Cherkasy Oblast (Alerts.in.ua region 24).

## Encoder

- Hold the button and rotate to highlight one of six sections; release to open it. This gesture also works from settings.
- Rotate without holding to change the current section's value or move through a settings list.
- Short click performs the action below.
- Hold without rotating for 1.2 seconds to save/back in settings, or open the section selector from a section.
- The first interaction after the display turns off only wakes it. Release and use the next interaction normally.

| Section | Rotate | Click |
| --- | --- | --- |
| TACTICAL: TIMER | When paused: stopwatch, 1, 5, 15, 25 minutes; selecting resets elapsed time | Start/pause; acknowledge finished alarm |
| TIME & DATE | — | Cycle 24-hour, 12-hour, seconds formats |
| METEO: SECTOR | — | Temperature/humidity or wind/pressure |
| DOSIMETER | Select simulated dose (0.05–3.0 uSv/h) | Toggle uSv/h / approximate gamma uR/h |
| SURGE: ALERT | — | Mute the current siren |
| SYS: SETTINGS | Select setting | Open/edit setting |

Timers continue when another section is open. Each elapsed minute triggers a double beep; a completed countdown uses continuous sound and flashing red onboard RGB LED. Click acknowledges the timer alarm from any screen. A countdown's final minute uses the final alarm instead of the minute double beep.

The OLED's existing doubled-height text workaround remains enabled, with three blue text rows and a yellow status row.

## Settings

SOUND offers MUTE, LOW, MAX, CLICKS ONLY. LOW is the initial default; idle is silent. LOW/MAX change pulse lengths, not actual acoustic volume. MUTE disables all sounds; CLICKS ONLY retains encoder feedback and suppresses timer/alert/demo sounds. Visual alarms remain enabled. The active-low MH-FMD buzzer stays on GPIO7, powered by **3.3V**, as confirmed working on this device.

RADAR SENS adjusts software hold time (5–120 seconds), not the module's electrical sensitivity. DISPLAY opens the existing brightness, dimming, and screen-off settings. WI-FI CONFIG opens GILKA-SETUP / gilka1234, with the portal at http://192.168.4.1. WI-FI INFO shows the active setup details or station address. Settings are stored in ESP32 NVS.

No radar GPIO or battery divider values were supplied. Those functions show unconfigured rather than fabricated readings. To enable them after wiring is confirmed, set `GILKA_RADAR_PIN`, `GILKA_BATTERY_PIN`, and `GILKA_BATTERY_RATIO` in build flags. The ratio is `(Rtop + Rbottom) / Rbottom`; the ADC input must remain within the board's valid voltage range. Battery percentage is an approximate linear 3.2–4.2V estimate, not a fuel-gauge measurement.

## Internet data

Open http://gilka.local (or the IP shown in WI-FI INFO) and use the Sections form:

- Enter an [OpenWeatherMap key](https://openweathermap.org/api) for current weather.
- Enter an [Alerts.in.ua token](https://devs.alerts.in.ua/) for Cherkasy Oblast alerts.
- Kyiv time defaults to `EET-2EEST,M3.5.0/3,M10.5.0/4`, with NTP synchronization and automatic seasonal offset. The POSIX timezone can be edited.

Blank key fields preserve stored keys; the remove checkboxes clear them. Saved keys are not echoed into the page. The local configuration portal is HTTP, so configure it on a trusted network. Outbound API connections verify TLS certificates (see TLS_ROOTS.md).

Weather refreshes every 10 minutes, with 30-second retries on errors. Data older than 20 minutes since retrieval or 2 hours since observation is unavailable. Alerts poll every 30 seconds; after 90 seconds without a successful response, or a Wi-Fi disconnection, the state is unknown/stale. A previously received active alert remains latched until a fresh no-alert response; clicking mutes its sound. Missing data never displays a clear status. Keep using official alert channels; this device is an additional display.

DOSIMETER is explicitly a **simulation**, with randomly spaced clicks increasing with the selected demo dose. It does not measure radiation or fetch SaveEcoBot data. Its approximate gamma conversion and simulated clicks are not calibrated measurements.

## OTA

Select SYS: SETTINGS → OTA UPDATE while Wi-Fi is connected. This arms a 120-second upload window and shows a temporary password and IP on the OLED. Leaving the screen or timing out closes the listener; clicking can disarm/re-arm it.

Build the normal firmware, then upload from the same LAN using the displayed IP and password:

```sh
~/.platformio/penv/bin/pio run -e esp32-s3-zero
~/.platformio/penv/bin/python ~/.platformio/packages/framework-arduinoespressif32/tools/espota.py \
  -i DEVICE_IP -p 3232 -a DISPLAYED_PASSWORD \
  -f .pio/build/esp32-s3-zero/firmware.bin
```

USB upload remains available with the normal PlatformIO environment. OTA authentication and transfer use ArduinoOTA's protocol; firmware images are not cryptographically signed by this application.

## Validation

Host tests cover countdown pause/resume, completion/acknowledgment, stopwatch minute events, millisecond rollover, stale-data timing, section wrapping, and existing OLED settings validation. The ESP32 build checks all integrated screens, network services, and OTA code. Physical encoder gestures, sound levels, LED color, keyed API responses, optional sensors, and an actual OTA transfer need device-level verification.
