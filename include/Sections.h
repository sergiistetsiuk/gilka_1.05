#pragma once
#include <ArduinoOTA.h>
#include <esp32-hal-rgb-led.h>
#include <time.h>
#include <math.h>
#include "SectionLogic.h"
#include "SectionServices.h"


namespace Sections
{
enum Section { CLOCK, TIMER, WEATHER, DOSIMETER, ALERT, SETTINGS, COUNT };
enum Sound { MUTE, LOW_SOUND, MAX_SOUND, CLICKS_ONLY };
enum Setting { SOUND, DISPLAY_SETTINGS, WIFI_CONFIG, OTA, BATTERY, WIFI_INFO, SYS_COUNT };
static const char *titles[] = {"TIME & DATE", "TACTICAL: TIMER", "METEO: SECTOR", "DOSIMETER", "SURGE: ALERT", "SYS: SETTINGS"};
static const char *soundNames[] = {"MUTE", "LOW", "MAX", "CLICKS ONLY"};
static bool ready = false, selecting = false;
static int current = CLOCK, highlighted = CLOCK, sysIndex = 0, sysTop = 0;
static Sound sound = LOW_SOUND;
static int clockFormat = 0, weatherPage = 0;
static bool radiationRoentgen = false;
static bool radiationDetails = false;
static int settingEdit = 0;
static SectionLogic::Timer timer;
static uint32_t timerSeconds = 60;
static uint32_t refreshAt = 0;
static uint8_t minuteBeeps = 0;
static uint32_t nextMinuteBeep = 0;
static bool alertLatched = false, alertMuted = false;
static uint32_t alertSosStarted = 0;
static bool otaEnabled = false, otaBusy = false;
static uint32_t otaStarted = 0;
static String otaPassword;
static String timezone = "EET-2EEST,M3.5.0/3,M10.5.0/4";
static SectionServices::Config config;
static SectionServices::Snapshot data;
static bool ledWasOn = false;
constexpr uint8_t STATUS_LED_PIN = 8;
static bool externalLedWasOn = false;


inline bool alarmsAllowed() { return sound == LOW_SOUND || sound == MAX_SOUND; }
inline void feedback(bool press)
{
  if (sound == MUTE) return;
  EncoderBuzzer::beep(press ? (sound == MAX_SOUND ? 160 : 80) : (sound == MAX_SOUND ? 50 : 20));
}
inline void save()
{
  preferences.putUChar("sound", sound);
  preferences.putUChar("clockFmt", clockFormat);
  preferences.putBool("radUnit", radiationRoentgen);
}
inline bool alertFresh()
{ return WiFi.status() == WL_CONNECTED && !data.alertError[0] && SectionLogic::fresh(millis(), data.alertAt, data.alertValid, SectionLogic::ALERT_MAX_AGE_MS); }
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
    oledTextRow(0, "SOUND");
    oledTextRow(1, String(soundNames[settingEdit]));
    oledTextRow(2, "Click/hold to save");
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
    oledTextRow(0, "Li-ion " + String(BatteryMonitor::CAPACITY_MAH) + " mAh");
    if (BatteryMonitor::valid)
    {
      oledTextRow(1, String(BatteryMonitor::millivolts / 1000.0f, 2) + " V");
      oledTextRow(2, "~" + String(BatteryGauge::percent(BatteryMonitor::millivolts)) + "% (voltage est.)");
    }
    else
    {
      oledTextRow(1, "Voltage unavailable");
      oledTextRow(2, "Check ADC9 / divider");
    }
    oledStatusBar("BATTERY");
  }
  else switch (current)
  {
    case TIMER:
      oledTextRow(0, timer.stopwatch ? "STOPWATCH" : "TIMER " + durationText(timerSeconds));
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
    {
      const bool cached = WiFi.status() != WL_CONNECTED || data.radiationError[0] ||
        !SectionLogic::fresh(millis(), data.radiationAt, data.radiationValid, 1200000);
      const bool archive = data.radiation.old || time(nullptr) - data.radiation.captured > 86400;
      oledTextRow(0, cached && data.radiationValid ? "SAVE ECO BOT STALE" : archive && data.radiationValid ? "SAVE ECO BOT ARCHIVE" : "SAVE ECO BOT");
      if (data.radiationValid)
      {
        if (radiationDetails)
        {
          oledTextRow(1, String(data.radiation.distanceKm, 1) + " km from centre");
          oledTextRow(2, "ID " + String(data.radiation.id));
        }
        else
        {
          oledTextRow(1, radiationRoentgen ? "~" + String(data.radiation.value * 100, 1) + " uR/h (gamma)" : String(data.radiation.value, 3) + " uSv/h");
          oledTextRow(2, String(data.radiation.date));
        }
      }
      else
      {
        oledTextRow(1, "No radiation data");
        oledTextRow(2, clipText(String(data.radiationError), 21));
      }
      oledStatusBar(data.radiationValid && data.radiation.publicSource ? "PUBLIC" : "API"); break;
    }
    case ALERT:
      oledTextRow(0, "UKRAINE ALARM");
      if (alertLatched)
      {
        oledTextRow(1, alertFresh() ? data.alertLabel : "ALERT / DATA STALE");
        oledTextRow(2, alertMuted ? "MUTED - stay alert" : "Click: mute siren");
      }
      else
      {
        oledTextRow(1, alertFresh() ? data.alertLabel : "STATUS UNKNOWN");
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
    settingEdit = SectionLogic::wrap(settingEdit + direction, 4);
  else if (uiMode == UI_HOME && current == TIMER && !timer.running && !timer.alarm)
  { timerSeconds = SectionLogic::adjustTimerSeconds(timerSeconds, direction); timer.reset(timerSeconds * 1000, timerSeconds == 0); }
  else if (uiMode == UI_HOME && current == DOSIMETER)
  { radiationDetails = !radiationDetails; }
  else if (uiMode != UI_HOME && uiMode != UI_OTA && uiMode != UI_BATTERY) return false;
  renderUI(); return true;
}
inline bool click()
{
  if (timer.alarm)
  {
    timer.reset(timerSeconds * 1000, timerSeconds == 0);
    EncoderBuzzer::cancel(); renderUI(); return true;
  }
  if (uiMode == UI_SYS)
  {
    switch (sysIndex)
    {
      case SOUND: settingEdit = sound; uiMode = UI_SYS_EDIT; break;
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
      case DOSIMETER:
        if (data.radiationValid && !radiationDetails)
        { radiationRoentgen = !radiationRoentgen; save(); }
        else radiationDetails = !radiationDetails;
        break;
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
  clockFormat = clampLong(preferences.getUChar("clockFmt", 0), 0, 2);
  radiationRoentgen = preferences.getBool("radUnit", false);
  timezone = preferences.getString("tz", FirmwareDefaults::DEVICE_TIMEZONE);
  preferences.getString("owmKey", FirmwareDefaults::OPENWEATHERMAP_API_KEY).toCharArray(config.weatherKey, sizeof(config.weatherKey));
  preferences.getString("uaAlarmKey", FirmwareDefaults::UKRAINEALARM_API_KEY).toCharArray(config.alertKey, sizeof(config.alertKey));
  strlcpy(config.alertRegionName, FirmwareDefaults::UKRAINEALARM_REGION_NAME, sizeof(config.alertRegionName));
  preferences.getString("ecoKey", FirmwareDefaults::SAVEECOBOT_API_KEY).toCharArray(config.saveecobotKey, sizeof(config.saveecobotKey));
  strlcpy(config.saveecobotCity, FirmwareDefaults::SAVEECOBOT_STATION, sizeof(config.saveecobotCity));
  config.saveecobotRadiusKm = FirmwareDefaults::SAVEECOBOT_RADIUS_KM;
  strlcpy(config.saveecobotPublicUrl, FirmwareDefaults::SAVEECOBOT_PUBLIC_JSON_URL, sizeof(config.saveecobotPublicUrl));
  timer.reset(timerSeconds * 1000);
  configTzTime(timezone.c_str(), "pool.ntp.org", "time.google.com");
  SectionServices::begin(config);
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
        if (!alertLatched) { alertMuted = false; alertSosStarted = now; oledWake(false); }
        alertLatched = true;
      }
      else if (data.alert == 'N' || data.alert == 'I') { alertLatched = false; alertMuted = false; }
    }
  }
  if (otaEnabled)
  {
    ArduinoOTA.handle();
    if (!otaBusy && (uint32_t(now - otaStarted) >= 120000 || WiFi.status() != WL_CONNECTED)) stopOTA();
  }
  const bool alertPulse = alertLatched && SectionLogic::sosOn(now, alertSosStarted);
  const bool alarmActive = timer.alarm || (alertLatched && !alertMuted);
  const bool alarmSound = alarmsAllowed() && !otaBusy && (alertPulse && !alertMuted);
  EncoderBuzzer::setAlarm(alarmSound);
  // One-shot pulses stop independently of OLED/network work; leave silence between beeps.
  if (timer.takeAlarmBeep(now) && alarmsAllowed() && !otaBusy && !(alertLatched && !alertMuted))
    EncoderBuzzer::beep(sound == MAX_SOUND ? 300 : 200);
  const bool ledOn = alertLatched ? alertPulse : timer.alarm && (now / 350) % 2 == 0;
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
  const bool externalLedOn = alertLatched ? alertPulse : SectionLogic::indicatorOn(now, timer.alarm,
                                                       timer.running, timer.elapsedMs, false);
  if (externalLedOn != externalLedWasOn)
  {
    digitalWrite(STATUS_LED_PIN, externalLedOn ? HIGH : LOW);
    externalLedWasOn = externalLedOn;
  }
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
  html += "<label>OpenWeatherMap API key</label><input type='password' name='owmKey' maxlength='128' autocomplete='new-password' placeholder='Leave blank to keep saved key'>";
  html += "<p class='small'>" + String(*config.weatherKey ? "Key saved" : "No key saved") + "</p>";
  html += "<label><input type='checkbox' name='clearWeather' value='1'> Remove weather key</label>";
  html += "<label>Ukraine Alarm API token</label><input type='password' name='uaAlarmKey' maxlength='128' autocomplete='new-password' placeholder='Leave blank to keep saved token'>";
  html += "<p class='small'>" + String(*config.alertKey ? "Token saved" : "No token saved") + "</p>";
  html += "<label><input type='checkbox' name='clearAlerts' value='1'> Remove alert token</label>";
  html += "<label>SaveEcoBot API key (optional with public JSON)</label><input type='password' name='ecoKey' maxlength='128' autocomplete='new-password' placeholder='Leave blank to keep saved key'>";
  html += "<p class='small'>" + String(*config.saveecobotKey ? "Key configured" : "Public JSON fallback") + "</p>";
  html += "<label><input type='checkbox' name='clearSaveEcoBot' value='1'> Remove SaveEcoBot key</label>";
  html += "<button class='blue' type='submit'>Save section settings</button></form>";
  html += "<p class='small'>Data: <a href='https://www.saveecobot.com/'>SaveEcoBot</a>. With key: nearest station within the configured city radius. Without key: the public station JSON configured in .env, checked against the same radius. City coordinates: <a href='https://open-meteo.com/'>Open-Meteo</a> / <a href='https://www.geonames.org/'>GeoNames</a>. UTC measurement time shown; ARCHIVE marks old measurements, STALE marks cached data after a failed refresh. Remote gamma dose rate, not a local sensor.</p>";
  html += "<p class='small'>Air alerts require internet and a valid token. Unknown or stale data never means safe. Continue using official alerts.</p>";
  html += "<p><a href='https://openweathermap.org/api'>Weather API key</a> · <a href='https://api.ukrainealarm.com/'>Alert API token</a></p>";
  html += "<p class='small'>OTA: select SYS: SETTINGS → OTA UPDATE to open a 120-second window. The one-time password appears on the OLED. Battery: 1S Li-ion, 6000 mAh, GPIO9 with a 100k/100k divider.</p></div>";
  return html;
}
inline void saveWebSettings()
{
  uint32_t newSound;
  String newTZ = server.arg("tz");
  String newWeather = server.arg("owmKey"), newAlerts = server.arg("uaAlarmKey"), newSaveEcoBot = server.arg("ecoKey");
  if (!OLEDSettings::parseNumber(server.arg("sound").c_str(), 0, 3, newSound) ||
      newTZ.isEmpty() || newTZ.length() > 80 || newWeather.length() > 128 || newAlerts.length() > 128 || newSaveEcoBot.length() > 128)
  { server.send(400, "text/plain", "Invalid section settings"); return; }
  for (size_t i = 0; i < newTZ.length(); ++i)
    if (!isalnum(static_cast<unsigned char>(newTZ[i])) && !strchr("+-:,./<>_", newTZ[i]))
    { server.send(400, "text/plain", "Invalid POSIX timezone"); return; }
  for (const String *key : {&newWeather, &newAlerts, &newSaveEcoBot})
    for (size_t i = 0; i < key->length(); ++i)
      if ((*key)[i] <= ' ' || (*key)[i] >= 127)
      { server.send(400, "text/plain", "API key must contain printable characters without spaces"); return; }
  sound = static_cast<Sound>(newSound); timezone = newTZ;
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
    preferences.putString("uaAlarmKey", newAlerts); credentialsChanged = true;
  }
  if (!newSaveEcoBot.isEmpty() || server.hasArg("clearSaveEcoBot"))
  {
    if (server.hasArg("clearSaveEcoBot")) newSaveEcoBot = "";
    newSaveEcoBot.toCharArray(config.saveecobotKey, sizeof(config.saveecobotKey));
    preferences.putString("ecoKey", newSaveEcoBot); credentialsChanged = true;
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
