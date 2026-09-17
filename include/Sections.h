#pragma once
#include <ArduinoOTA.h>
#include <esp32-hal-rgb-led.h>
#include <time.h>
#include <math.h>
#include "SectionLogic.h"
#include "SectionServices.h"

#ifndef GILKA_RADAR_PIN
#define GILKA_RADAR_PIN -1
#endif
#ifndef GILKA_BATTERY_PIN
#define GILKA_BATTERY_PIN -1
#endif
#ifndef GILKA_BATTERY_RATIO
#define GILKA_BATTERY_RATIO 0.0f
#endif

namespace Sections
{
enum Section { TIMER, CLOCK, WEATHER, DOSIMETER, ALERT, SETTINGS, COUNT };
enum Sound { MUTE, LOW_SOUND, MAX_SOUND, CLICKS_ONLY };
enum Setting { SOUND, RADAR, DISPLAY_SETTINGS, WIFI_CONFIG, OTA, BATTERY, WIFI_INFO, SYS_COUNT };
static const char *titles[] = {"TACTICAL: TIMER", "TIME & DATE", "METEO: SECTOR", "DOSIMETER", "SURGE: ALERT", "SYS: SETTINGS"};
static const char *soundNames[] = {"MUTE", "LOW", "MAX", "CLICKS ONLY"};
static bool ready = false, selecting = false;
static int current = CLOCK, highlighted = CLOCK, sysIndex = 0, sysTop = 0;
static Sound sound = LOW_SOUND;
static int clockFormat = 0, weatherPage = 0;
static bool radiationRoentgen = false;
static int demoDoseIndex = 1;
static const float demoDoses[] = {0.05f, 0.12f, 0.30f, 1.0f, 3.0f};
static uint32_t radarHold = 15;
static int settingEdit = 0;
static SectionLogic::Timer timer;
static int preset = 1;
static const uint32_t minutes[] = {0, 1, 5, 15, 25};
static uint32_t refreshAt = 0, lastRadar = 0;
static bool radarDetected = false;
static uint8_t minuteBeeps = 0;
static uint32_t nextMinuteBeep = 0, nextGeiger = 0;
static bool alertLatched = false, alertMuted = false;
static bool otaEnabled = false, otaBusy = false;
static uint32_t otaStarted = 0;
static String otaPassword;
static String timezone = "EET-2EEST,M3.5.0/3,M10.5.0/4";
static SectionServices::Config config;
static SectionServices::Snapshot data;
static bool ledWasOn = false;

inline bool alarmsAllowed() { return sound == LOW_SOUND || sound == MAX_SOUND; }
inline void feedback(bool press)
{
  if (sound == MUTE) return;
  EncoderBuzzer::beep(press ? (sound == MAX_SOUND ? 160 : 80) : (sound == MAX_SOUND ? 50 : 20));
}
inline void save()
{
  preferences.putUChar("sound", sound);
  preferences.putUInt("radHold", radarHold);
  preferences.putUChar("clockFmt", clockFormat);
  preferences.putBool("radUnit", radiationRoentgen);
}
inline bool alertFresh()
{ return WiFi.status() == WL_CONNECTED && SectionLogic::fresh(millis(), data.alertAt, data.alertValid, 90000); }
inline bool weatherFresh()
{
  const time_t now = time(nullptr);
  return WiFi.status() == WL_CONNECTED && SectionLogic::fresh(millis(), data.weatherAt, data.weatherValid, 1200000) &&
         now >= data.weatherObserved && now - data.weatherObserved <= 7200;
}
inline String durationText(uint32_t seconds)
{
  char text[20];
  snprintf(text, sizeof(text), "%02lu:%02lu", static_cast<unsigned long>(seconds / 60), static_cast<unsigned long>(seconds % 60));
  return text;
}
inline String settingName(int i)
{
  switch (i)
  {
    case SOUND: return "SOUND: " + String(soundNames[sound]);
    case RADAR: return "RADAR SENS";
    case DISPLAY_SETTINGS: return "DISPLAY";
    case WIFI_CONFIG: return "WI-FI CONFIG";
    case OTA: return "OTA UPDATE";
    case BATTERY: return "BATTERY";
    case WIFI_INFO: return "WI-FI INFO";
  }
  return "";
}
inline void stopOTA()
{
  if (otaEnabled && !otaBusy) { ArduinoOTA.end(); otaEnabled = false; otaPassword = ""; }
}
inline void armOTA()
{
  if (otaEnabled || WiFi.status() != WL_CONNECTED) return;
  char password[9]; snprintf(password, sizeof(password), "%08lx", static_cast<unsigned long>(esp_random()));
  otaPassword = password;
  ArduinoOTA.setHostname(HOSTNAME);
  ArduinoOTA.setMdnsEnabled(false); // Main Wi-Fi state machine owns mDNS.
  ArduinoOTA.setPassword(otaPassword.c_str());
  ArduinoOTA.onStart([]() { otaBusy = true; EncoderBuzzer::cancel(); });
  ArduinoOTA.onEnd([]() { otaBusy = false; });
  ArduinoOTA.onError([](ota_error_t) { otaBusy = false; });
  ArduinoOTA.begin();
  otaStarted = millis(); otaEnabled = true;
}
inline void weatherIcon(int code)
{
  // Two-pixel strokes remain visible on the panel's missing-row workaround.
  const int x = 116, y = 6;
  if (code == 800)
  {
    display.fillCircle(x, y, 4, SSD1306_WHITE);
    display.fillRect(x - 1, 0, 2, 2, SSD1306_WHITE);
    display.fillRect(x - 1, 12, 2, 2, SSD1306_WHITE);
  }
  else
  {
    display.fillCircle(x - 4, y + 2, 4, SSD1306_WHITE);
    display.fillCircle(x + 2, y, 5, SSD1306_WHITE);
    display.fillRect(x - 7, y + 2, 15, 4, SSD1306_WHITE);
    if (code >= 200 && code < 700)
      for (int offset = -5; offset <= 5; offset += 5) display.fillRect(x + offset, 12, 2, 2, SSD1306_WHITE);
  }
}
inline void render()
{
  oledBegin();
  if (timer.alarm && !selecting)
  {
    oledTextRow(0, "TIMER COMPLETE");
    oledTextRow(1, "00:00");
    oledTextRow(2, "Click: stop alarm");
    oledStatusBar("ALARM");
  }
  else if (selecting)
  {
    oledTextRow(0, String(highlighted + 1) + "/6 " + titles[highlighted]);
    oledTextRow(1, "Hold + rotate");
    oledTextRow(2, "Release to open");
    oledStatusBar("SELECT");
  }
  else if (uiMode == UI_SYS)
  {
    if (sysIndex < sysTop) sysTop = sysIndex;
    if (sysIndex >= sysTop + 3) sysTop = sysIndex - 2;
    sysTop = clampLong(sysTop, 0, SYS_COUNT - 3);
    for (int row = 0; row < 3; ++row)
      oledTextRow(row, String(sysTop + row == sysIndex ? ">" : " ") + settingName(sysTop + row));
    oledStatusBar("SETTINGS");
  }
  else if (uiMode == UI_SYS_EDIT)
  {
    oledTextRow(0, sysIndex == SOUND ? "SOUND" : "RADAR HOLD TIME");
    oledTextRow(1, sysIndex == SOUND ? String(soundNames[settingEdit]) : String(settingEdit) + " seconds");
    oledTextRow(2, sysIndex == RADAR && GILKA_RADAR_PIN < 0 ? "Sensor not wired" : "Click/hold to save");
    oledStatusBar("EDIT");
  }
  else if (uiMode == UI_OTA)
  {
    if (otaEnabled)
    {
      const uint32_t used = (millis() - otaStarted) / 1000;
      oledTextRow(0, otaBusy ? "OTA UPLOADING" : "OTA READY " + String(used < 120 ? 120 - used : 0) + "s");
      oledTextRow(1, "Pass " + otaPassword);
      oledTextRow(2, WiFi.localIP().toString());
    }
    else
    {
      oledTextRow(0, "OTA UPDATE");
      oledTextRow(1, WiFi.status() == WL_CONNECTED ? "Click to arm 120s" : "Connect WiFi first");
      oledTextRow(2, "Hold: back");
    }
    oledStatusBar("OTA");
  }
  else if (uiMode == UI_BATTERY)
  {
    oledTextRow(0, "BATTERY 21700");
#if GILKA_BATTERY_PIN >= 0
    if (GILKA_BATTERY_RATIO > 0)
    {
      const float voltage = analogReadMilliVolts(GILKA_BATTERY_PIN) * GILKA_BATTERY_RATIO / 1000.0f;
      const int percent = clampLong(lroundf((voltage - 3.2f) * 100.0f), 0, 100);
      oledTextRow(1, String(voltage, 2) + " V");
      oledTextRow(2, "~" + String(percent) + "% (estimate)");
    }
    else
#endif
    {
      oledTextRow(1, "Not configured");
      oledTextRow(2, "Needs ADC divider");
    }
    oledStatusBar("BATTERY");
  }
  else switch (current)
  {
    case TIMER:
      oledTextRow(0, timer.stopwatch ? "STOPWATCH" : "TIMER " + String(minutes[preset]) + " min");
      oledTextRow(1, durationText(timer.shownSeconds()));
      oledTextRow(2, timer.alarm ? "DONE! Click: stop" : timer.running ? "Click: pause" : "Click: start");
      oledStatusBar(timer.alarm ? "ALARM" : timer.running ? "RUN" : "TIMER");
      break;
    case CLOCK:
    {
      time_t now = time(nullptr); struct tm local;
      if (now < 1700000000 || !localtime_r(&now, &local))
      {
        oledTextRow(0, "TIME & DATE"); oledTextRow(1, "Waiting for NTP");
        oledTextRow(2, WiFi.status() == WL_CONNECTED ? "Synchronizing..." : "WiFi offline");
      }
      else
      {
        char text[24];
        strftime(text, sizeof(text), clockFormat == 1 ? "%I:%M %p" : clockFormat == 2 ? "%H:%M:%S" : "%H:%M", &local);
        oledTextRow(0, text);
        strftime(text, sizeof(text), "%d.%m.%Y", &local); oledTextRow(1, text);
        oledTextRow(2, WiFi.status() == WL_CONNECTED ? "Cherkasy / Kyiv TZ" : "Clock / WiFi offline");
      }
      oledStatusBar("TIME"); break;
    }
    case WEATHER:
      oledTextRow(0, "CHERKASY");
      if (weatherFresh())
      {
        weatherIcon(data.weatherCode);
        oledTextRow(1, weatherPage == 0 ? String(data.temperature, 1) + " C  " + String(data.humidity, 0) + "%" : "Wind " + String(data.wind, 1) + " m/s");
        oledTextRow(2, weatherPage == 0 ? "Click: wind/pressure" : String(data.pressure, 0) + " hPa");
      }
      else
      {
        oledTextRow(1, !*config.weatherKey ? "Set API key on web" : data.weatherValid ? "STALE / NO LIVE DATA" : "No weather data");
        oledTextRow(2, clipText(String(data.weatherError), 21));
      }
      oledStatusBar("METEO"); break;
    case DOSIMETER:
      oledTextRow(0, "SIMULATION ONLY");
      oledTextRow(1, radiationRoentgen ? "~" + String(demoDoses[demoDoseIndex] * 100, 1) + " uR/h DEMO" : String(demoDoses[demoDoseIndex], 3) + " uSv/h DEMO");
      oledTextRow(2, "No radiation sensor");
      oledStatusBar("SIM"); break;
    case ALERT:
      oledTextRow(0, "CHERKASY OBLAST");
      if (alertLatched)
      {
        oledTextRow(1, alertFresh() ? "AIR RAID ALERT" : "ALERT / DATA STALE");
        oledTextRow(2, alertMuted ? "MUTED - stay alert" : "Click: mute siren");
      }
      else
      {
        oledTextRow(1, alertFresh() && data.alert == 'N' ? "No reported alert" : "STATUS UNKNOWN");
        oledTextRow(2, !*config.alertKey ? "Set API token on web" : clipText(String(data.alertError), 21));
      }
      oledStatusBar(alertLatched ? "ALERT" : "SURGE"); break;
    case SETTINGS: oledTextRow(0, "SYS: SETTINGS"); oledTextRow(1, "Click to open"); oledStatusBar("SYSTEM"); break;
  }
  oledEnd();
}
inline bool renderIfActive()
{
  if (!ready) return false;
  if (timer.alarm || selecting || uiMode == UI_HOME || uiMode == UI_SYS || uiMode == UI_SYS_EDIT || uiMode == UI_OTA || uiMode == UI_BATTERY)
  { render(); return true; }
  return false;
}
inline void commitSetting()
{
  if (sysIndex == SOUND)
  {
    sound = static_cast<Sound>(settingEdit);
    if (sound == MUTE || sound == CLICKS_ONLY) EncoderBuzzer::cancel();
  }
  else if (sysIndex == RADAR) radarHold = settingEdit;
  save(); uiMode = UI_SYS;
}
inline void beginSelection()
{
  if (selecting) return;
  if (uiMode == UI_EDIT) saveEdit();
  if (uiMode == UI_SYS_EDIT) commitSetting();
  stopOTA(); highlighted = current; selecting = true;
}
inline void heldStep(int direction)
{
  beginSelection(); highlighted = SectionLogic::wrap(highlighted + direction, COUNT);
  renderUI();
}
inline void releaseSelection()
{
  current = highlighted; selecting = false;
  uiMode = current == SETTINGS ? UI_SYS : UI_HOME;
  oledWake(false); renderUI();
}
inline void longPress()
{
  if (selecting) return;
  if (uiMode == UI_EDIT) { saveEdit(); uiMode = UI_MENU; }
  else if (uiMode == UI_SYS_EDIT) commitSetting();
  else if (uiMode == UI_MENU || uiMode == UI_WIFI_INFO || uiMode == UI_BATTERY || uiMode == UI_OTA)
  { stopOTA(); uiMode = UI_SYS; }
  else if (uiMode == UI_SYS) { save(); uiMode = UI_HOME; }
  else beginSelection();
  oledWake(false); renderUI();
}
inline bool rotate(int direction)
{
  if (uiMode == UI_SYS) sysIndex = SectionLogic::wrap(sysIndex + direction, SYS_COUNT);
  else if (uiMode == UI_SYS_EDIT)
    settingEdit = sysIndex == SOUND ? SectionLogic::wrap(settingEdit + direction, 4) : clampLong(settingEdit + direction * 5, 5, 120);
  else if (uiMode == UI_HOME && current == TIMER && !timer.running && !timer.alarm)
  { preset = SectionLogic::wrap(preset + direction, 5); timer.reset(minutes[preset] * 60000, preset == 0); }
  else if (uiMode == UI_HOME && current == DOSIMETER)
  { demoDoseIndex = SectionLogic::wrap(demoDoseIndex + direction, 5); nextGeiger = millis(); }
  else if (uiMode != UI_HOME && uiMode != UI_OTA && uiMode != UI_BATTERY) return false;
  renderUI(); return true;
}
inline bool click()
{
  if (timer.alarm)
  {
    timer.reset(minutes[preset] * 60000, preset == 0);
    EncoderBuzzer::setAlarm(false); renderUI(); return true;
  }
  if (uiMode == UI_SYS)
  {
    switch (sysIndex)
    {
      case SOUND: settingEdit = sound; uiMode = UI_SYS_EDIT; break;
      case RADAR: settingEdit = radarHold; uiMode = UI_SYS_EDIT; break;
      case DISPLAY_SETTINGS: menuIndex = menuTop = 0; uiMode = UI_MENU; break;
      case WIFI_CONFIG: startSetupAP(); apCloseAt = 0; uiMode = UI_WIFI_INFO; break;
      case OTA: uiMode = UI_OTA; armOTA(); break;
      case BATTERY: uiMode = UI_BATTERY; break;
      case WIFI_INFO: uiMode = UI_WIFI_INFO; break;
    }
  }
  else if (uiMode == UI_SYS_EDIT) commitSetting();
  else if (uiMode == UI_OTA) { if (otaEnabled) stopOTA(); else armOTA(); }
  else if (uiMode == UI_BATTERY) uiMode = UI_SYS;
  else if (uiMode == UI_HOME)
  {
    switch (current)
    {
      case TIMER: timer.click(millis()); break;
      case CLOCK: clockFormat = (clockFormat + 1) % 3; save(); break;
      case WEATHER: weatherPage = 1 - weatherPage; break;
      case DOSIMETER: radiationRoentgen = !radiationRoentgen; save(); break;
      case ALERT: alertMuted = true; EncoderBuzzer::setAlarm(false); break;
      case SETTINGS: uiMode = UI_SYS; break;
    }
  }
  else return false;
  renderUI(); return true;
}
inline void begin()
{
  sound = static_cast<Sound>(clampLong(preferences.getUChar("sound", LOW_SOUND), 0, 3));
  radarHold = clampLong(preferences.getUInt("radHold", 15), 5, 120);
  clockFormat = clampLong(preferences.getUChar("clockFmt", 0), 0, 2);
  radiationRoentgen = preferences.getBool("radUnit", false);
  timezone = preferences.getString("tz", "EET-2EEST,M3.5.0/3,M10.5.0/4");
  preferences.getString("owmKey", "").toCharArray(config.weatherKey, sizeof(config.weatherKey));
  preferences.getString("alertKey", "").toCharArray(config.alertKey, sizeof(config.alertKey));
  timer.reset(60000);
  configTzTime(timezone.c_str(), "pool.ntp.org", "time.google.com");
  SectionServices::begin(config);
#if GILKA_RADAR_PIN >= 0
  pinMode(GILKA_RADAR_PIN, INPUT);
#endif
#if GILKA_BATTERY_PIN >= 0
  analogSetPinAttenuation(GILKA_BATTERY_PIN, ADC_11db);
#endif
  // ESP32-S3 Zero onboard WS2812 on GPIO21 uses RGB wire order.
  neopixelWrite(21, 0, 0, 0);
  ready = true; uiMode = apRunning && WiFi.status() != WL_CONNECTED ? UI_WIFI_INFO : UI_HOME;
  oledWake(false); renderUI();
}
inline void tick()
{
  if (!ready) return;
  const uint32_t now = millis();
  timer.tick(now);
  if (timer.minuteEvent) { minuteBeeps = 2; nextMinuteBeep = now; }
  SectionServices::Snapshot next;
  if (SectionServices::receive(next) && next.revision == config.revision)
  {
    data = next;
    if (alertFresh())
    {
      if (data.alert == 'A' || data.alert == 'P')
      {
        if (!alertLatched) { alertMuted = false; oledWake(false); }
        alertLatched = true;
      }
      else if (data.alert == 'N') { alertLatched = false; alertMuted = false; }
    }
  }
  if (otaEnabled)
  {
    ArduinoOTA.handle();
    if (!otaBusy && (uint32_t(now - otaStarted) >= 120000 || WiFi.status() != WL_CONNECTED)) stopOTA();
  }
  const bool alarmActive = timer.alarm || (alertLatched && !alertMuted);
  const bool alarmSound = alarmsAllowed() && !otaBusy && (timer.alarm || (alertLatched && !alertMuted && (now / 500) % 2 == 0));
  EncoderBuzzer::setAlarm(alarmSound);
  const bool ledOn = (timer.alarm || alertLatched) && (now / 350) % 2 == 0;
  if (ledOn != ledWasOn)
  {
    // neopixelWrite emits GRB; this board's RGB LED needs swapped R/G arguments.
    neopixelWrite(21, 0, ledOn ? 40 : 0, 0); ledWasOn = ledOn;
  }
  if (timer.alarm && oledPowerState != OLED_ACTIVE) oledWake(false);
  if (minuteBeeps && int32_t(now - nextMinuteBeep) >= 0)
  {
    if (alarmsAllowed() && !alarmActive) EncoderBuzzer::beep(sound == MAX_SOUND ? 120 : 60);
    --minuteBeeps; nextMinuteBeep = now + 200;
  }
  if (current == DOSIMETER && uiMode == UI_HOME && !selecting && alarmsAllowed() && !alarmActive && int32_t(now - nextGeiger) >= 0)
  {
    EncoderBuzzer::beep(sound == MAX_SOUND ? 20 : 8);
    // Illustrative random counts increase with the selected demo dose, not a calibrated detector.
    const double u = (double(esp_random()) + 1.0) / (double(UINT32_MAX) + 2.0);
    nextGeiger = now + uint32_t(30 + (-log(u) * 1800 * 0.12 / demoDoses[demoDoseIndex]));
  }
#if GILKA_RADAR_PIN >= 0
  if (digitalRead(GILKA_RADAR_PIN))
  {
    radarDetected = true; lastRadar = now;
    if (oledPowerState != OLED_ACTIVE) oledWake(false);
  }
  if (radarDetected && uint32_t(now - lastRadar) <= radarHold * 1000) oledLastActivity = now;
#endif
  if (!selecting && uiMode != UI_EDIT && oledPowerState != OLED_OFF && uint32_t(now - refreshAt) >= 1000)
  { refreshAt = now; renderUI(); }
}
}

namespace Sections
{
inline String webSettings()
{
  String html = "<div class='section'><h2>Sections / Розділи</h2>";
  html += "<p>Cherkasy, Ukraine · Alerts: Cherkasy Oblast</p>";
  html += "<p>Hold the encoder and rotate, then release to open a section.</p>";
  html += "<form method='POST' action='/sections'>";
  html += "<label>Sound</label><select name='sound'>";
  for (int i = 0; i < 4; ++i)
    html += "<option value='" + String(i) + "'" + (i == sound ? " selected" : "") + ">" + soundNames[i] + "</option>";
  html += "</select><p class='small'>LOW/MAX control pulse duration, not true volume. MUTE and CLICKS ONLY suppress alarm sounds; visual alarms remain.</p>";
  html += "<label>Timezone (POSIX)</label><input name='tz' maxlength='80' required value='" + htmlEscape(timezone) + "'>";
  html += "<label>Radar hold, seconds (sensor not configured)</label><input type='number' name='radHold' min='5' max='120' required value='" + String(radarHold) + "'>";
  html += "<label>OpenWeatherMap API key</label><input type='password' name='owmKey' maxlength='128' autocomplete='new-password' placeholder='Leave blank to keep saved key'>";
  html += "<p class='small'>" + String(*config.weatherKey ? "Key saved" : "No key saved") + "</p>";
  html += "<label><input type='checkbox' name='clearWeather' value='1'> Remove weather key</label>";
  html += "<label>Alerts.in.ua API token</label><input type='password' name='alertKey' maxlength='128' autocomplete='new-password' placeholder='Leave blank to keep saved token'>";
  html += "<p class='small'>" + String(*config.alertKey ? "Token saved" : "No token saved") + "</p>";
  html += "<label><input type='checkbox' name='clearAlerts' value='1'> Remove alert token</label>";
  html += "<button class='blue' type='submit'>Save section settings</button></form>";
  html += "<p class='small'>Dosimeter is a clearly labelled simulation, not a measurement. Radiation clicks have no calibrated relationship to a real detector. Approximate uR/h conversion is for the gamma demo only.</p>";
  html += "<p class='small'>Air alerts require internet and a valid token. Unknown or stale data never means safe. Continue using official alerts.</p>";
  html += "<p><a href='https://openweathermap.org/api'>Weather API key</a> · <a href='https://devs.alerts.in.ua/'>Alert API token</a></p>";
  html += "<p class='small'>OTA: select SYS: SETTINGS → OTA UPDATE to open a 120-second window. The one-time password appears on the OLED. Radar and battery hardware are not configured.</p></div>";
  return html;
}
inline void saveWebSettings()
{
  uint32_t newSound, newHold;
  String newTZ = server.arg("tz");
  String newWeather = server.arg("owmKey"), newAlerts = server.arg("alertKey");
  if (!OLEDSettings::parseNumber(server.arg("sound").c_str(), 0, 3, newSound) ||
      !OLEDSettings::parseNumber(server.arg("radHold").c_str(), 5, 120, newHold) ||
      newTZ.isEmpty() || newTZ.length() > 80 || newWeather.length() > 128 || newAlerts.length() > 128)
  { server.send(400, "text/plain", "Invalid section settings"); return; }
  for (size_t i = 0; i < newTZ.length(); ++i)
    if (!isalnum(static_cast<unsigned char>(newTZ[i])) && !strchr("+-:,./<>_", newTZ[i]))
    { server.send(400, "text/plain", "Invalid POSIX timezone"); return; }
  for (const String *key : {&newWeather, &newAlerts})
    for (size_t i = 0; i < key->length(); ++i)
      if ((*key)[i] <= ' ' || (*key)[i] >= 127)
      { server.send(400, "text/plain", "API key must contain printable characters without spaces"); return; }
  sound = static_cast<Sound>(newSound); radarHold = newHold; timezone = newTZ;
  if (sound == MUTE || sound == CLICKS_ONLY) EncoderBuzzer::cancel();
  bool credentialsChanged = false;
  if (!newWeather.isEmpty() || server.hasArg("clearWeather"))
  {
    if (server.hasArg("clearWeather")) newWeather = "";
    newWeather.toCharArray(config.weatherKey, sizeof(config.weatherKey));
    preferences.putString("owmKey", newWeather); credentialsChanged = true;
  }
  if (!newAlerts.isEmpty() || server.hasArg("clearAlerts"))
  {
    if (server.hasArg("clearAlerts")) newAlerts = "";
    newAlerts.toCharArray(config.alertKey, sizeof(config.alertKey));
    preferences.putString("alertKey", newAlerts); credentialsChanged = true;
  }
  preferences.putString("tz", timezone); save();
  configTzTime(timezone.c_str(), "pool.ntp.org", "time.google.com");
  if (credentialsChanged)
  {
    ++config.revision; data = SectionServices::Snapshot{};
    // Retain any latched threat until a fresh no-alert response, or user mute.
    SectionServices::configure(config);
  }
  if (uiMode == UI_SYS_EDIT) uiMode = UI_SYS;
  oledWake(true);
  server.sendHeader("Location", "/", true); server.send(303, "text/plain", "");
}
}
