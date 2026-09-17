#pragma once

#include <Arduino.h>
#include <atomic>
#include <driver/gpio.h>
#include <esp_timer.h>

// Three-pin active-low buzzer module with a 3.3V-compatible input.
// A one-shot timer ends the pulse even while OLED/I2C or HTTP blocks loop().
namespace EncoderBuzzer
{
constexpr gpio_num_t PIN = GPIO_NUM_7;
constexpr uint32_t ROTATE_MS = 8;
constexpr uint32_t PRESS_MS = 30;

static esp_timer_handle_t timer = nullptr;
static std::atomic<bool> active{false};

inline void stop(void *)
{
  gpio_set_level(PIN, 1);
  active.store(false);
}

inline void begin()
{
  // Set the output latch before enabling the output: HIGH means silent.
  digitalWrite(static_cast<uint8_t>(PIN), HIGH);
  pinMode(static_cast<uint8_t>(PIN), OUTPUT);
  gpio_set_level(PIN, 1);

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
  if (!timer || active.exchange(true))
    return;

  gpio_set_level(PIN, 0);
  if (esp_timer_start_once(timer, static_cast<uint64_t>(durationMs) * 1000) != ESP_OK)
    stop(nullptr);
}
}
