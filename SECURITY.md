# Security policy

GILKA is an ESP32-S3 firmware project intended for use on a trusted local network. This document describes the current implementation and its limitations; it is not a claim of a completed security audit or certification.

## Reporting a vulnerability

Report vulnerabilities privately to the repository maintainer before publishing exploit details or credentials. If this repository has GitHub private vulnerability reporting enabled, use **Security → Report a vulnerability**. Otherwise, use an existing private contact with the maintainer. No dedicated security email address or response-time commitment is currently documented by this project.

Include the affected revision, relevant configuration with secrets removed, prerequisites, reproduction steps, observed impact and any proposed mitigation. Test against your own device and authorized accounts. Do not include live API keys, Wi-Fi passwords, firmware dumps containing credentials or unredacted screenshots in public issues.

If no private contact is available, a public issue may request a private reporting channel without disclosing the vulnerability details.

## Supported revisions

Security fixes are developed against the current source revision. There is no separate published maintenance schedule for older firmware versions. When reporting an issue, identify the commit or build date as well as the device board; the startup version label alone does not uniquely identify a build.

## Credentials and local files

The firmware can contain Wi-Fi credentials and keys for OpenWeatherMap, SaveEcoBot and Ukraine Alarm. Current service configuration names include `OPENWEATHERMAP_API_KEY`, `SAVEECOBOT_API_KEY` and `UKRAINEALARM_API_KEY`.

- `.env` and `.pio/` are excluded from Git. `.env.example` contains example/default configuration, not personal credentials.
- `scripts/env_config.py` parses literal values without executing shell commands or expanding variables. It validates lengths and characters before generating C++ defaults.
- Generated `FirmwareDefaults.h` is stored under `.pio/build/<environment>/generated/` with owner-only file permissions. Values are not included in compiler command-line flags.
- Credentials are still embedded in firmware binaries. Encoding generated strings is not encryption. Do not distribute firmware built with personal secrets.
- Wi-Fi settings and API overrides are stored using ESP32 Preferences/NVS. The project configuration does not enable flash encryption, NVS encryption or secure boot; do not assume protection against physical flash extraction.
- Saved NVS settings take precedence over compiled defaults where supported. Changing `.env` alone does not replace an existing NVS key. The web form's removal checkbox clears the saved API value, but does not remove a credential already embedded in the firmware image.

Keep credentials out of logs, command-line arguments, fixtures, screenshots and issue reports. Password input fields hide typing on screen; they do not encrypt network traffic or flash storage. Each provider's key is separate and must only be sent to that provider.

## Network interfaces

| Interface | Current behavior and trust boundary |
| --- | --- |
| Local web configuration | HTTP on port 80; no application login or CSRF token checks are implemented |
| Setup access point | `GILKA-SETUP`, shared default password `gilka1234`; these are public defaults, not a unique device secret |
| Captive portal | Local DNS/HTTP redirection while the setup AP is active |
| mDNS | Advertises the device as `gilka.local` on the local network |
| Outbound services | HTTPS requests with CA certificate verification |
| OTA | Enabled from the physical settings menu, with a temporary password and a limited waiting window |

The HTTP routes `/save`, `/forget`, `/display` and `/sections` change device settings. They use POST, but POST alone does not provide authentication or CSRF protection. A party able to reach the web interface can attempt configuration changes. Credentials submitted through these pages travel over HTTP.

Use the device and its configuration interface on a trusted network. Do not expose its web or OTA ports through Internet port forwarding. The setup AP can appear at startup when connection fails, after a prolonged Wi-Fi outage, or through the device menu; account for access by nearby users who know the default password. For a deployment requiring stronger isolation or authentication, those controls must be added rather than assumed to exist.

## OTA and physical access

OTA is armed through **SYS: SETTINGS → OTA UPDATE** while Wi-Fi is connected. Each arming generates an eight-character hexadecimal password from `esp_random()` and displays it on the OLED. The waiting window is 120 seconds. Leaving the screen, cancelling or losing Wi-Fi stops the service when a transfer is not already in progress; an active transfer can extend beyond the waiting window.

This is ArduinoOTA password authentication, not HTTPS transport or a project-level signed-firmware verification scheme. The password has 32 bits of random input and is visible to someone looking at the display. Physical access also permits USB flashing and access to the board. Use OTA on a trusted network and keep both the device and credentials under your control.

The application silences alarm sound during an OTA transfer. Do not treat the device as providing uninterrupted alarm coverage while updating it.

## Outbound API security

TLS verification uses roots in `include/ServiceTrust.h`; provenance is recorded in [TLS_ROOTS.md](docs/TLS_ROOTS.md). The firmware waits for network time before verified HTTPS. Preserve certificate validation when diagnosing connection errors.

Ukraine Alarm receives its token in the `Authorization` header without a Bearer prefix. SaveEcoBot's authenticated API and OpenWeatherMap use credentials in HTTPS query parameters. Do not log complete authenticated URLs; the destination service and any endpoint-side request logs can see query parameters.

The configurable public SaveEcoBot fallback URL is restricted by the build parser to HTTPS station JSON addresses on `www.saveecobot.com`. The current implementation does not follow redirects. JSON handling uses bounded documents and response-size checks; these are resource controls, not proof that every possible response has been security-tested.

A valid HTTP error response is distinct from a TLS failure:

- A protected Ukraine Alarm endpoint returning 401 requires investigation of the token and provider access. Reflashing the same rejected credential does not establish access.
- SaveEcoBot public JSON has returned a Cloudflare browser challenge (403) to this ESP32 while desktop access succeeded. The firmware reports `Public blocked 403` and backs off to ten-minute retries. It does not solve browser challenges or disable TLS verification.

These are recorded observations for the tested device/configuration, not guarantees about all networks or credentials.

## Integrity of displayed alerts and measurements

GILKA is an additional display, not a replacement for official warning channels or a certified radiation instrument. Network availability, provider data, configuration and the local device all affect what it can show.

The current implementation:

- Checks Ukraine Alarm changes every minute and marks data stale after 180 seconds without successful verification, immediately on a request error, or while offline.
- Does not interpret missing regions, null alert arrays, unknown types or malformed responses as an all-clear. A received threat remains latched until a confirmed clear/informational state. Mute stops alert sound while visual indication continues.
- Displays supported threat types and drives SOS subject to the configured sound mode. MUTE, CLICKS ONLY, an OTA transfer or user mute can suppress alarm sound.
- Distinguishes remote radiation values from locally measured battery voltage. It preserves radiation source time and old/stale flags, checks location/radius, and does not generate substitute radiation measurements.

An unknown or stale state must not be presented as proof that conditions are safe. API parser changes should include tests for malformed, missing, foreign-region, stale and unsupported data, not only successful responses.

## If a credential is exposed

1. Revoke or rotate it with the issuing provider; rotate the Wi-Fi password if that was exposed.
2. Replace the local value and any saved NVS override. Build and flash replacement firmware if the old credential was embedded in an image.
3. Remove exposed files or logs from shared locations and review repository history and published artifacts. Deleting a file from the latest commit does not remove earlier copies.
4. If transferring the device to another owner, remove personal configuration and use a build without personal defaults. A blank web form does not erase existing keys.

Coordinate any repository-history cleanup with collaborators. Firmware upload and application-level key removal should not be represented as a verified secure erase of flash.

## Changes affecting security

Review changes to network routes, authentication, OTA, TLS roots, configuration parsing, NVS persistence and API response handling for their impact on these boundaries. Keep this document aligned with the implementation and record what was actually verified in [VERIFICATION.md](VERIFICATION.md).

See [CONTRIBUTING.md](CONTRIBUTING.md) for build/test instructions and [HARDWARE.md](docs/HARDWARE.md) for electrical connections and power-path limitations.
