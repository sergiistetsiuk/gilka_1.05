#pragma once
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <time.h>
#include "ServiceTrust.h"
#include "SaveEcoBotData.h"
#include "UkraineAlarmData.h"
#include "SectionLogic.h"

namespace SectionServices
{
struct Config
{
  char weatherKey[129] = {};
  char alertKey[129] = {};
  char saveecobotKey[129] = {};
  char saveecobotPublicUrl[193] = {};
  char saveecobotCity[129] = "Cherkasy, Cherkasy Oblast";
  double saveecobotRadiusKm = 25;
  char city[65] = "Cherkasy,UA";
  char alertRegionName[129] = "Черкаська область";
  uint32_t revision = 0;
};
struct Snapshot
{
  uint32_t revision = 0;
  bool radiationValid = false;
  uint32_t radiationAt = 0;
  SaveEcoBotData::Measurement radiation;
  char radiationError[32] = "Waiting for SaveEcoBot";
  bool weatherValid = false;
  uint32_t weatherAt = 0;
  int64_t weatherObserved = 0;
  float temperature = 0, humidity = 0, pressure = 0, wind = 0;
  int weatherCode = 0;
  char weatherError[32] = "API key required";
  bool alertValid = false;
  uint32_t alertAt = 0;
  char alert = '?';
  char alertLabel[21] = {};
  char alertRegionId[24] = {};
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
                    char *error, size_t errorSize, const char *source = "API")
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
    snprintf(error, errorSize, "%s %s %d", source, status > 0 ? "HTTP" : "TLS/net", status);
    Serial.printf("[HTTP] %s status=%d\n", source, status);
    if (status == 403 && !strcmp(source, "Public"))
      strlcpy(error, "Public blocked 403", errorSize);
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

// Ukraine Alarm v3 uses a raw Authorization token, not an Alerts.in.ua bearer token.
inline bool getAlertJSON(const String &path, const char *token, JsonDocument &doc,
                         const JsonDocument &filter, char *error, size_t errorSize) {
  if (time(nullptr) < 1700000000) { strlcpy(error, "Waiting for NTP", errorSize); return false; }
  WiFiClientSecure client;
  client.setCACert(SERVICE_ROOTS); client.setHandshakeTimeout(5); client.setTimeout(4);
  HTTPClient http;
  http.setConnectTimeout(4000); http.setTimeout(4000); http.useHTTP10(true);
  if (!http.begin(client, "https://api.ukrainealarm.com/api/v3/" + path)) {
    strlcpy(error,"Alert HTTPS init",errorSize); return false;
  }
  http.addHeader("Authorization", token);
  http.addHeader("Accept", "application/json");
  int status = http.GET();
  if (status != 200) {
    snprintf(error,errorSize,"Alert %s %d", status > 0 ? "HTTP" : "TLS/net",status);
    http.end(); return false;
  }
  // /regions contains all communities; retain only the small state-level projection.
  if (http.getSize() > 524288) {
    strlcpy(error,"Alert response too big",errorSize); http.end(); return false;
  }
  auto result = deserializeJson(doc,http.getStream(),DeserializationOption::Filter(filter));
  http.end();
  if (result) { strlcpy(error,"Invalid alert JSON",errorSize); return false; }
  error[0]=0; return true;
}

// Stream one station/value at a time: the national station list must not fill ESP RAM.
inline bool ecoFetch(const char *key, const SaveEcoBotData::Area &area, bool details, SaveEcoBotData::Measurement &reading,
                     char *error, size_t errorSize)
{
  WiFiClientSecure client;
  client.setCACert(SERVICE_ROOTS); client.setHandshakeTimeout(5); client.setTimeout(4);
  HTTPClient http;
  http.setConnectTimeout(4000); http.setTimeout(4000); http.useHTTP10(true);
  String url = "https://www.saveecobot.com/api/v1/radiation-ukraine/";
  url += details ? "sensors-details" : "sensor-last-data/" + String(reading.id);
  url += "?apikey="; url += encode(key);
  if (!http.begin(client, url)) { strlcpy(error, "HTTPS init failed", errorSize); return false; }
  int status = http.GET();
  if (status != 200)
  { snprintf(error, errorSize, "SaveEcoBot HTTP %d", status); http.end(); return false; }
  auto &stream = http.getStream(); stream.setTimeout(4000);
  // The documented envelope is {"data":[...]} (no arbitrary prefix accepted).
  const char *prefix = "{\"data\":[";
  const uint32_t started = millis();
  auto nextChar = [&]() -> int {
    while (uint32_t(millis() - started) < 20000) {
      if (stream.available()) { int c = stream.read(); if (!isspace(c)) return c; }
      else if (!http.connected()) return -1;
      else vTaskDelay(pdMS_TO_TICKS(1));
    }
    return -1;
  };
  bool good = true;
  for (const char *c = prefix; *c; ++c) if (nextChar() != *c) { good = false; break; }
  DynamicJsonDocument filter(512), item(1024), envelope(1536);
  for (const char *field : {"sensor_id", "latitude", "longitude", "phenomenon", "value", "updated_at_utc", "is_old"}) filter[field] = true;
  SaveEcoBotData::Measurement candidate = details ? SaveEcoBotData::Measurement{} : reading;
  bool found = false, closed = false;
  for (unsigned count = 0; good && count < 10000; ++count)
  {
    // peek leaves the object's opening brace for ArduinoJson.
    while (uint32_t(millis() - started) < 20000) {
      if (stream.available()) {
        if (!isspace(stream.peek())) break;
        stream.read();
      } else if (!http.connected()) break;
      else vTaskDelay(pdMS_TO_TICKS(1));
    }
    if (stream.peek() == ']') { stream.read(); closed = true; break; }
    if (stream.peek() != '{' || uint32_t(millis() - started) >= 20000) { good = false; break; }
    if (deserializeJson(item, stream, DeserializationOption::Filter(filter))) { good = false; break; }
    if (details) { SaveEcoBotData::considerStation(item.as<JsonObjectConst>(), candidate, area); found = candidate.id != 0; }
    else {
      envelope.clear(); envelope["data"][0].set(item.as<JsonVariantConst>());
      if (SaveEcoBotData::reading(envelope.as<JsonVariantConst>(), time(nullptr), candidate)) found = true;
    }
    int delimiter = nextChar();
    if (delimiter == ']') { closed = true; break; }
    if (delimiter != ',') { good = false; break; }
  }
  good = good && closed && nextChar() == '}';
  http.end();
  if (!good || !found) { strlcpy(error, good ? "No nearby gamma data" : "Invalid API response", errorSize); return false; }
  reading = candidate; error[0] = 0; return true;
}

inline void worker(void *)
{
  Config cfg;
  Snapshot snap;
  SaveEcoBotData::Measurement station;
  SaveEcoBotData::Area area;
  bool cityResolved = false;
  char alertRegionId[24] = {};
  int64_t lastAlertIndex = -1;
  uint32_t alertFullAt = 0;
  uint32_t weatherAttempt = 0, alertAttempt = 0, radiationAttempt = 0;
  bool weatherDue = true, alertDue = true, radiationDue = true;
  for (;;)
  {
    Config next;
    if (xQueueReceive(configQueue, &next, 0) == pdTRUE)
    {
      cfg = next; station = SaveEcoBotData::Measurement{}; snap = Snapshot{}; snap.revision = cfg.revision;
      alertRegionId[0]=0; lastAlertIndex=-1; alertFullAt=0;
      cityResolved = false; area = SaveEcoBotData::Area{};
      area.radiusKm = cfg.saveecobotRadiusKm;
      weatherDue = alertDue = radiationDue = true;
      xQueueOverwrite(resultQueue, &snap);
    }
    const uint32_t now = millis();
    if (WiFi.status() == WL_CONNECTED)
    {
      // A background worker prevents DNS/TLS timeouts from blocking the encoder.
      if (*cfg.alertKey && (alertDue || uint32_t(now - alertAttempt) >= SectionLogic::ALERT_POLL_MS))
      {
        alertDue = false; alertAttempt = now;
        bool ok = true;
        if (!*alertRegionId) {
          DynamicJsonDocument filter(256), doc(8192);
          for (const char *field : {"regionId", "regionName", "regionType"}) filter["states"][0][field]=true;
          ok = getAlertJSON("regions",cfg.alertKey,doc,filter,snap.alertError,sizeof(snap.alertError));
          if (ok) {
            ok = UkraineAlarmData::regionId(doc.as<JsonVariantConst>(),cfg.alertRegionName,alertRegionId,sizeof(alertRegionId));
            if (!ok) strlcpy(snap.alertError,"Alert region not found",sizeof(snap.alertError));
            else strlcpy(snap.alertRegionId,alertRegionId,sizeof(snap.alertRegionId));
          }
        }
        int64_t index = -1;
        if (ok) {
          DynamicJsonDocument filter(128), doc(256);
          filter["lastActionIndex"]=true;
          ok = getAlertJSON("alerts/status",cfg.alertKey,doc,filter,snap.alertError,sizeof(snap.alertError));
          if (ok && !UkraineAlarmData::actionIndex(doc.as<JsonVariantConst>(),index)) {
            ok=false; strlcpy(snap.alertError,"Invalid alert index",sizeof(snap.alertError));
          }
        }
        if (ok && (index != lastAlertIndex || !snap.alertValid || uint32_t(now-alertFullAt)>=300000)) {
          DynamicJsonDocument filter(256), doc(8192);
          filter[0]["regionId"]=true;
          filter[0]["activeAlerts"][0]["type"]=true;
          ok = getAlertJSON(String("alerts/")+encode(alertRegionId),cfg.alertKey,doc,filter,snap.alertError,sizeof(snap.alertError));
          if (ok) {
            UkraineAlarmData::Result result;
            ok = UkraineAlarmData::alerts(doc.as<JsonVariantConst>(),alertRegionId,result);
            if (ok) {
              snap.alert=result.state; strlcpy(snap.alertLabel,result.label,sizeof(snap.alertLabel));
              snap.alertValid=true; lastAlertIndex=index; alertFullAt=millis();
            } else strlcpy(snap.alertError,"Invalid region alert",sizeof(snap.alertError));
          }
        }
        if (ok) snap.alertAt=millis();
        if (snap.alertError[0]) Serial.printf("[UkraineAlarm] %s\n",snap.alertError);
        else Serial.printf("[UkraineAlarm] region=%s status=%c type=%s\n",alertRegionId,snap.alert,snap.alertLabel);
        xQueueOverwrite(resultQueue, &snap);
      }
      if (time(nullptr) >= 1700000000 && (radiationDue || uint32_t(now - radiationAttempt) >= (snap.radiationError[0] && strcmp(snap.radiationError, "Public blocked 403") ? 60000u : 600000u)))
      {
        radiationDue = false; radiationAttempt = now;
        if (!cityResolved)
        {
          DynamicJsonDocument cityDoc(6144);
          String url = "https://geocoding-api.open-meteo.com/v1/search?countryCode=UA&count=10&language=en&name=";
          url += encode(cfg.saveecobotCity);
          if (getJSON(url, nullptr, cityDoc, snap.radiationError, sizeof(snap.radiationError), "City"))
          {
            cityResolved = SaveEcoBotData::cityArea(cityDoc.as<JsonVariantConst>(), area);
            if (!cityResolved) strlcpy(snap.radiationError, "City unclear: add oblast", sizeof(snap.radiationError));
          }
        }
        SaveEcoBotData::Measurement reading = station;
        if (cityResolved && !*cfg.saveecobotKey)
        {
          if (!*cfg.saveecobotPublicUrl)
            strlcpy(snap.radiationError, "Set public JSON URL", sizeof(snap.radiationError));
          else
          {
            DynamicJsonDocument publicDoc(8192);
            if (getJSON(String(cfg.saveecobotPublicUrl), nullptr, publicDoc, snap.radiationError, sizeof(snap.radiationError), "Public"))
            {
              if (SaveEcoBotData::publicReading(publicDoc.as<JsonVariantConst>(), area, time(nullptr), reading))
              { snap.radiation = reading; snap.radiationValid = true; snap.radiationAt = millis(); }
              else strlcpy(snap.radiationError, "No gamma within radius", sizeof(snap.radiationError));
            }
          }
        }
        else if (cityResolved)
        {
          bool resolved = station.id || ecoFetch(cfg.saveecobotKey, area, true, reading, snap.radiationError, sizeof(snap.radiationError));
          if (resolved)
          {
            station = reading;
            if (ecoFetch(cfg.saveecobotKey, area, false, reading, snap.radiationError, sizeof(snap.radiationError)))
            { snap.radiation = reading; snap.radiationValid = true; snap.radiationAt = millis(); }
          }
        }
        if (snap.radiationError[0]) Serial.printf("[SaveEcoBot] %s\n", snap.radiationError);
        else Serial.printf("[SaveEcoBot] station=%lu value=%.3f uSv/h UTC=%s old=%d public=%d\n",
          static_cast<unsigned long>(snap.radiation.id), snap.radiation.value, snap.radiation.date, snap.radiation.old, snap.radiation.publicSource);
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
