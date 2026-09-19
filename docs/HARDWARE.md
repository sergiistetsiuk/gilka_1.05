# GILKA hardware

This describes the current ESP32-S3 build and the pin assignments in the firmware. Pin numbers are **GPIO numbers**, not physical header positions. All modules share GND.

## Controller and parts

| Part | Current configuration |
| --- | --- |
| Controller | ESP32-S3 Zero, ESP32-S3FH4R2; 4 MB flash, 2 MB embedded PSRAM |
| Display | SSD1306-compatible 128 × 64 I²C OLED, blue/yellow physical zones |
| Input | Rotary encoder with push button |
| Buzzer | MH-FMD active buzzer module with built-in transistor, low-level trigger |
| External indicator | LED on GPIO8 with a 330 Ω series resistor |
| Onboard indicator | Addressable RGB LED on GPIO21 |
| Battery | Confirmed 1S Li-ion, 6000 mAh, maximum 4.2 V |
| Voltage measurement | Two 100 kΩ resistors forming a 1:1 divider to GPIO9 |

PlatformIO environment: `esp32-s3-zero`. The project uses the `esp32-s3-devkitm-1` board definition with explicit **4 MB flash** overrides. PSRAM is not enabled by the current build configuration. USB serial monitor speed is 115200 baud.

## Wiring

| Connection | ESP32 / destination |
| --- | --- |
| OLED VCC | 3V3 |
| OLED GND | GND |
| OLED SDA | GPIO1 |
| OLED SCL | GPIO2 |
| Encoder A / CLK | GPIO5 |
| Encoder B / DT | GPIO4 |
| Encoder SW | GPIO6 |
| Encoder common / GND | GND |
| Buzzer VCC | **3V3**, as confirmed working with this module |
| Buzzer GND | GND |
| Buzzer I/O / IN / S | GPIO7 |
| External LED anode (+) | GPIO8 through 330 Ω |
| External LED cathode (−) | GND |
| Battery sense divider midpoint | GPIO9 |
| Onboard RGB LED | GPIO21; already on the board |

Encoder contacts use internal pull-ups and switch to GND. A bare mechanical encoder does not require a VCC connection. If using an encoder breakout with pull-ups, its signals must use 3.3 V logic.

## OLED

Firmware settings in [main.cpp](../src/main.cpp):

- SDA GPIO1, SCL GPIO2; I²C clock 100 kHz.
- Probes addresses `0x3C` and `0x3D`.
- Framebuffer: 128 × 64; rotation `2` (180°); COM configuration `0x12`.
- Current text workaround uses `setTextSize(1, 2)` following the reported missing text strokes on this panel.
- Main content is in the blue area; separator at y=47; status content in the yellow area below it. The colors are physical panel zones, not software-selectable colors.

The startup screen is inverted for two seconds: black “Гілка.ос” and “V1.05” lettering on a lit background. The normal status bar shows a three-segment battery icon at the lower left.

The `oled-diagnostic` and `oled-diagnostic-32` PlatformIO environments are display tests, not the main application firmware.

## Buzzer

The tested MH-FMD module is **active-low**:

| GPIO7 level | Behavior |
| --- | --- |
| HIGH | Silent |
| LOW | Sound enabled |

The module has a built-in transistor interface. Operation with **VCC connected to 3V3** was confirmed by the user. Earlier direct GPIO control with the module powered from 5 V caused continuous sound; this document records the working 3V3 arrangement.

Firmware sets GPIO7 HIGH before enabling the output and before other startup initialization. This establishes silence once firmware starts; it does not establish a guaranteed pin level during reset before firmware execution.

Encoder feedback uses short pulses; sound modes are MUTE, LOW, MAX and CLICKS ONLY. LOW/MAX change pulse behavior, not calibrated acoustic volume. One-shot beeps use an ESP timer to stop the pulse independently of main-loop work. Alarm sound is disabled in MUTE/CLICKS ONLY and during an OTA transfer.

Implementation: [EncoderBuzzer.h](../include/EncoderBuzzer.h), [Sections.h](../include/Sections.h).

## LED and alarm patterns

```text
GPIO8 ── 330 Ω ── LED anode (+)
                  LED cathode (−) ── GND
```

GPIO8 HIGH lights the LED; the output is initialized LOW at boot.

| State | External GPIO8 LED | Onboard LED / buzzer |
| --- | --- | --- |
| Running timer or stopwatch | 150 ms flash each elapsed second | Timer minute feedback follows sound settings |
| Paused timer | Heartbeat stops | No timer heartbeat sound |
| Timer finished | 350 ms on / 350 ms off | Onboard red indication follows the same flash; buzzer emits periodic short beeps |
| Active Ukraine Alarm threat | Repeating SOS | Onboard red LED and enabled buzzer follow the same SOS phase |
| Alert muted by button | SOS continues | Onboard SOS continues; alert sound stops |
| DOSIMETER | No simulated radiation flashes | No simulated Geiger clicks |
| Idle | Off | No active alarm indication |

An active regional threat takes priority over the timer LED pattern. Its SOS starts at the first dot:

```text
···   ———   ···       (repeat)
```

- Dot: 200 ms on; dash: 600 ms on.
- Gap between symbols: 200 ms.
- Gap between letters: 600 ms.
- Gap before repeating SOS: 1400 ms.
- Complete cycle: 6800 ms.

This is the current firmware behavior in [SectionLogic.h](../include/SectionLogic.h) and [Sections.h](../include/Sections.h): **350/350 ms belongs to timer completion; regional alerts use SOS**. Timing is driven by the main loop without blocking delays, so loop work can affect exact output transition times. SOS logic is tested; physical timing and brightness still need confirmation on the assembled unit.

GPIO21 is an addressable LED, not a plain LED output. The firmware accounts for the board's color order when producing red.

## Battery voltage divider

Confirmed wiring:

```text
Battery + (1S, up to 4.2 V)
    │
  100 kΩ
    │
    ├──────── GPIO9 (ADC)
    │
  100 kΩ
    │
Battery − ─── common GND
```

Equal resistors halve the voltage: 4.2 V at the battery produces approximately 2.1 V at GPIO9. Firmware calculates `battery millivolts = ADC millivolts × 2`. The battery must not be connected directly to GPIO9.

Measurement uses `analogReadMilliVolts`, `ADC_11db`, 16-sample averaging and smoothing, updated once per second. The three display segments use these nominal thresholds, with 30 mV hysteresis:

| Battery voltage | Filled segments |
| --- | --- |
| Below 3.40 V | 0 |
| 3.40–3.69 V | 1 |
| 3.70–3.94 V | 2 |
| At least 3.95 V | 3 |

Readings outside 2.50–4.35 V are treated as invalid. SYS: SETTINGS → BATTERY shows voltage, the configured 6000 mAh capacity and an approximate percentage based on 3.2–4.2 V. This is a voltage estimate, not a charging detector, capacity measurement or runtime calculation. Compare the displayed voltage with a multimeter to check calibration.

Implementation: [BatteryMonitor.h](../include/BatteryMonitor.h), [BatteryGauge.h](../include/BatteryGauge.h).

## Power path and assembly verification

The divider is a **measurement connection only**. The battery charging, protection and regulator wiring has not been documented or verified in this project; the divider diagram does not specify how to power the ESP32 from the cell. USB is used for firmware upload. Confirm the actual power circuit before connecting battery power to a board supply pin.

Recorded checks:

- User confirmed working 3V3 buzzer operation and the 1S / two-100-kΩ battery divider.
- USB upload identifies an ESP32-S3 with 4 MB flash and embedded 2 MB PSRAM.
- Current firmware assigns GPIO8 to the external LED; resistor installation, LED brightness and battery ADC calibration require physical confirmation.
- Weather, radiation and regional threats are network data. No local radiation sensor is connected.

See [VERIFICATION.md](../VERIFICATION.md) for build/upload results and remaining device checks, and [README.md](../README.md) for operation and API configuration.
