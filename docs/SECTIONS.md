# GILKA sections

Location: Cherkasy city, Ukraine. Air alerts cover Cherkasy Oblast (Ukraine Alarm v3; region ID resolved by name).

## Encoder

- Hold the button and rotate to highlight one of six sections; release to open it. This gesture also works from settings.
- Rotate without holding to change the current section's value or move through a settings list.
- Short click performs the action below.
- Hold without rotating for 1.2 seconds to save/back in settings, or open the section selector from a section.
- The first interaction after the display turns off only wakes it. Release and use the next interaction normally.

| Section | Rotate | Click |
| --- | --- | --- |
| TIME & DATE | — | Cycle 24-hour, 12-hour, seconds formats |
| TACTICAL: TIMER | When paused: stopwatch (zero), then 00:30–99:30 in 30-second steps; changing duration resets elapsed time | Start/pause; acknowledge finished alarm |
| METEO: SECTOR | — | Temperature/humidity or wind/pressure |
| DOSIMETER | Toggle measurement / distance and ID | Toggle approximate gamma uR/h |
| SURGE: ALERT | — | Mute the current siren |
| SYS: SETTINGS | Select setting | Open/edit setting |

Timers continue when another section is open. Each elapsed minute triggers a double beep; a completed countdown beeps once per second (200 ms in LOW, 300 ms in MAX) and flashes the red onboard RGB LED. Click acknowledges the timer alarm from any screen. A countdown's final minute uses the final alarm instead of the minute double beep.

The OLED's existing doubled-height text workaround remains enabled, with three blue text rows and a yellow status row.

## Settings

SOUND offers MUTE, LOW, MAX, CLICKS ONLY. LOW is the initial default; idle is silent. LOW/MAX change pulse lengths, not actual acoustic volume. MUTE disables all sounds; CLICKS ONLY retains encoder feedback and suppresses timer/alert sounds. Visual alarms remain enabled. The active-low MH-FMD buzzer stays on GPIO7, powered by **3.3V**, as confirmed working on this device.

DISPLAY opens the existing brightness, dimming, and screen-off settings. WI-FI CONFIG opens GILKA-SETUP / gilka1234, with the portal at http://192.168.4.1. WI-FI INFO shows the active setup details or station address. Settings are stored in ESP32 NVS.

The battery is configured as 1S Li-ion, 6000 mAh: GPIO9 reads the midpoint of two 100 kΩ resistors (battery + to GPIO9, GPIO9 to common GND). Firmware doubles ADC voltage. The lower-left status bar contains a three-segment battery icon instead of GILKA. Segments indicate approximate voltage levels at 3.40/3.70/3.95 V, with 30 mV hysteresis; invalid readings show `?`. BATTERY displays voltage and an approximate linear 3.2–4.2 V percentage. Capacity is a rated value, not a measured remaining charge. The startup screen remains unchanged.

## Internet data

Open http://gilka.local (or the IP shown in WI-FI INFO) and use the Sections form:

- Enter an [OpenWeatherMap key](https://openweathermap.org/api) for current weather.
- Enter an [Ukraine Alarm token](https://api.ukrainealarm.com/) for Cherkasy Oblast alerts.
- Kyiv time defaults to `EET-2EEST,M3.5.0/3,M10.5.0/4`, with NTP synchronization and automatic seasonal offset. The POSIX timezone can be edited.

Blank key fields preserve stored keys; the remove checkboxes clear them. Saved keys are not echoed into the page. The local configuration portal is HTTP, so configure it on a trusted network. Outbound API connections verify TLS certificates (see TLS_ROOTS.md).

Weather refreshes every 10 minutes, with 30-second retries on errors. Data older than 20 minutes since retrieval or 2 hours since observation is unavailable. Alerts poll every 60 seconds; after 180 seconds without a successful response, or a Wi-Fi disconnection, the state is unknown/stale. A previously received active alert remains latched until a fresh no-alert response; clicking mutes its sound. Missing data never displays a clear status. Keep using official alert channels; this device is an additional display.


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

## Startup screen

At startup, the OLED displays **Гілка.ос** across the blue region and **V1.05** in the yellow region for two seconds. The entire background is lit with dark lettering. Normal section screens follow with their usual colors. The title uses custom Cyrillic pixel glyphs and retains the display row workaround.

## External LED

GPIO8 drives an active-high LED: connect GPIO8 through 330 Ω to the anode, with the cathode connected to GND. It flashes every elapsed second while a countdown/stopwatch runs, stops that heartbeat on pause, and flashes every 350 ms on/off for timer completion or a latched air alert. DOSIMETER uses remote data and does not synthesize detector flashes. Sound muting leaves visual indication active. Other idle sections leave it off.

## SaveEcoBot dosimeter

Source: [SaveEcoBot](https://www.saveecobot.com/), using its [documented radiation API](https://www.saveecobot.com/en/docs/api/radiation).

Set `SAVEECOBOT_API_KEY` in `.env` and rebuild/upload, or save it on the web page. NVS `ecoKey` overrides compiled defaults; clearing it selects public JSON fallback. Existing Safecast credentials are never reused. Auth uses the URL-encoded `apikey` query parameter over verified HTTPS; keys are never logged.

The worker streams `sensors-details` one object at a time and selects the closest station within the configured radius of the selected city (49.4444, 32.0598). The selection is cached until restart/configuration change. It then polls `sensor-last-data/{sensor_id}` every 10 minutes, retrying errors after 60 seconds. Only `gamma_nsv_h` is accepted and converted to uSv/h by dividing by 1000. Localized unit labels and other pollutants do not determine the measurement type. History is discarded while parsing to bound heap use.

The screen shows the UTC measurement date/time. `is_old` or age >24 hours produces ARCHIVE; network errors/offline/cache age >20 minutes produce STALE. Invalid or unavailable responses retain any previous reading only as STALE; without a previous reading there is no value. Missing UTC timestamps, future timestamps, null/negative readings and non-gamma pollutants are rejected. There is no simulation or generated Geiger clicking. The chosen nearest station can be offline; the firmware does not claim its stale data is current.

Rotation opens distance and station ID. Click toggles an approximate gamma uR/h representation. Data describes a remote station, not a local radiation measurement.

Run tests:

```sh
clang++ -std=c++11 -Wall -Wextra -Werror -I include \
  -I .pio/libdeps/esp32-s3-zero/ArduinoJson/src \
  test/saveecobot_test.cpp -o /tmp/gilka-saveecobot-test
/tmp/gilka-saveecobot-test
python3 test/env_config_test.py
```

The unauthenticated endpoint returned HTTP 401. Authenticated device retrieval awaits the user's SaveEcoBot access key.

### SaveEcoBot city and search radius

```dotenv
SAVEECOBOT_STATION="Cherkasy, Cherkasy Oblast"
SAVEECOBOT_RADIUS_KM=25
```

`SAVEECOBOT_STATION` is a Ukrainian city name, not a sensor ID. Ukrainian or English names are accepted by [Open-Meteo geocoding](https://open-meteo.com/en/docs/geocoding-api), based on GeoNames. Add an oblast after a comma for namesakes; ambiguous or missing results stop station lookup with a configuration message. Only Ukrainian populated places are accepted. No API credentials are sent to the geocoder.

`SAVEECOBOT_RADIUS_KM` is the maximum distance from the resolved city centre, in kilometres (greater than 0, at most 500; decimals supported). Firmware selects the nearest SaveEcoBot station inside that circle; it does not average all stations. No station within the radius means no data. Coordinates and selection are cached until reboot/configuration change. Rebuild/upload after editing these values. `SAVEECOBOT_STATION_ID` has been replaced and is no longer used.

### Public JSON fallback without a key

When the effective API key (NVS first, then `.env`) is empty, the worker reads `SAVEECOBOT_PUBLIC_JSON_URL` instead of calling the authenticated API:

```dotenv
SAVEECOBOT_API_KEY=""
SAVEECOBOT_PUBLIC_JSON_URL="https://www.saveecobot.com/en/station/22800.json"
```

This default is the public radiation station on vul. Hoholia, Cherkasy. It is a specific fallback station, not automatic public station discovery. Its coordinates must be inside `SAVEECOBOT_RADIUS_KM` of the configured city; if the city changes, select another public station URL. No national map download occurs on the device. Requests occur every ten minutes, with errors retried after one minute. A nonempty but rejected API key does not silently fall back.

Only public `last_data[].phenomenon == gamma` is accepted, in nSv/h, divided by 1000 for uSv/h. The actual station response was checked against the map's `gamma_t` UNIX timestamp: `2026-09-19 06:12:00` corresponds to `1789798320` UTC. Preserve source time and `is_old`; never substitute fetch time. OLED footer shows PUBLIC for this source and retains ARCHIVE/STALE labels. Missing/malformed/out-of-radius gamma data produces an error or explicitly stale cached reading, never generated values.

The actual public response is in `test/fixtures/saveecobot_public_22800.json`. Data attribution: [SaveEcoBot](https://www.saveecobot.com/). Public JSON usage is described on its [API page](https://www.saveecobot.com/en/static/api).

**Device limitation verified 2026-09-19:** although desktop retrieval of the public JSON succeeds, SaveEcoBot returns an HTTP 403 Cloudflare browser challenge to this ESP32. Firmware shows `Public blocked 403` and retries after ten minutes. The public fallback is therefore not currently usable on this device/network. Obtain an official API key for the authenticated path; authenticated device access still needs verification.

## Ukraine Alarm v3 integration

The supplied `swagger.json` describes [Ukraine Alarm / Stfalcon](https://api.ukrainealarm.com/), not the previous provider. Configure:

```dotenv
UKRAINEALARM_API_KEY=""
UKRAINEALARM_REGION_NAME="Черкаська область"
```

The token is sent verbatim in `Authorization`, with no Bearer prefix. Old Alerts.in.ua credentials are not reused. The web page stores this provider's key in separate NVS `uaAlarmKey`, which takes priority over the compiled `.env` default. Blank form input preserves it; Remove clears it. Rebuild/upload after changing `.env`.

The worker resolves the matching State ID from `/api/v3/regions`, retaining only state names/types/IDs while parsing. It checks `/api/v3/alerts/status` every 60 seconds, loads `/api/v3/alerts/{regionId}` initially or when `lastActionIndex` changes, and also refreshes full state every five minutes. A successful unchanged-index check refreshes connection freshness. Data is stale after 180 seconds without verification, immediately after a request error, or while offline.

AIR, ARTILLERY, URBAN_FIGHTS, CHEMICAL, NUCLEAR and CUSTOM activate the existing buzzer/LED alarm with an appropriate OLED label. INFO displays an informational status without the siren. An explicit empty activeAlerts array for the selected region clears the alarm; null/missing/unknown data does not. A latched threat survives communication errors and invalid responses. Encoder click mutes sound; LED continues until a confirmed clear state. Existing sound mode settings apply. This device supplements official alerts.

Run the parser tests:

```sh
clang++ -std=c++11 -Wall -Wextra -Werror -Iinclude \
  -I.pio/libdeps/esp32-s3-zero/ArduinoJson/src \
  test/ukraine_alarm_test.cpp -o /tmp/gilka-alert-test
/tmp/gilka-alert-test
python3 test/env_config_test.py
```

Ukraine Alarm sound and both LEDs repeat synchronized SOS (three short, three long, three short). Dot: 200 ms; dash: 600 ms; symbol pause: 200 ms; letter pause: 600 ms; repeat pause: 1400 ms. Each new alert starts at the first dot. Mute silences the buzzer while LED SOS continues. Timer signalling is unchanged.
