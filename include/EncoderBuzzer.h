#pragma once

#include <Arduino.h>
#include <atomic>
#include <driver/gpio.h>
#include <esp_timer.h>

// MH-FMD active-low module, confirmed working with VCC connected to 3V3.
// A one-shot timer ends the pulse even while OLED/I2C or HTTP blocks loop().
namespace EncoderBuzzer
{
constexpr gpio_num_t PIN = GPIO_NUM_7;
constexpr uint8_t ON_LEVEL = LOW;
constexpr uint8_t OFF_LEVEL = HIGH;
// Longer test pulses give the active buzzer time to start at the lower supply.
constexpr uint32_t ROTATE_MS = 20;
constexpr uint32_t PRESS_MS = 80;

static esp_timer_handle_t timer = nullptr;
static std::atomic<bool> active{false};
static std::atomic<bool> alarm{false};
static portMUX_TYPE outputMux = portMUX_INITIALIZER_UNLOCKED;

inline void stop(void *)
{
  portENTER_CRITICAL(&outputMux);
  gpio_set_level(PIN, alarm.load() ? ON_LEVEL : OFF_LEVEL);
  active.store(false);
  portEXIT_CRITICAL(&outputMux);
}

inline void silenceOutput()
{
  // Set the output latch before enabling the output: HIGH means silent.
  digitalWrite(static_cast<uint8_t>(PIN), OFF_LEVEL);
  pinMode(static_cast<uint8_t>(PIN), OUTPUT);
  gpio_set_level(PIN, OFF_LEVEL);
}

inline void begin()
{
  silenceOutput();

  esp_timer_create_args_t args = {};
  args.callback = stop;
  args.dispatch_method = ESP_TIMER_TASK;
  args.name = "encoder_beep";
  if (esp_timer_create(&args, &timer) != ESP_OK)
  {
    timer = nullptr;
    Serial.println("[Buzzer] timer unavailable; sound disabled");
  }
}

inline void beep(uint32_t durationMs)
{
  // Do not merge rapid detents into a continuous tone.
  if (!timer || alarm.load() || active.exchange(true))
    return;

  gpio_set_level(PIN, ON_LEVEL);
  if (esp_timer_start_once(timer, static_cast<uint64_t>(durationMs) * 1000) != ESP_OK)
    stop(nullptr);
}

inline void setAlarm(bool enabled)
{
  if (alarm.load() == enabled) return;
  if (timer) esp_timer_stop(timer);
  portENTER_CRITICAL(&outputMux);
  alarm.store(enabled);
  active.store(false);
  gpio_set_level(PIN, enabled ? ON_LEVEL : OFF_LEVEL);
  portEXIT_CRITICAL(&outputMux);
}

inline void cancel()
{
  if (timer) esp_timer_stop(timer);
  portENTER_CRITICAL(&outputMux);
  alarm.store(false);
  active.store(false);
  gpio_set_level(PIN, OFF_LEVEL);
  portEXIT_CRITICAL(&outputMux);
}
}
