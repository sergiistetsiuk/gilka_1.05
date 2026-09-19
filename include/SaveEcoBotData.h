#pragma once
#include <ArduinoJson.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

namespace SaveEcoBotData
{
struct Area { double latitude = 0, longitude = 0, radiusKm = 25; };
struct Measurement
{
  double value = 0, distanceKm = 0;
  int64_t captured = 0;
  uint32_t id = 0;
  bool old = true;
  bool publicSource = false;
  char date[18] = {};
};
inline bool leap(int y) { return y % 4 == 0 && (y % 100 != 0 || y % 400 == 0); }
inline int64_t timestamp(const char *s)
{
  if (!s || strlen(s) < 20) return 0;
  int y, m, d, h, n, sec, used = 0;
  if (sscanf(s, "%4d-%2d-%2dT%2d:%2d:%2d%n", &y, &m, &d, &h, &n, &sec, &used) != 6 || used != 19) return 0;
  const char *zone = s + used;
  if (*zone == '.') { ++zone; if (*zone < '0' || *zone > '9') return 0; while (*zone >= '0' && *zone <= '9') ++zone; }
  if (strcmp(zone, "Z") && strcmp(zone, "+00:00")) return 0; // API supplies UTC.
  if (y < 2000 || y > 2099 || m < 1 || m > 12 || h < 0 || h > 23 || n < 0 || n > 59 || sec < 0 || sec > 59) return 0;
  const int monthDays[] = {31,28,31,30,31,30,31,31,30,31,30,31};
  if (d < 1 || d > monthDays[m - 1] + (m == 2 && leap(y))) return 0;
  int64_t days = 0;
  for (int year = 1970; year < y; ++year) days += leap(year) ? 366 : 365;
  for (int month = 1; month < m; ++month) days += monthDays[month - 1] + (month == 2 && leap(y));
  return ((days + d - 1) * 24 + h) * 3600 + n * 60 + sec;
}
inline double distance(double lat, double lon, const Area &area)
{
  constexpr double rad = 3.141592653589793 / 180;
  double a = pow(sin((lat - area.latitude) * rad / 2), 2) +
             cos(lat * rad) * cos(area.latitude * rad) * pow(sin((lon - area.longitude) * rad / 2), 2);
  return 6371 * 2 * asin(sqrt(fmin(1.0, fmax(0.0, a))));
}
inline bool coordinate(JsonVariantConst v, double &out)
{
  if (v.is<double>()) out = v.as<double>();
  else if (v.is<const char *>())
  {
    const char *s = v.as<const char *>(); char *end;
    out = strtod(s, &end);
    if (end == s || *end) return false;
  }
  else return false;
  return isfinite(out);
}
inline bool cityArea(JsonVariantConst root, Area &area)
{
  JsonArrayConst cities = root["results"].as<JsonArrayConst>();
  JsonObjectConst city;
  for (JsonObjectConst candidate : cities)
  {
    if (strcmp(candidate["country_code"] | "", "UA") ||
        strncmp(candidate["feature_code"] | "", "PPL", 3)) continue;
    if (!city.isNull()) return false; // Require an oblast qualifier for namesakes.
    city = candidate;
  }
  if (city.isNull()) return false;
  double lat, lon;
  if (strcmp(city["country_code"] | "", "UA") ||
      strncmp(city["feature_code"] | "", "PPL", 3) ||
      !coordinate(city["latitude"], lat) || !coordinate(city["longitude"], lon) ||
      fabs(lat) > 90 || fabs(lon) > 180) return false;
  area.latitude = lat; area.longitude = lon; return true;
}
inline bool considerStation(JsonObjectConst item, Measurement &best, const Area &area)
{
  double lat, lon;
  if (!item["sensor_id"].is<uint32_t>() || !item["sensor_id"].as<uint32_t>() ||
      !coordinate(item["latitude"], lat) || !coordinate(item["longitude"], lon) ||
      fabs(lat) > 90 || fabs(lon) > 180) return false;
  const double km = distance(lat, lon, area);
  if (km > area.radiusKm || (best.id && km >= best.distanceKm)) return false;
  best.id = item["sensor_id"]; best.distanceKm = km;
  return true;
}
inline bool reading(JsonVariantConst root, int64_t now, Measurement &out)
{
  if (!root["data"].is<JsonArrayConst>()) return false;
  for (JsonObjectConst item : root["data"].as<JsonArrayConst>())
  {
    // The canonical phenomenon specifies nSv/h independently of localized unit labels.
    if (strcmp(item["phenomenon"] | "", "gamma_nsv_h") || !item["value"].is<double>()) continue;
    const double value = item["value"];
    const char *utc = item["updated_at_utc"] | "";
    const int64_t stamp = timestamp(utc);
    if (!isfinite(value) || value < 0 || !stamp || stamp > now + 300) continue;
    out.value = value / 1000.0; out.captured = stamp;
    out.old = !item["is_old"].is<int>() || item["is_old"].as<int>() != 0;
    memcpy(out.date, utc, 16); out.date[10] = ' '; out.date[16] = 'Z'; out.date[17] = 0;
    return true;
  }
  return false;
}
inline bool publicReading(JsonVariantConst root, const Area &area, int64_t now, Measurement &out)
{
  double lat, lon;
  if (!root["id"].is<uint32_t>() || !root["id"].as<uint32_t>() ||
      !coordinate(root["latitude"], lat) || !coordinate(root["longitude"], lon) ||
      fabs(lat) > 90 || fabs(lon) > 180 || !root["last_data"].is<JsonArrayConst>()) return false;
  const double km = distance(lat, lon, area);
  if (km > area.radiusKm) return false;
  bool found = false;
  Measurement best;
  for (JsonObjectConst item : root["last_data"].as<JsonArrayConst>())
  {
    // Public SaveEcoBot's canonical gamma field is nSv/h (not CPM or air quality).
    if (strcmp(item["phenomenon"] | "", "gamma") || !item["value"].is<double>()) continue;
    const double value = item["value"];
    const char *date = item["updated_at"] | "";
    if (strlen(date) != 19 || date[10] != ' ') continue;
    char utc[21]; memcpy(utc, date, 19); utc[10] = 'T'; utc[19] = 'Z'; utc[20] = 0;
    // Public timestamps match the map's gamma_t UNIX UTC timestamps.
    int64_t stamp = timestamp(utc);
    if (!isfinite(value) || value < 0 || !stamp || stamp > now + 300 || (found && stamp <= best.captured)) continue;
    best.value = value / 1000.0; best.captured = stamp; best.id = root["id"];
    best.distanceKm = km; best.publicSource = true;
    best.old = item["is_old"].is<bool>() ? item["is_old"].as<bool>() :
      !item["is_old"].is<int>() || item["is_old"].as<int>() != 0;
    memcpy(best.date, date, 16); best.date[16] = 'Z'; best.date[17] = 0;
    found = true;
  }
  if (found) out = best;
  return found;
}

}
