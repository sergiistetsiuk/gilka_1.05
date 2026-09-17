#pragma once
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <time.h>
#include "ServiceTrust.h"

namespace SectionServices
{
struct Config
{
  char weatherKey[129] = {};
  char alertKey[129] = {};
  char city[65] = "Cherkasy,UA";
  uint16_t alertRegion = 24;
  uint32_t revision = 0;
};
struct Snapshot
{
  uint32_t revision = 0;
  bool weatherValid = false;
  uint32_t weatherAt = 0;
  int64_t weatherObserved = 0;
  float temperature = 0, humidity = 0, pressure = 0, wind = 0;
  int weatherCode = 0;
  char weatherError[32] = "API key required";
  bool alertValid = false;
  uint32_t alertAt = 0;
  char alert = '?';
  char alertError[32] = "API token required";
};
static QueueHandle_t configQueue = nullptr, resultQueue = nullptr;
static bool available = false;

inline String encode(const char *value)
{
  String out;
  for (const uint8_t *p = reinterpret_cast<const uint8_t *>(value); *p; ++p)
  {
    if (isalnum(*p) || *p == '-' || *p == '_' || *p == '.') out += char(*p);
    else { char hex[4]; snprintf(hex, sizeof(hex), "%%%02X", *p); out += hex; }
  }
  return out;
}

inline bool getJSON(const String &url, const char *bearer, JsonDocument &doc,
                    char *error, size_t errorSize)
{
  if (time(nullptr) < 1700000000)
  { strlcpy(error, "Waiting for NTP", errorSize); return false; }
  WiFiClientSecure client;
  client.setCACert(SERVICE_ROOTS);
  client.setHandshakeTimeout(5);
  client.setTimeout(4); // WiFiClientSecure uses seconds; HTTPClient uses milliseconds.
  HTTPClient http;
  http.setConnectTimeout(4000);
  http.setTimeout(4000);
  http.useHTTP10(true);
  if (!http.begin(client, url))
  { strlcpy(error, "HTTPS init failed", errorSize); return false; }
  if (bearer && *bearer) http.addHeader("Authorization", String("Bearer ") + bearer);
  const int status = http.GET();
  if (status != 200)
  {
    snprintf(error, errorSize, "HTTP/TLS error %d", status);
    http.end(); return false;
  }
  if (http.getSize() > 8192)
  { strlcpy(error, "Response too large", errorSize); http.end(); return false; }
  DeserializationError result = deserializeJson(doc, http.getStream());
  http.end();
  if (result)
  { strlcpy(error, "Invalid API response", errorSize); return false; }
  error[0] = 0;
  return true;
}

inline void worker(void *)
{
  Config cfg;
  Snapshot snap;
  uint32_t weatherAttempt = 0, alertAttempt = 0;
  bool weatherDue = true, alertDue = true;
  for (;;)
  {
    Config next;
    if (xQueueReceive(configQueue, &next, 0) == pdTRUE)
    {
      cfg = next; snap = Snapshot{}; snap.revision = cfg.revision;
      weatherDue = alertDue = true;
      xQueueOverwrite(resultQueue, &snap);
    }
    const uint32_t now = millis();
    if (WiFi.status() == WL_CONNECTED)
    {
      // A background worker prevents DNS/TLS timeouts from blocking the encoder.
      if (*cfg.alertKey && (alertDue || uint32_t(now - alertAttempt) >= 30000))
      {
        alertDue = false; alertAttempt = now;
        DynamicJsonDocument doc(256);
        String url = "https://api.alerts.in.ua/v1/iot/active_air_raid_alerts/";
        url += cfg.alertRegion; url += ".json";
        if (getJSON(url, cfg.alertKey, doc, snap.alertError, sizeof(snap.alertError)))
        {
          const char *value = doc.as<const char *>();
          if (value && strlen(value) == 1 && strchr("APN", value[0]))
          { snap.alert = value[0]; snap.alertValid = true; snap.alertAt = millis(); }
          else strlcpy(snap.alertError, "Unknown alert response", sizeof(snap.alertError));
        }
        xQueueOverwrite(resultQueue, &snap);
      }
      if (*cfg.weatherKey && (weatherDue || uint32_t(now - weatherAttempt) >= (snap.weatherError[0] ? 30000u : 600000u)))
      {
        weatherDue = false; weatherAttempt = now;
        DynamicJsonDocument doc(6144);
        String url = "https://api.openweathermap.org/data/2.5/weather?q=";
        url += encode(cfg.city); url += "&units=metric&appid="; url += encode(cfg.weatherKey);
        if (getJSON(url, nullptr, doc, snap.weatherError, sizeof(snap.weatherError)))
        {
          if (doc["main"]["temp"].is<float>() && doc["main"]["humidity"].is<float>() &&
              doc["main"]["pressure"].is<float>() && doc["wind"]["speed"].is<float>() &&
              doc["weather"][0]["id"].is<int>() && doc["dt"].is<int64_t>())
          {
            snap.temperature = doc["main"]["temp"];
            snap.humidity = doc["main"]["humidity"];
            snap.pressure = doc["main"]["pressure"];
            snap.wind = doc["wind"]["speed"];
            snap.weatherCode = doc["weather"][0]["id"];
            snap.weatherObserved = doc["dt"];
            snap.weatherAt = millis(); snap.weatherValid = true;
          }
          else strlcpy(snap.weatherError, "Missing weather fields", sizeof(snap.weatherError));
        }
        xQueueOverwrite(resultQueue, &snap);
      }
    }
    vTaskDelay(pdMS_TO_TICKS(250));
  }
}

inline bool begin(const Config &cfg)
{
  configQueue = xQueueCreate(1, sizeof(Config));
  resultQueue = xQueueCreate(1, sizeof(Snapshot));
  if (!configQueue || !resultQueue) return false;
  xQueueOverwrite(configQueue, &cfg);
  available = xTaskCreate(worker, "section_services", 12288, nullptr, 1, nullptr) == pdPASS;
  return available;
}
inline void configure(const Config &cfg)
{ if (available) xQueueOverwrite(configQueue, &cfg); }
inline bool receive(Snapshot &snap)
{ return available && xQueueReceive(resultQueue, &snap, 0) == pdTRUE; }
}
