# Firmware verification

## Latest build and upload, 2026-09-19

- Current firmware compiled and uploaded to ESP32-S3 via /dev/cu.usbmodem2101. Flash hash verified; board reset successfully. Includes Ukraine Alarm, 60-second polling and SOS signalling.
- Program size: 1,081,653 bytes; static RAM: 52,588 bytes. This confirms build/flash completion, not live API or physical alarm validation.

## Minute polling and SOS, 2026-09-19

- Ukraine Alarm change-index polling is 60 seconds; freshness timeout is 180 seconds. Immediate HTTP error/offline handling remains.
- Active threats drive synchronized repeating SOS on the buzzer, GPIO8 LED and onboard LED, with 200 ms dots and 600 ms dashes. Pattern starts on alert onset. Mute stops sound while LED signalling continues; timer behavior is preserved.
- Native tests verify two complete cycles at millisecond boundaries, rollover and freshness limits. PlatformIO build passed. This revision is not uploaded; live alarm timing still needs device confirmation.

## Ukraine Alarm v3, 2026-09-19

- Implemented the supplied Ukraine Alert API 3.0 swagger: raw Authorization token, region name lookup, lastActionIndex polling and region activeAlerts parsing. Separate UKRAINEALARM_API_KEY / uaAlarmKey credential prevents reuse of the prior provider token. SaveEcoBot behavior is unchanged.
- Tests passed for region matching, clear/AIR/NUCLEAR/INFO, null/unknown/missing responses, large action indices, JSON projection and environment validation. Seven environment tests passed. PlatformIO build passed (1,081,457 bytes flash, 52,580 bytes static RAM).
- Token was empty during validation. Authenticated API calls and device alarm response remain unverified; this revision has not been uploaded.

## Device public JSON HTTP 403 diagnosis, 2026-09-19

- On-device diagnostics isolated the failure to the public SaveEcoBot request; city geocoding succeeded. The 403 body is a Cloudflare browser challenge (Just a moment, challenges.cloudflare.com), not a TLS certificate failure or malformed gamma JSON. Equivalent desktop requests returned 200.
- Firmware now identifies the request source in HTTP errors, labels this case Public blocked 403, and backs off to ten-minute retries. TLS validation remains enabled; no attempt is made to bypass the browser challenge. Temporary HTML logging removed. Public live data is not working on this device/network; official authenticated API access still requires a key and testing.

## Public JSON fallback, 2026-09-19

- Empty effective SaveEcoBot key now selects the configured public station JSON. Default station 22800 on vul. Hoholia in Cherkasy returned HTTP 200 without credentials, gamma=150 nSv/h, UTC 2026-09-19 06:12:00. Time was cross-checked with the public map marker gamma_t=1789798320; gamma units are defined as nSv/h in SaveEcoBot map metadata.
- Public station coordinates are checked against the selected city and radius. OLED source is PUBLIC; stale/old flags remain. API credentials select the existing authenticated path.
- Six environment tests and native parser tests passed, including actual public-response fixture, wrong pollutant, missing values/time, future time and out-of-radius cases. PlatformIO build passed. USB upload to ESP32-S3 on /dev/cu.usbmodem2101 succeeded with flash hash verification and hardware reset. Live source retrieval is recorded separately when confirmed.

## City/radius configuration, 2026-09-19

- Replaced numeric station override with SAVEECOBOT_STATION city name and SAVEECOBOT_RADIUS_KM. Geocoding is limited to Ukraine; ambiguous names require an oblast qualifier. Station selection uses the resolved coordinates and configured radius.
- Host tests cover changed city coordinates, radius filtering, ambiguous/foreign geocoding responses, UTF-8 city names and invalid radius values.

## Current: SaveEcoBot dosimeter, 2026-09-19

- Replaced runtime Safecast integration with documented SaveEcoBot radiation endpoints, separate ecoKey NVS credential and SAVEECOBOT_API_KEY environment default.
- Station lookup streams bounded objects; gamma nSv/h is normalized to uSv/h with UTC time and provider-old status. Prior cached data stays explicitly STALE after errors.
- Final PlatformIO build and USB upload to ESP32-S3 on /dev/cu.usbmodem2101 succeeded with flash hash verification (1,074,289 bytes program, 52,068 bytes static RAM). The SaveEcoBot key was still empty at build time.
- Host radiation parser tests and four environment tests passed. Authenticated API retrieval requires the user’s new key; unauthenticated endpoint returns HTTP 401.

## Previous: Safecast dosimeter, 2026-09-19

- Added case-sensitive `safecast_API_KEY` to .env/.env.example and the build loader. Added optional web/NVS `safeKey` configuration with existing precedence and clear semantics. Public reads also work anonymously.
- DOSIMETER now queries verified HTTPS measurements within 25 km of Cherkasy. Displays original CPM or uSv/h, capture date, distance/ID details, and explicit ARCHIVE (>24 hours) / STALE cache status. CPM is not converted to dose. Synthetic Geiger sound/LED simulation removed from this section.
- Host parser tests passed using a reduced real public response and malformed/empty/future/out-of-radius/unsupported-unit cases; four .env tests passed.
- First device test exposed a missing Amazon CA root. Added official Amazon Root CA 1 after fingerprint comparison with the system trust store; TLS validation remains enabled.
- Final PlatformIO build and USB upload succeeded with hash verification. On-device boot log confirms: `[Safecast] id=231812764 value=19.000 CPM date=2023-12-05 distance=1.4km`. This confirms real HTTPS retrieval/parsing, not current local radiation measurement.
- OLED appearance and key-authenticated requests are not physically/independently verified; anonymous reads and the complete embedded HTTPS path are verified.


## Previous: .env firmware defaults, 2026-09-19

- PlatformIO pre-build hook generates a private header under ignored build output from .env. Wi-Fi SSID/password, weather/alert keys, and timezone are defaults only; existing NVS values, including empty API overrides, take priority.
- Literal parser performs no shell execution or variable substitution. Secrets are absent from compiler flags and validation messages, but are embedded in binaries. Host/OTA helper variables are not firmware defaults.
- Three Python tests passed for quoting, UTF-8, literal special characters, validation, missing files, and header refresh. PlatformIO build and USB upload passed with flash verification. README and environment-file comments updated.


## Previous: battery indicator, 2026-09-19

- User confirmed 1S Li-ion (4.2 V maximum), 6000 mAh, with 100 kΩ from battery + to GPIO9 and 100 kΩ from GPIO9 to GND. ADC millivolts are doubled, averaged, and filtered.
- Three battery segments replace the lower-left GILKA status text. Approximate thresholds are 3.40/3.70/3.95 V with 30 mV hysteresis. Invalid readings display a question mark. Startup branding is unchanged.
- BATTERY shows voltage, rated capacity, and a voltage-based approximate percentage. No charging-current or remaining-capacity measurement is claimed.
- Native battery tests passed for invalid ranges, percentage clamping, thresholds, and hysteresis. PlatformIO build and USB upload succeeded with flash hash verification. Actual voltage accuracy needs comparison with a multimeter; the OLED icon needs physical confirmation.


## Previous: GPIO8 indicator, 2026-09-19

- Added an active-high external LED on GPIO8, initialized off before startup delays. Wiring: GPIO8 through 330 Ω to anode, cathode to GND.
- Running timers/stopwatch flash each elapsed second; pause stops the heartbeat. Timer completion and latched air alerts take priority with 350 ms on/off. Simulated detector counts flash independently of the sound setting. Existing onboard RGB alarm behavior is retained.
- Native tests passed for heartbeat boundaries, pause, alarm priority, and simulated indication. PlatformIO build and USB upload passed with flash verification. Physical LED operation awaits wiring/user confirmation.


## Previous: settings cleanup, 2026-09-19

- Settings now contain SOUND, DISPLAY, WI-FI CONFIG, OTA UPDATE, BATTERY, and WI-FI INFO. Removed the unused sensor configuration and its runtime code, web field, validation dependency, and documentation references.
- PlatformIO build passed (application flash 1,029,633 bytes); `git diff --check` passed.
- After reconnecting the device, USB upload to `/dev/cu.usbmodem2101` succeeded with flash hash verification and automatic reset (2026-09-19).


## Previous: inverted startup screen, 2026-09-19

- Startup now renders custom Cyrillic `Гілка.ос` glyphs across 117×42 pixels in the blue region, with `V1.05` centered in the yellow region. The framebuffer is lit with black lettering; no normal status separator is drawn on the splash.
- The completed frame stays visible for a 2000 ms delay before encoder/Wi-Fi startup continues. Later screens reset their normal drawing colors.
- PlatformIO build and USB upload passed; flash hash verified. `git diff --check` passed. Physical appearance awaits user confirmation.


## Previous: timer changes, 2026-09-19

- Section selector now starts with TIME & DATE, followed by TACTICAL: TIMER.
- Countdown adjusts in 30-second increments from 00:30 through 99:30; zero selects stopwatch. Boundary values clamp instead of wrapping.
- Timer completion emits a one-shot 200 ms (LOW) or 300 ms (MAX) beep once per second until acknowledged. Dedicated buzzer pulse timing prevents blocking display work from making the tone continuous. MUTE/CLICKS ONLY still suppress timer alarms; active air-alert sound has priority.
- Acknowledgment immediately cancels any timer beep.
- Native section tests passed, including step boundaries, alarm cadence/acknowledgment, and rollover during alarm repetition. PlatformIO build passed; USB upload to /dev/cu.usbmodem2101 succeeded with hash verification.
- Physical sound and encoder behavior await user confirmation. README and section documentation updated.


## Previous: six-section firmware, 2026-09-17

This status supersedes the older review and pending buzzer notes below. The user confirmed the 3.3V active-low buzzer works.

- Implemented hold + rotate + release selection across six sections, timer/stopwatch, NTP clock, Cherkasy weather and regional alert clients, explicitly labeled radiation simulation, persistent settings, and physically armed OTA. See `docs/SECTIONS.md`.
- Retained GPIOs, 3.3V buzzer wiring, 4MB flash overrides, and the OLED doubled-height text workaround.
- Both native test executables passed with `-Wall -Wextra -Werror`: OLED settings and section timer/freshness/wrapping logic. `git diff --check` passed.
- PlatformIO build passed: RAM 51,500 bytes / 327,680; application flash 1,030,365 / 1,310,720 bytes.
- USB upload to `/dev/cu.usbmodem2101` succeeded with flash hashes verified.
- A subsequent 27-second boot observation confirmed OLED at 0x3C, encoder GPIO5/4/6, HTTP server startup, Wi-Fi connection at 192.168.50.136, and mDNS startup. No crash was observed. Initial NVS missing-key messages for timezone and API keys are expected; defaults are used.
- The computer could not resolve gilka.local or route to the device IP during web checks. Live HTTP rendering and invalid POST validation therefore remain unverified on hardware.
- No API keys are stored, so live API results/TLS handshakes were not tested. Physical menu gestures, LED color, timing/sound perception, and OTA transfer require device confirmation. Battery inputs remain deliberately unconfigured pending wiring details.


## 2026-09-17 source review and fixes

- Reject incomplete, malformed, negative, overflowing, or out-of-range OLED web settings with HTTP 400 before modifying RAM or NVS.
- Keep DIM brightness at or below normal brightness, limit stored timers to 86400 seconds, and keep enabled OFF time at or after DIM time when loading NVS.
- Allow encoder DIM timeout editing up to 86400 seconds, matching the web interface.
- Close pending encoder edits after a successful web settings update to prevent stale values from overwriting it.

The initial review retained GPIO1/2 for OLED, GPIO5/4/6 for the encoder, COM configuration 0x02, rotation 2, blue/yellow geometry, NVS namespace `wifi`, 4 MB flash overrides, and Wi-Fi stack initialization before `server.begin()`.

## Follow-up: broken OLED rows in the supplied photo

The photo shows fragmented text and status characters spanning the blue/yellow boundary. Incorrect COM row mapping is the working diagnosis, not yet confirmed on hardware.

- Changed the forced COM configuration from sequential `0x02` to alternative `0x12`, matching the installed Adafruit driver's 128x64 initialization. This supersedes the handoff's claim that `0x02` was required; the new setting still needs physical confirmation.
- Set both SSD1306 constructor clock arguments to 100 kHz. `Wire.setClock(100000)` alone does not prevent this driver from using its default 400 kHz during display transactions.
- Kept rotation, pixel coordinates, pins, and framebuffer dimensions unchanged.

After building and uploading, check that all four ONLINE rows are readable, the separator is a single line, and `GILKA / ONLINE` sits entirely in yellow. If this does not resolve the issue, compare a full-screen row/grid test under both COM configurations before changing geometry or choosing a different controller driver.

These follow-up changes have been checked against the installed driver API but have not been compiled, uploaded, or verified on the panel.

## Follow-up: rows still missing with COM 0x12

The second photo shows correct placement but missing text strokes, also confirmed by direct observation. The COM change did not fully resolve the fault.

Added a separate `oled-diagnostic` environment. The default build still selects the normal application. The diagnostic runs seven stages, eight seconds each, at low contrast:

1. Controller all-pixels-on (`A5`), bypassing display RAM and text drawing.
2. All pixels on via framebuffer.
3. Even logical rows.
4. Odd logical rows.
5. Text.
6. The same text shifted down one pixel.
7. Black.

Build/upload and serial monitor:

```sh
pio run -e oled-diagnostic -t upload
pio device monitor -b 115200
```

Serial commands: `p` pauses/resumes, `n` advances, `a` selects COM 0x12, `s` selects COM 0x02. COM changes apply to the current stage and are not saved. The firmware does not access NVS. Wi-Fi is unavailable while this diagnostic is installed.

- If the controller all-on stage still has missing physical rows under both mappings, investigate panel/driver hardware, supply, or controller identification; changing font coordinates will not resolve that result. Do not conclude hardware damage from the photo alone.
- If controller all-on is complete but framebuffer fill is incomplete, investigate data transfer/addressing.
- Compare even and odd stages: both should illuminate 32 rows, offset by one pixel under the normal 0x12 mapping.

Restore the normal application afterward:

```sh
pio run -e esp32-s3-zero -t upload
```

Diagnostic build/upload was subsequently authorized and completed successfully on ESP32-S3 at `/dev/cu.usbmodem2101`; flash hashes verified and the board reset. The first upload attempt was blocked by an existing PlatformIO serial monitor, which was stopped before retrying. The uploader confirmed 4 MB embedded flash and 2 MB PSRAM. The physical fault remains unresolved pending visual test results.

### Latest diagnostic revision

The user reports that all-pixels-on looks correct directly. Photos of identical text at Y and Y+1 show different missing strokes, suggesting a row-parity issue; this does not yet establish hardware damage.

The diagnostic now has an eighth stage and starts paused there. Across the blue area it draws three 40-pixel-wide blocks: left = even logical rows, center = odd logical rows, right = all rows. The yellow area contains `GILKA 012345` at text scale (1, 2), duplicating each font row across both row parities as a possible readability workaround. Normal firmware has not adopted that workaround. This diagnostic revision compiled and uploaded successfully; visual results are pending. Resuming cycles through all eight stages (64 seconds).

The supplied photo subsequently showed the even-row block blank, with odd-row and all-row blocks illuminated alike. Double-height text retained its strokes. This confirms missing even logical-row output in the tested 128x64/COM 0x12 configuration, but does not identify the underlying cause.

Added `oled-diagnostic-32` to test a native 128x32 framebuffer (512 bytes), multiplex 31 and COM 0x02 using the library's 32-row initialization. It displays four static lines at Y=0,8,16,24, with normal-size text and rotation 2. This is a geometry diagnostic, not a confirmed fix or a change to the main application. It does not access NVS.

### Readability workaround in the main application

The 128x32 diagnostic compiled and uploaded, but the user reported a black screen. Restored the main application's 128x64/COM 0x12 setup and applied text scale (1, 2), which retained complete glyph strokes in the comparison photo. This is a workaround for the observed missing row parity, not a confirmed repair of the panel/controller.

All normal screens now use three blue text rows at Y=0,16,32 and yellow status text at Y=48. Default-font ink ends at Y=45 in the blue region and Y=61 in the yellow region. Menu scrolling shows three items at a time while retaining all six items. RSSI is available in WiFi info and on the web page; the HOME screen shows SSID, IP, and hostname. Setup still shows SSID, password, and IP. Firmware compilation succeeded. Hardware readability and menu operation need user confirmation after upload.

### Encoder buzzer

Added output on GPIO7 for a three-pin buzzer module with a 3.3V-compatible input. The user observed continuous sound with active-high logic and silence during actions, confirming active-low behavior. Restored LOW for beep and HIGH for silence, including initialization, timer completion, and error handling. Rotation currently triggers a 20 ms pulse per completed detent; a debounced button press triggers one 80 ms pulse, including a wake-only press. Release and long-press recognition do not add a second beep.

An ESP one-shot timer stops each pulse independently of blocking OLED updates or HTTP processing. Overlapping pulses are ignored rather than extended into a continuous tone; timer errors leave the output silent. Durations are in `include/EncoderBuzzer.h`. This controls duration, not true loudness. Firmware build and upload succeeded; acoustic output and module wiring still need physical confirmation.

The user subsequently reported sustained sound even after startup. Added `silenceOutput()` as the first operation in `setup()`, before Serial initialization; this revision built and uploaded successfully. This only reduces the application startup window and does not explain sustained sound with an idle HIGH output. Buzzer pin labels, actual wiring, supply voltage, and module type need confirmation before further polarity or electrical changes. A pull-up suggestion for a brief startup chirp is not a confirmed remedy for this sustained-sound report.

## Automated checks

Passed native settings tests with:

```sh
clang++ -std=c++11 -Wall -Wextra -Werror -I include test/oled_settings_test.cpp -o /tmp/gilka-oled-settings-test
/tmp/gilka-oled-settings-test
```

Full firmware compilation remains unverified: `pio run` was blocked from writing the PlatformIO cache lock outside the workspace, and the access request was declined. No firmware was uploaded during this review.

## On-board checks still required

1. Build and upload with PlatformIO. Confirm the ONLINE screen and `http://gilka.local` after boot.
2. Scroll through all six menu items in both directions; verify four rows stay above the separator. Check short press and long press (1.2 seconds).
3. Preview and save brightness, then reboot to check persistence. Long press during an unsaved preview should restore saved brightness.
4. While online with the setup AP closed, set DIM to 5 seconds and OFF to 10 seconds. Verify dimming and power-off. First rotation or button press after OFF should only wake the display; a held wake press should not trigger HOME.
5. Test zero timeouts separately. The setup AP intentionally keeps the OLED awake.
6. Set DIM above 3600 seconds through the web page, then adjust it one encoder detent: it should change by 5 seconds, not jump back to 3600.
7. Submit incomplete or non-numeric `/display` POST data: expect HTTP 400 and unchanged settings. Submit valid settings while editing on the encoder: expect HOME with the new settings applied.
8. Disable the router temporarily. Confirm reconnect attempts and setup AP after approximately 30 seconds. Restore the router; confirm mDNS recovery and AP closure after approximately 10 seconds online.
9. Check captive portal access at `192.168.4.1`, Wi-Fi credential persistence, and OLED settings through the setup AP. Avoid deleting working credentials unless intentionally testing provisioning from scratch.

Buzzer integration is not part of this pass.

### Current buzzer test: 3.3V supply

The module photo identifies an MH-FMD active-low board. The user confirmed that, powered at 5V and disconnected from GPIO7, grounding I/O produces sound and tying I/O to 5V silences it. Direct 3.3V supply previously produced no sound, with test conditions not fully established. The user now requested another 3.3V trial. Kept LOW=on/HIGH=off and early startup silence; increased rotation/press pulses to 20/80 ms to test whether very short pulses were inaudible. No automatic startup beep is added. This does not guarantee that this module operates at 3.3V. Physical sound verification remains pending.
