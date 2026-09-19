# Contributing to GILKA

GILKA is Arduino firmware for an ESP32-S3 device with an OLED, rotary encoder, buzzer, LEDs and battery voltage measurement. Start with [README.md](README.md) for operation and [HARDWARE.md](docs/HARDWARE.md) for the current wiring.

## Development setup

Use PlatformIO Core (or the PlatformIO extension), Python 3, and a C++ compiler such as Clang for host tests. Run the commands below from the repository root.

Create local configuration only if it does not already exist:

```sh
if [ ! -f .env ]; then
  cp .env.example .env
fi
```

Edit `.env` locally. An API key is not required to compile or run host tests. PlatformIO reads this file through `scripts/load_env.py`; do not source it as a shell script. The parser treats values literally and does not expand environment variables or execute commands.

Build the main application:

```sh
pio run -e esp32-s3-zero
```

If `pio` is not on PATH, use the executable from your PlatformIO installation, commonly `~/.platformio/penv/bin/pio` on macOS/Linux.

The board definition is `esp32-s3-devkitm-1`, with project overrides for the actual 4 MB flash device. Preserve these overrides when changing build configuration.

## Project layout

| Location | Responsibility |
| --- | --- |
| `src/main.cpp` | Startup, OLED rendering, encoder input, Wi-Fi, captive portal and web server |
| `include/Sections.h` | Section UI, settings, alarm outputs and service snapshots |
| `include/SectionLogic.h` | Hardware-independent timer, freshness and SOS logic |
| `include/SectionServices.h` | Background HTTP/TLS worker and configuration/result queues |
| `include/UkraineAlarmData.h` | Ukraine Alarm region, change-index and alert parsing |
| `include/SaveEcoBotData.h` | Radiation parsing, city coordinates and distance filtering |
| `include/EncoderBuzzer.h` | Active-low buzzer and timed pulses |
| `include/BatteryMonitor.h`, `include/BatteryGauge.h` | ADC sampling and battery indication |
| `include/OLEDSettings.h` | OLED settings validation |
| `include/ServiceTrust.h` | Trusted TLS roots; provenance in `docs/TLS_ROOTS.md` |
| `scripts/` | Literal `.env` parsing and generated firmware defaults |
| `test/` | Standalone host tests and response fixtures |
| `docs/`, `VERIFICATION.md` | Hardware, behavior, troubleshooting and verification records |

`Sections.h` integrates with globals in `main.cpp`; it is not a standalone translation unit. Keep portable calculations and response parsers separate so they can be tested without Arduino hardware.

## Configuration and secrets

Update `.env.example`, parser validation, relevant tests and documentation together when adding or renaming a firmware setting. Current service settings include:

- `UKRAINEALARM_API_KEY` and `UKRAINEALARM_REGION_NAME`.
- `SAVEECOBOT_API_KEY`, `SAVEECOBOT_STATION`, `SAVEECOBOT_RADIUS_KM` and `SAVEECOBOT_PUBLIC_JSON_URL`.
- `OPENWEATHERMAP_API_KEY`, alongside Wi-Fi credentials and `DEVICE_TIMEZONE`.

`SAVEECOBOT_STATION` is a city name, optionally qualified by oblast; the radius is in kilometres. It is not a station ID.

Generated defaults are written to `.pio/build/<environment>/generated/FirmwareDefaults.h`. Existing NVS values take precedence for settings that are persisted, including service keys. A successful rebuild does not replace an existing NVS override. The web form keeps saved keys when its fields are blank; its removal checkboxes explicitly clear them.

Keep `.env`, generated headers, build output and credentials out of commits and shared logs. Credentials are embedded in firmware binaries, even though they are not printed in compiler arguments. Do not publish a binary built with personal credentials. Use synthetic credentials in tests and remove secrets from fixtures and issue reports. Each API provider has its own credentials; do not reuse a key when changing providers.

## Testing

The tests are standalone C++ executables and Python tests; there is no configured PlatformIO native test environment. A main firmware build installs the ArduinoJson dependency needed by the parser tests.

Run Python configuration tests:

```sh
python3 test/env_config_test.py
```

Run all C++ host tests with Clang (or substitute `g++`):

```sh
(
  set -eu
  test_dir=$(mktemp -d)
  trap 'rm -rf "$test_dir"' EXIT
  for source in test/*_test.cpp; do
    executable="$test_dir/$(basename "$source" .cpp)"
    clang++ -std=c++11 -Wall -Wextra -Werror \
      -Iinclude -I.pio/libdeps/esp32-s3-zero/ArduinoJson/src \
      "$source" -o "$executable"
    "$executable"
  done
)
```

Run these from the project root because parser tests load fixtures using relative paths. Successful C++ tests normally produce no output.

For code changes, run the relevant host tests and the main firmware build. For documentation-only edits, check commands, paths and statements against the implementation; a device upload is unnecessary. Check whitespace before submitting:

```sh
git diff --check
```

Add tests for changed behavior and failure cases: malformed JSON, missing fields, incorrect units, wrong region, stale data, timing boundaries and `millis()` rollover where applicable. Tests should verify behavior, not simply reproduce the implementation.

## Firmware behavior to preserve

- Keep network operations in the background worker so DNS/TLS delays do not block encoder interaction. Bound JSON memory use and discard unneeded large fields while parsing.
- Keep TLS certificate validation enabled. Document root changes in `docs/TLS_ROOTS.md`; do not log request URLs containing keys or authorization headers.
- A failed request, unknown alert type, missing region or invalid payload must not be interpreted as an all-clear. Preserve a latched threat until a confirmed clear/informational state; button mute suppresses sound, not the visual alarm.
- Ukraine Alarm checks changes once per minute. Threat signalling is synchronized SOS on the enabled buzzer and both LEDs. Timer completion has its own periodic beeps and 350/350 ms LED pattern.
- Preserve source units and measurement time for remote radiation. Do not present generated values as measurements or treat fetch time as observation time. Public fallback must respect the configured city/radius and display its source.
- GPIO7 HIGH is silent for the tested active-low buzzer; initialize silence early. Preserve the working 3V3 wiring, OLED panel settings and battery divider scale unless the hardware change is intentional and documented.
- Use elapsed-time state machines instead of blocking delays for ongoing UI and alarm patterns. Keep hardware changes consistent with `docs/HARDWARE.md`.

## Upload and device checks

Identify the connected board's port:

```sh
pio device list
```

Compile and upload, replacing the example macOS port with your device's port:

```sh
pio run -e esp32-s3-zero -t upload --upload-port /dev/cu.usbmodem2101
pio device monitor --port /dev/cu.usbmodem2101 --baud 115200
```

Close other serial monitors before uploading. Record successful flash verification separately from application behavior: an upload does not prove API access, OLED appearance, voltage accuracy or physical SOS timing.

For changes affecting hardware, check the relevant behavior on the device: encoder gestures, startup silence, display layout, LED/buzzer timing, mute, reconnection and battery voltage against a multimeter. Record what was tested and what remains unverified in `VERIFICATION.md`.

The `oled-diagnostic` and `oled-diagnostic-32` environments replace the application with display diagnostics. Use them for display investigation, then restore `esp32-s3-zero`.

## API troubleshooting

Inspect the source and HTTP status before changing firmware. A valid HTTP response is different from a TLS/transport failure.

During the recorded device checks, SaveEcoBot public JSON returned a Cloudflare browser challenge (403) to the ESP32 despite working from the desktop. The configured Ukraine Alarm token also returned 401 for protected alert endpoints from the desktop. These observations do not establish that every credential or network will behave the same way. Do not mark an integration working based only on compilation or a desktop request; verify the authenticated device path and record limitations.

## Submitting changes

Keep changes focused and preserve unrelated local work. Describe the problem, resulting behavior, affected configuration or wiring, and the checks performed. State whether the firmware was uploaded and whether live API responses were verified. Include remaining limitations and sanitized reproduction steps for failures.

Update the relevant documentation with the implementation rather than leaving conflicting behavior descriptions. For bug reports, include the firmware revision, board, steps to reproduce, expected/actual behavior and relevant sanitized serial output. Display photos are useful for OLED issues; never include credentials in screenshots or logs.
