#pragma once
#include <ArduinoJson.h>
#include <stdint.h>
#include <string.h>

namespace UkraineAlarmData {
inline bool regionId(JsonVariantConst root, const char *name, char *out, size_t size) {
  if (!root["states"].is<JsonArrayConst>()) return false;
  const char *id = nullptr;
  for (JsonObjectConst item : root["states"].as<JsonArrayConst>()) {
    if (strcmp(item["regionType"] | "", "State") || strcmp(item["regionName"] | "", name)) continue;
    if (id) return false;
    id = item["regionId"] | "";
    if (!*id || strlen(id) >= size) return false;
    for (const char *c = id; *c; ++c) if (*c < '0' || *c > '9') return false;
  }
  if (!id) return false;
  strcpy(out, id); return true;
}
inline bool actionIndex(JsonVariantConst root, int64_t &out) {
  if (!root["lastActionIndex"].is<int64_t>()) return false;
  int64_t value = root["lastActionIndex"];
  if (value < 0) return false;
  out = value; return true;
}
struct Result { char state = '?'; char label[21] = {}; };
inline bool alerts(JsonVariantConst root, const char *region, Result &out) {
  if (!root.is<JsonArrayConst>()) return false;
  bool found = false, info = false;
  int priority = 0;
  const char *label = "No reported alert";
  for (JsonObjectConst item : root.as<JsonArrayConst>()) {
    if (strcmp(item["regionId"] | "", region)) continue;
    if (found || !item["activeAlerts"].is<JsonArrayConst>()) return false;
    found = true;
    for (JsonVariantConst entry : item["activeAlerts"].as<JsonArrayConst>()) {
      if (!entry.is<JsonObjectConst>()) return false;
      const char *type = entry["type"] | "";
      int level; const char *text;
      if (!strcmp(type,"AIR")) { level=4; text="AIR RAID ALERT"; }
      else if (!strcmp(type,"ARTILLERY")) { level=3; text="ARTILLERY ALERT"; }
      else if (!strcmp(type,"URBAN_FIGHTS")) { level=2; text="URBAN FIGHTS"; }
      else if (!strcmp(type,"CHEMICAL")) { level=5; text="CHEMICAL ALERT"; }
      else if (!strcmp(type,"NUCLEAR")) { level=6; text="NUCLEAR ALERT"; }
      else if (!strcmp(type,"CUSTOM")) { level=1; text="REGIONAL ALERT"; }
      else if (!strcmp(type,"INFO")) { info=true; continue; }
      else return false; // Unknown types must not clear a latched threat.
      if (level > priority) { priority=level; label=text; }
    }
  }
  if (!found) return false;
  Result result;
  result.state = priority ? 'A' : info ? 'I' : 'N';
  strcpy(result.label, priority ? label : info ? "INFO MESSAGE" : label);
  out=result; return true;
}
}
