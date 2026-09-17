#include <Arduino.h>
#include "OLEDSettings.h"
#include "EncoderBuzzer.h"

#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Preferences.h>
#include <ESPmDNS.h>

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ======================================================
// DEVICE
// ======================================================

static const char *HOSTNAME = "gilka";

static const char *AP_SSID = "GILKA-SETUP";
static const char *AP_PASSWORD = "gilka1234";

// ======================================================
// WIFI TIMINGS
// ======================================================

static const uint32_t WIFI_BOOT_TIMEOUT_MS = 20000;
static const uint32_t WIFI_RETRY_INTERVAL_MS = 10000;
static const uint32_t WIFI_LOST_AP_DELAY_MS = 30000;
static const uint32_t AP_CLOSE_DELAY_MS = 10000;

// ======================================================
// OLED HARDWARE
// ======================================================

#define OLED_SDA 1
#define OLED_SCL 2

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

#define OLED_I2C_SPEED 100000

// Alternative COM mapping used by 128x64 panels. Sequential mapping (0x02)
// can split/interleave text rows and move them across the physical color boundary.
#define OLED_COM_PINS 0x12

// 180 градусів
#define OLED_ROTATION 2

// ======================================================
// OLED PHYSICAL ZONES
//
// Після rotation(2):
//
// Y 0..46  -> BLUE
// Y 47     -> separator
// Y 48..63 -> YELLOW
// ======================================================

#define BLUE_Y_START 0
#define BLUE_Y_END 46

#define SEPARATOR_Y 47

#define YELLOW_Y_START 48
#define YELLOW_Y_END 63

// Readability workaround: repeat each glyph row across both row parities.
// The panel currently displays only odd logical rows under COM 0x12.
#define STATUS_TEXT_Y 48
#define OLED_TEXT_ROW_HEIGHT 16
#define OLED_MENU_VISIBLE_ROWS 3

// ======================================================
// ENCODER
// ======================================================

#define ENCODER_A 5
#define ENCODER_B 4
#define ENCODER_BUTTON 6

static const uint32_t BUTTON_DEBOUNCE_MS = 25;
static const uint32_t BUTTON_LONG_MS = 1200;

// ======================================================
// OLED DEFAULT SETTINGS
// ======================================================

static const uint8_t OLED_DEFAULT_BRIGHTNESS = 120;
static const uint8_t OLED_DEFAULT_DIM_BRIGHTNESS = 20;

static const uint32_t OLED_DEFAULT_DIM_TIMEOUT = 30;
static const uint32_t OLED_DEFAULT_OFF_TIMEOUT = 300;

// ======================================================
// OBJECTS
// ======================================================

Adafruit_SSD1306 display(
    SCREEN_WIDTH,
    SCREEN_HEIGHT,
    &Wire,
    -1,
    OLED_I2C_SPEED,
    OLED_I2C_SPEED);

WebServer server(80);
DNSServer dnsServer;

Preferences preferences;

// ======================================================
// WIFI DATA
// ======================================================

String savedSSID;
String savedPassword;

// ======================================================
// OLED STATE
// ======================================================

bool oledReady = false;

uint8_t oledAddress = 0;

uint8_t oledBrightness =
    OLED_DEFAULT_BRIGHTNESS;

uint8_t oledDimBrightness =
    OLED_DEFAULT_DIM_BRIGHTNESS;

uint32_t oledDimTimeout =
    OLED_DEFAULT_DIM_TIMEOUT;

uint32_t oledOffTimeout =
    OLED_DEFAULT_OFF_TIMEOUT;

enum OLEDPowerState
{

  OLED_ACTIVE,
  OLED_DIMMED,
  OLED_OFF
};

OLEDPowerState oledPowerState =
    OLED_ACTIVE;

uint32_t oledLastActivity = 0;
uint32_t lastOLEDRefresh = 0;

// ======================================================
// SYSTEM STATE
// ======================================================

bool apRunning = false;
bool mdnsRunning = false;

bool wasConnected = false;

uint32_t wifiLostAt = 0;
uint32_t lastReconnectAttempt = 0;

uint32_t apCloseAt = 0;

// ======================================================
// UI
// ======================================================

enum UIMode
{

  UI_HOME,
  UI_MENU,
  UI_EDIT,
  UI_WIFI_INFO
};

UIMode uiMode =
    UI_HOME;

// ======================================================
// MENU
// ======================================================

enum MenuItem
{

  MENU_BRIGHTNESS,
  MENU_DIM_LEVEL,
  MENU_DIM_TIMEOUT,
  MENU_OFF_TIMEOUT,
  MENU_WIFI_INFO,
  MENU_EXIT,

  MENU_COUNT
};

int menuIndex = 0;
int menuTop = 0;

long editValue = 0;

// ======================================================
// ENCODER STATE
// ======================================================

uint8_t encoderPreviousAB = 0;

int8_t encoderAccumulator = 0;

const int8_t encoderTable[16] = {

    0, -1, 1, 0,
    1, 0, 0, -1,
    -1, 0, 0, 1,
    0, 1, -1, 0};

bool buttonRaw = HIGH;
bool buttonStable = HIGH;

uint32_t buttonChangedAt = 0;
uint32_t buttonDownAt = 0;

bool buttonLongHandled = false;
bool buttonWakeOnly = false;

// ======================================================
// FORWARD DECLARATIONS
// ======================================================

void renderUI();

void showBoot();
void showOnline();
void showSetup();
void showConnecting();
void showWiFiLost();

void showMenu();
void showEdit();
void showWiFiInfo();

void oledWake(bool redraw = true);

void pollEncoder();

void startSetupAP();
void stopSetupAP();

// ======================================================
// GENERAL HELPERS
// ======================================================

long clampLong(
    long value,
    long minimum,
    long maximum)
{

  if (value < minimum)
  {
    return minimum;
  }

  if (value > maximum)
  {
    return maximum;
  }

  return value;
}

String clipText(
    const String &text,
    size_t maxLength)
{

  if (
      text.length() <=
      maxLength)
  {

    return text;
  }

  return text.substring(
      0,
      maxLength);
}

String htmlEscape(
    const String &src)
{

  String result;

  result.reserve(
      src.length() + 16);

  for (
      size_t i = 0;
      i < src.length();
      i++)
  {

    char c =
        src[i];

    switch (c)
    {

    case '&':
      result += "&amp;";
      break;

    case '<':
      result += "&lt;";
      break;

    case '>':
      result += "&gt;";
      break;

    case '"':
      result += "&quot;";
      break;

    case '\'':
      result += "&#39;";
      break;

    default:
      result += c;
      break;
    }
  }

  return result;
}

// ======================================================
// OLED ADDRESS
// ======================================================

uint8_t detectOLEDAddress()
{

  const uint8_t addresses[] = {

      0x3C,
      0x3D};

  for (
      uint8_t address :
      addresses)
  {

    Wire.beginTransmission(
        address);

    if (
        Wire.endTransmission() ==
        0)
    {

      return address;
    }
  }

  return 0;
}

// ======================================================
// OLED CONTRAST
// ======================================================

void oledSetContrast(
    uint8_t value)
{

  if (!oledReady)
  {
    return;
  }

  display.ssd1306_command(
      SSD1306_SETCONTRAST);

  display.ssd1306_command(
      value);
}

// ======================================================
// SAVE OLED SETTINGS
// ======================================================

void saveOLEDSettings()
{

  preferences.putUChar(
      "oledB",
      oledBrightness);

  preferences.putUChar(
      "oledDimB",
      oledDimBrightness);

  preferences.putUInt(
      "oledDimS",
      oledDimTimeout);

  preferences.putUInt(
      "oledOffS",
      oledOffTimeout);

  Serial.println(
      "[OLED] settings saved");
}

// ======================================================
// OLED INIT
// ======================================================

void initOLED()
{

  Serial.println();

  Serial.println(
      "========================");

  Serial.println(
      "OLED init");

  Serial.println(
      "========================");

  Wire.begin(
      OLED_SDA,
      OLED_SCL);

  Wire.setClock(
      OLED_I2C_SPEED);

  delay(100);

  oledAddress =
      detectOLEDAddress();

  if (
      oledAddress == 0)
  {

    Serial.println(
        "OLED not found");

    oledReady = false;

    return;
  }

  Serial.print(
      "OLED address: 0x");

  Serial.println(
      oledAddress,
      HEX);

  if (
      !display.begin(
          SSD1306_SWITCHCAPVCC,
          oledAddress,
          true,
          false))
  {

    Serial.println(
        "OLED init FAILED");

    oledReady = false;

    return;
  }

  oledReady = true;

  // --------------------------------------------------
  // COM configuration
  //
  // Use the 128x64 COM mapping. The previous forced 0x02 is under
  // correction after the reported split rows in the display photo.
  // --------------------------------------------------

  display.ssd1306_command(
      SSD1306_SETCOMPINS);

  display.ssd1306_command(
      OLED_COM_PINS);

  // --------------------------------------------------
  // Rotate 180°
  // --------------------------------------------------

  display.setRotation(
      OLED_ROTATION);

  display.clearDisplay();

  display.setTextColor(
      SSD1306_WHITE);

  display.setTextSize(1, 2);

  display.setTextWrap(
      false);

  oledSetContrast(
      oledBrightness);

  display.display();

  oledPowerState =
      OLED_ACTIVE;

  oledLastActivity =
      millis();

  Serial.println(
      "OLED initialized");
}

// ======================================================
// OLED DRAW START
// ======================================================

void oledBegin()
{

  if (!oledReady)
  {
    return;
  }

  display.clearDisplay();

  display.setTextColor(
      SSD1306_WHITE);

  display.setTextSize(1, 2);

  display.setTextWrap(
      false);
}

// ======================================================
// OLED DRAW END
// ======================================================

void oledEnd()
{

  if (!oledReady)
  {
    return;
  }

  display.display();
}

// ======================================================
// YELLOW STATUS BAR
// ======================================================

void oledStatusBar(
    const String &status)
{

  if (!oledReady)
  {
    return;
  }

  // --------------------------------------------------
  // Border between blue / yellow
  // --------------------------------------------------

  display.drawFastHLine(
      0,
      SEPARATOR_Y,
      SCREEN_WIDTH,
      SSD1306_WHITE);

  // --------------------------------------------------
  // Left
  // --------------------------------------------------

  display.setCursor(
      1,
      STATUS_TEXT_Y);

  display.print(
      "GILKA");

  // --------------------------------------------------
  // Right
  // default font = 6 px per character
  // --------------------------------------------------

  int rightX =
      SCREEN_WIDTH -
      ((int)status.length() * 6) -
      1;

  if (rightX < 0)
  {
    rightX = 0;
  }

  display.setCursor(
      rightX,
      STATUS_TEXT_Y);

  display.print(
      status);
}

// ======================================================
// BOOT SCREEN
// ======================================================

// Default font ink is 7 pixels tall: at vertical scale 2, the last
// blue text row ends at Y=45, before the separator at Y=47.
void oledTextRow(int row, const String &text)
{
  display.setCursor(0, row * OLED_TEXT_ROW_HEIGHT);
  display.print(text);
}

void showBoot()
{
  if (!oledReady) return;
  oledBegin();
  oledTextRow(0, "ESP32-S3");
  oledTextRow(1, "Starting...");
  oledStatusBar("BOOT");
  oledEnd();
}

// ======================================================
// CONNECTING SCREEN
// ======================================================

void showConnecting()
{
  if (!oledReady) return;
  oledBegin();
  oledTextRow(0, "Connecting WiFi");
  oledTextRow(1, clipText(savedSSID, 20));
  oledTextRow(2, "Please wait...");
  oledStatusBar("CONNECT");
  oledEnd();
}

// ======================================================
// ONLINE SCREEN
// ======================================================

void showOnline()
{
  if (!oledReady || WiFi.status() != WL_CONNECTED) return;
  oledBegin();
  oledTextRow(0, "WiFi " + clipText(WiFi.SSID(), 15));
  oledTextRow(1, "IP " + WiFi.localIP().toString());
  oledTextRow(2, "gilka.local");
  // RSSI remains available on the WiFi info screen and web page.
  oledStatusBar("ONLINE");
  oledEnd();
}

// ======================================================
// SETUP SCREEN
// ======================================================

void showSetup()
{
  if (!oledReady) return;
  oledBegin();
  oledTextRow(0, AP_SSID);
  oledTextRow(1, String("Pass ") + AP_PASSWORD);
  oledTextRow(2, "IP 192.168.4.1");
  oledStatusBar("SETUP");
  oledEnd();
}

// ======================================================
// WIFI LOST SCREEN
// ======================================================

void showWiFiLost()
{
  if (!oledReady) return;
  oledBegin();
  oledTextRow(0, "WiFi connection lost");
  oledTextRow(1, "Reconnecting...");
  oledTextRow(2, "Setup after 30 sec");
  oledStatusBar("OFFLINE");
  oledEnd();
}

// ======================================================
// MENU TEXT
// ======================================================

String menuText(
    int index)
{

  switch (index)
  {

  case MENU_BRIGHTNESS:

    return "Bright " +
           String(
               oledBrightness);

  case MENU_DIM_LEVEL:

    return "Dim    " +
           String(
               oledDimBrightness);

  case MENU_DIM_TIMEOUT:

    if (
        oledDimTimeout == 0)
    {

      return "Dim time OFF";
    }

    return "Dim time " +
           String(
               oledDimTimeout) +
           "s";

  case MENU_OFF_TIMEOUT:

    if (
        oledOffTimeout == 0)
    {

      return "Off time OFF";
    }

    return "Off time " +
           String(
               oledOffTimeout) +
           "s";

  case MENU_WIFI_INFO:

    return "WiFi info";

  case MENU_EXIT:

    return "Exit";
  }

  return "";
}

// ======================================================
// KEEP SELECTED MENU ITEM VISIBLE
// ======================================================

void ensureMenuVisible()
{
  if (menuIndex < menuTop)
    menuTop = menuIndex;
  if (menuIndex >= menuTop + OLED_MENU_VISIBLE_ROWS)
    menuTop = menuIndex - OLED_MENU_VISIBLE_ROWS + 1;
  const int maxTop = MENU_COUNT > OLED_MENU_VISIBLE_ROWS
                         ? MENU_COUNT - OLED_MENU_VISIBLE_ROWS : 0;
  menuTop = clampLong(menuTop, 0, maxTop);
}

// ======================================================
// MENU SCREEN
// ======================================================

void showMenu()
{
  if (!oledReady) return;
  ensureMenuVisible();
  oledBegin();
  for (int row = 0; row < OLED_MENU_VISIBLE_ROWS; ++row)
  {
    const int item = menuTop + row;
    if (item >= MENU_COUNT) break;
    oledTextRow(row, String(item == menuIndex ? ">" : " ") + menuText(item));
  }
  oledStatusBar("MENU");
  oledEnd();
}

// ======================================================
// EDIT HELPERS
// ======================================================

String editTitle()
{

  switch (menuIndex)
  {

  case MENU_BRIGHTNESS:

    return "Brightness";

  case MENU_DIM_LEVEL:

    return "Dim brightness";

  case MENU_DIM_TIMEOUT:

    return "Dim timeout";

  case MENU_OFF_TIMEOUT:

    return "Off timeout";
  }

  return "Edit";
}

String editValueText()
{

  if (
      menuIndex ==
          MENU_DIM_TIMEOUT ||
      menuIndex ==
          MENU_OFF_TIMEOUT)
  {

    if (
        editValue == 0)
    {

      return "OFF";
    }

    return String(editValue) +
           " sec";
  }

  return String(
      editValue);
}

// ======================================================
// EDIT SCREEN
// ======================================================

void showEdit()
{
  if (!oledReady) return;
  oledBegin();
  oledTextRow(0, editTitle());
  oledTextRow(1, "Value " + editValueText());
  oledTextRow(2, "Press = save");
  oledStatusBar("EDIT");
  oledEnd();
}

// ======================================================
// WIFI INFO SCREEN
// ======================================================

void showWiFiInfo()
{
  if (!oledReady) return;
  oledBegin();
  if (WiFi.status() == WL_CONNECTED)
  {
    oledTextRow(0, clipText(WiFi.SSID(), 20));
    oledTextRow(1, WiFi.localIP().toString());
    oledTextRow(2, "RSSI " + String(WiFi.RSSI()) + " dBm");
  }
  else
  {
    oledTextRow(0, "WiFi OFFLINE");
    if (apRunning)
    {
      oledTextRow(1, AP_SSID);
      oledTextRow(2, "192.168.4.1");
    }
  }
  oledStatusBar("WIFI");
  oledEnd();
}

// ======================================================
// RENDER CURRENT UI
// ======================================================

void renderUI()
{

  if (!oledReady)
  {
    return;
  }

  switch (uiMode)
  {

  case UI_MENU:

    showMenu();

    return;

  case UI_EDIT:

    showEdit();

    return;

  case UI_WIFI_INFO:

    showWiFiInfo();

    return;

  case UI_HOME:

  default:

    break;
  }

  if (
      WiFi.status() ==
      WL_CONNECTED)
  {

    showOnline();
  }
  else if (
      apRunning)
  {

    showSetup();
  }
  else
  {

    showWiFiLost();
  }
}

// ======================================================
// OLED WAKE
// ======================================================

void oledWake(
    bool redraw)
{

  oledLastActivity =
      millis();

  if (!oledReady)
  {
    return;
  }

  if (
      oledPowerState ==
      OLED_OFF)
  {

    display.ssd1306_command(
        SSD1306_DISPLAYON);
  }

  oledSetContrast(
      oledBrightness);

  oledPowerState =
      OLED_ACTIVE;

  if (redraw)
  {

    renderUI();
  }
}

// ======================================================
// OLED DIM
// ======================================================

void oledDim()
{

  if (!oledReady)
  {
    return;
  }

  if (
      oledPowerState !=
      OLED_ACTIVE)
  {

    return;
  }

  oledSetContrast(
      oledDimBrightness);

  oledPowerState =
      OLED_DIMMED;

  Serial.println(
      "[OLED] DIM");
}

// ======================================================
// OLED OFF
// ======================================================

void oledOff()
{

  if (!oledReady)
  {
    return;
  }

  if (
      oledPowerState ==
      OLED_OFF)
  {

    return;
  }

  display.ssd1306_command(
      SSD1306_DISPLAYOFF);

  oledPowerState =
      OLED_OFF;

  Serial.println(
      "[OLED] OFF");
}

// ======================================================
// OLED POWER MANAGER
// ======================================================

void oledPowerManager()
{

  if (!oledReady)
  {
    return;
  }

  /*
     Поки працює Setup AP,
     не вимикаємо OLED.
  */

  if (apRunning)
  {

    if (
        oledPowerState !=
        OLED_ACTIVE)
    {

      oledWake(
          true);
    }

    return;
  }

  uint32_t inactive =
      millis() -
      oledLastActivity;

  // --------------------------------------------------
  // OFF
  // --------------------------------------------------

  if (
      oledOffTimeout > 0 &&
      inactive >=
          oledOffTimeout *
              1000UL)
  {

    oledOff();

    return;
  }

  // --------------------------------------------------
  // DIM
  // --------------------------------------------------

  if (
      oledDimTimeout > 0 &&
      inactive >=
          oledDimTimeout *
              1000UL)
  {

    oledDim();
  }
}

// ======================================================
// BEGIN EDIT
// ======================================================

void beginEdit()
{

  switch (menuIndex)
  {

  case MENU_BRIGHTNESS:

    editValue =
        oledBrightness;

    break;

  case MENU_DIM_LEVEL:

    editValue =
        oledDimBrightness;

    break;

  case MENU_DIM_TIMEOUT:

    editValue =
        oledDimTimeout;

    break;

  case MENU_OFF_TIMEOUT:

    editValue =
        oledOffTimeout;

    break;

  default:

    return;
  }

  uiMode =
      UI_EDIT;

  showEdit();
}

// ======================================================
// CHANGE EDIT VALUE
// ======================================================

void changeEditValue(
    int direction)
{

  switch (menuIndex)
  {

  case MENU_BRIGHTNESS:

    editValue +=
        direction * 5;

    editValue =
        clampLong(
            editValue,
            1,
            255);

    // live preview

    oledSetContrast(
        (uint8_t)
            editValue);

    break;

  case MENU_DIM_LEVEL:

    editValue +=
        direction * 5;

    editValue =
        clampLong(
            editValue,
            1,
            oledBrightness);

    // live preview

    oledSetContrast(
        (uint8_t)
            editValue);

    break;

  case MENU_DIM_TIMEOUT:

    editValue +=
        direction * 5;

    editValue =
        clampLong(
            editValue,
            0,
            OLEDSettings::MAX_TIMEOUT_SECONDS);

    break;

  case MENU_OFF_TIMEOUT:

    editValue +=
        direction * 30;

    editValue =
        clampLong(
            editValue,
            0,
            86400);

    break;
  }

  showEdit();
}

// ======================================================
// SAVE EDIT
// ======================================================

void saveEdit()
{

  switch (menuIndex)
  {

  case MENU_BRIGHTNESS:

    oledBrightness =
        (uint8_t)
            editValue;

    if (
        oledDimBrightness >
        oledBrightness)
    {

      oledDimBrightness =
          oledBrightness;
    }

    break;

  case MENU_DIM_LEVEL:

    oledDimBrightness =
        (uint8_t)
            editValue;

    break;

  case MENU_DIM_TIMEOUT:

    oledDimTimeout =
        (uint32_t)
            editValue;

    if (
        oledOffTimeout > 0 &&
        oledDimTimeout > 0 &&
        oledOffTimeout <
            oledDimTimeout)
    {

      oledOffTimeout =
          oledDimTimeout;
    }

    break;

  case MENU_OFF_TIMEOUT:

    oledOffTimeout =
        (uint32_t)
            editValue;

    if (
        oledOffTimeout > 0 &&
        oledDimTimeout > 0 &&
        oledOffTimeout <
            oledDimTimeout)
    {

      oledOffTimeout =
          oledDimTimeout;
    }

    break;
  }

  saveOLEDSettings();

  oledSetContrast(
      oledBrightness);

  uiMode =
      UI_MENU;

  oledWake(
      false);

  showMenu();
}

// ======================================================
// ENCODER ROTATION
// ======================================================

void handleEncoderStep(
    int direction)
{

  EncoderBuzzer::beep(EncoderBuzzer::ROTATE_MS);

  bool wasOff =
      oledPowerState ==
      OLED_OFF;

  oledWake(
      false);

  /*
     Перший рух після OFF
     лише будить дисплей.
  */

  if (wasOff)
  {

    renderUI();

    return;
  }

  // --------------------------------------------------
  // HOME -> MENU
  // --------------------------------------------------

  if (
      uiMode ==
      UI_HOME)
  {

    uiMode =
        UI_MENU;

    menuIndex =
        0;

    menuTop =
        0;

    showMenu();

    return;
  }

  // --------------------------------------------------
  // MENU
  // --------------------------------------------------

  if (
      uiMode ==
      UI_MENU)
  {

    menuIndex +=
        direction;

    if (
        menuIndex < 0)
    {

      menuIndex =
          MENU_COUNT - 1;
    }

    if (
        menuIndex >=
        MENU_COUNT)
    {

      menuIndex =
          0;
    }

    showMenu();

    return;
  }

  // --------------------------------------------------
  // EDIT
  // --------------------------------------------------

  if (
      uiMode ==
      UI_EDIT)
  {

    changeEditValue(
        direction);

    return;
  }

  // --------------------------------------------------
  // WIFI INFO
  // --------------------------------------------------

  if (
      uiMode ==
      UI_WIFI_INFO)
  {

    uiMode =
        UI_MENU;

    showMenu();
  }
}

// ======================================================
// SHORT BUTTON PRESS
// ======================================================

void handleShortPress()
{

  oledWake(
      false);

  switch (uiMode)
  {

    // ------------------------------------------------
    // HOME
    // ------------------------------------------------

  case UI_HOME:

    uiMode =
        UI_MENU;

    menuIndex =
        0;

    menuTop =
        0;

    showMenu();

    break;

    // ------------------------------------------------
    // MENU
    // ------------------------------------------------

  case UI_MENU:

    switch (
        menuIndex)
    {

    case MENU_BRIGHTNESS:
    case MENU_DIM_LEVEL:
    case MENU_DIM_TIMEOUT:
    case MENU_OFF_TIMEOUT:

      beginEdit();

      break;

    case MENU_WIFI_INFO:

      uiMode =
          UI_WIFI_INFO;

      showWiFiInfo();

      break;

    case MENU_EXIT:

      uiMode =
          UI_HOME;

      renderUI();

      break;
    }

    break;

    // ------------------------------------------------
    // EDIT
    // ------------------------------------------------

  case UI_EDIT:

    saveEdit();

    break;

    // ------------------------------------------------
    // WIFI INFO
    // ------------------------------------------------

  case UI_WIFI_INFO:

    uiMode =
        UI_MENU;

    showMenu();

    break;
  }
}

// ======================================================
// LONG BUTTON PRESS
// ======================================================

void handleLongPress()
{

  Serial.println(
      "[ENC] long press -> HOME");

  /*
     Long press =
     повернутися на HOME.

     Якщо редагування не було
     збережене — повертаємо
     реальну яскравість.
  */

  oledSetContrast(
      oledBrightness);

  uiMode =
      UI_HOME;

  oledWake(
      false);

  renderUI();
}

// ======================================================
// ENCODER POLL
// ======================================================

void pollEncoder()
{

  // ==================================================
  // ROTATION
  // ==================================================

  uint8_t currentAB =
      (digitalRead(
           ENCODER_A)
       << 1) |
      digitalRead(
          ENCODER_B);

  uint8_t index =
      (encoderPreviousAB << 2) |
      currentAB;

  encoderAccumulator +=
      encoderTable[index];

  encoderPreviousAB =
      currentAB;

  if (
      encoderAccumulator >=
      4)
  {

    encoderAccumulator =
        0;

    handleEncoderStep(
        +1);
  }

  if (
      encoderAccumulator <=
      -4)
  {

    encoderAccumulator =
        0;

    handleEncoderStep(
        -1);
  }

  // ==================================================
  // BUTTON RAW STATE
  // ==================================================

  bool raw =
      digitalRead(
          ENCODER_BUTTON);

  if (
      raw !=
      buttonRaw)
  {

    buttonRaw =
        raw;

    buttonChangedAt =
        millis();
  }

  // ==================================================
  // DEBOUNCE
  // ==================================================

  if (
      millis() -
          buttonChangedAt >=
      BUTTON_DEBOUNCE_MS)
  {

    if (
        buttonStable !=
        buttonRaw)
    {

      buttonStable =
          buttonRaw;

      // --------------------------------------------
      // PRESS
      // --------------------------------------------

      if (
          buttonStable ==
          LOW)
      {

        // One beep per debounced press, including a press that only wakes OLED.
        EncoderBuzzer::beep(EncoderBuzzer::PRESS_MS);

        buttonDownAt =
            millis();

        buttonLongHandled =
            false;

        buttonWakeOnly =
            (oledPowerState ==
             OLED_OFF);

        oledWake(
            false);

        renderUI();
      }

      // --------------------------------------------
      // RELEASE
      // --------------------------------------------

      else
      {

        if (
            !buttonLongHandled &&
            !buttonWakeOnly)
        {

          handleShortPress();
        }

        buttonWakeOnly =
            false;
      }
    }
  }

  // ==================================================
  // LONG PRESS
  // ==================================================

  if (
      buttonStable ==
          LOW &&
      !buttonLongHandled &&
      !buttonWakeOnly &&
      millis() -
              buttonDownAt >=
          BUTTON_LONG_MS)
  {

    buttonLongHandled =
        true;

    handleLongPress();
  }
}

// ======================================================
// ENCODER INIT
// ======================================================

void initEncoder()
{

  pinMode(
      ENCODER_A,
      INPUT_PULLUP);

  pinMode(
      ENCODER_B,
      INPUT_PULLUP);

  pinMode(
      ENCODER_BUTTON,
      INPUT_PULLUP);

  encoderPreviousAB =
      (digitalRead(
           ENCODER_A)
       << 1) |
      digitalRead(
          ENCODER_B);

  buttonRaw =
      digitalRead(
          ENCODER_BUTTON);

  buttonStable =
      buttonRaw;

  Serial.println();

  Serial.println(
      "========================");

  Serial.println(
      "Encoder init");

  Serial.println(
      "========================");

  Serial.println(
      "A      = GPIO5");

  Serial.println(
      "B      = GPIO4");

  Serial.println(
      "Button = GPIO6");
}

// ======================================================
// WIFI EVENT
// ======================================================

void WiFiEvent(
    WiFiEvent_t event,
    WiFiEventInfo_t info)
{

  switch (event)
  {

  case ARDUINO_EVENT_WIFI_STA_START:

    Serial.println(
        "[WiFi] STA started");

    break;

  case ARDUINO_EVENT_WIFI_STA_CONNECTED:

    Serial.println(
        "[WiFi] Associated");

    break;

  case ARDUINO_EVENT_WIFI_STA_GOT_IP:

    Serial.print(
        "[WiFi] IP: ");

    Serial.println(
        WiFi.localIP());

    break;

  case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:

    Serial.print(
        "[WiFi] DISCONNECTED reason: ");

    Serial.println(
        info.wifi_sta_disconnected.reason);

    break;

  default:

    break;
  }
}

// ======================================================
// MDNS
// ======================================================

void startMDNS()
{

  if (mdnsRunning)
  {
    return;
  }

  if (
      WiFi.status() !=
      WL_CONNECTED)
  {

    return;
  }

  if (
      MDNS.begin(
          HOSTNAME))
  {

    MDNS.addService(
        "http",
        "tcp",
        80);

    mdnsRunning =
        true;

    Serial.println(
        "mDNS: http://gilka.local");
  }
}

void stopMDNS()
{

  if (!mdnsRunning)
  {
    return;
  }

  MDNS.end();

  mdnsRunning =
      false;

  Serial.println(
      "mDNS stopped");
}

// ======================================================
// WIFI STACK
// ======================================================

void initWiFiStack()
{

  Serial.println();

  Serial.println(
      "========================");

  Serial.println(
      "WiFi init");

  Serial.println(
      "========================");

  WiFi.onEvent(
      WiFiEvent);

  /*
     Створюємо WiFi / lwIP
     ДО server.begin().
  */

  WiFi.mode(
      WIFI_STA);

  delay(150);

  /*
     Hostname до WiFi.begin().
  */

  WiFi.setHostname(
      HOSTNAME);

  WiFi.setAutoReconnect(
      true);

  WiFi.setSleep(
      false);

  delay(100);

  Serial.println(
      "WiFi stack initialized");
}

// ======================================================
// CONNECT SAVED WIFI
// ======================================================

bool connectSavedWiFi()
{

  if (
      savedSSID.length() ==
      0)
  {

    return false;
  }

  uiMode =
      UI_HOME;

  oledWake(
      false);

  showConnecting();

  Serial.println();

  Serial.println(
      "========================");

  Serial.println(
      "Connecting WiFi");

  Serial.println(
      "========================");

  Serial.print(
      "SSID: ");

  Serial.println(
      savedSSID);

  WiFi.disconnect(
      false,
      false);

  delay(100);

  WiFi.begin(
      savedSSID.c_str(),
      savedPassword.c_str());

  uint32_t started =
      millis();

  while (
      WiFi.status() !=
          WL_CONNECTED &&
      millis() -
              started <
          WIFI_BOOT_TIMEOUT_MS)
  {

    pollEncoder();

    delay(10);
  }

  if (
      WiFi.status() ==
      WL_CONNECTED)
  {

    Serial.println(
        "WiFi CONNECTED");

    Serial.print(
        "IP: ");

    Serial.println(
        WiFi.localIP());

    wifiLostAt =
        0;

    lastReconnectAttempt =
        0;

    wasConnected =
        true;

    startMDNS();

    uiMode =
        UI_HOME;

    oledWake(
        false);

    showOnline();

    return true;
  }

  Serial.println(
      "WiFi connection FAILED");

  wifiLostAt =
      millis();

  return false;
}

// ======================================================
// START SETUP AP
// ======================================================

void startSetupAP()
{

  if (apRunning)
  {
    return;
  }

  Serial.println();

  Serial.println(
      "========================");

  Serial.println(
      "Starting GILKA-SETUP");

  Serial.println(
      "========================");

  WiFi.mode(
      WIFI_AP_STA);

  delay(150);

  if (
      !WiFi.softAP(
          AP_SSID,
          AP_PASSWORD))
  {

    Serial.println(
        "AP start FAILED");

    return;
  }

  apRunning =
      true;

  dnsServer.start(
      53,
      "*",
      WiFi.softAPIP());

  Serial.print(
      "AP IP: ");

  Serial.println(
      WiFi.softAPIP());

  uiMode =
      UI_HOME;

  oledWake(
      false);

  showSetup();
}

// ======================================================
// STOP SETUP AP
// ======================================================

void stopSetupAP()
{

  if (!apRunning)
  {
    return;
  }

  Serial.println(
      "Stopping GILKA-SETUP");

  dnsServer.stop();

  /*
     AP_STA -> STA

     Це вимикає AP,
     але залишає STA.
  */

  WiFi.mode(
      WIFI_STA);

  delay(100);

  apRunning =
      false;

  apCloseAt =
      0;

  uiMode =
      UI_HOME;

  oledWake(
      false);

  renderUI();
}

// ======================================================
// WEB PAGE
// ======================================================

String makePage()
{

  String html;

  html.reserve(
      8500);

  html += R"HTML(
<!DOCTYPE html>
<html lang="uk">

<head>

<meta charset="UTF-8">

<meta name="viewport"
content="width=device-width,initial-scale=1">

<title>GILKA</title>

<style>

* {
    box-sizing:border-box;
}

body {
    margin:0;
    padding:20px;
    background:#111827;
    color:#f9fafb;
    font-family:Arial,sans-serif;
}

.card {
    max-width:520px;
    margin:20px auto;
    padding:24px;
    background:#1f2937;
    border-radius:16px;
}

.section {
    margin-top:25px;
    padding-top:20px;
    border-top:1px solid #374151;
}

.status {
    padding:16px;
    background:#374151;
    border-radius:10px;
    line-height:1.7;
}

.ok {
    color:#4ade80;
    font-weight:bold;
}

.bad {
    color:#f87171;
    font-weight:bold;
}

label {
    display:block;
    margin-top:14px;
    margin-bottom:5px;
}

input {
    width:100%;
    padding:11px;
    border-radius:8px;
    border:1px solid #4b5563;
    background:#111827;
    color:white;
    font-size:16px;
}

button {
    width:100%;
    margin-top:18px;
    padding:13px;
    border:0;
    border-radius:8px;
    font-size:16px;
}

.blue {
    background:#2563eb;
    color:white;
}

.red {
    background:#dc2626;
    color:white;
}

.gray {
    background:#4b5563;
    color:white;
}

.small {
    color:#9ca3af;
    font-size:13px;
    line-height:1.5;
}

</style>

</head>

<body>

<div class="card">

<h1>GILKA</h1>

<div class="status">
)HTML";

  if (
      WiFi.status() ==
      WL_CONNECTED)
  {

    html +=
        "<div class='ok'>● ONLINE</div>";

    html +=
        "<br>SSID: <b>" +
        htmlEscape(
            WiFi.SSID()) +
        "</b>";

    html +=
        "<br>IP: <b>" +
        WiFi.localIP()
            .toString() +
        "</b>";

    html +=
        "<br>RSSI: <b>" +
        String(
            WiFi.RSSI()) +
        " dBm</b>";

    html +=
        "<br>Hostname: <b>gilka.local</b>";
  }
  else
  {

    html +=
        "<div class='bad'>● OFFLINE</div>";

    if (apRunning)
    {

      html +=
          "<br>AP: <b>" +
          String(AP_SSID) +
          "</b>";

      html +=
          "<br>IP: <b>" +
          WiFi.softAPIP()
              .toString() +
          "</b>";
    }
  }

  html += R"HTML(
</div>


<div class="section">

<h2>Wi-Fi</h2>

<form method="POST" action="/save">

<label>SSID</label>

<input
type="text"
name="ssid"
required
value=")HTML";

  html +=
      htmlEscape(
          savedSSID);

  html += R"HTML(">


<label>Password</label>

<input
type="password"
name="password"
placeholder="залиш порожнім для старого пароля">


<button
class="blue"
type="submit">
Зберегти Wi-Fi
</button>

</form>


<form
method="POST"
action="/forget">

<button
class="red"
type="submit">
Видалити Wi-Fi
</button>

</form>

</div>


<div class="section">

<h2>OLED</h2>

<form
method="POST"
action="/display">


<label>Brightness 1-255</label>

<input
type="number"
name="brightness"
min="1"
max="255"
value=")HTML";

  html +=
      String(
          oledBrightness);

  html += R"HTML(">


<label>DIM brightness</label>

<input
type="number"
name="dimBrightness"
min="1"
max="255"
value=")HTML";

  html +=
      String(
          oledDimBrightness);

  html += R"HTML(">


<label>DIM через, секунд</label>

<input
type="number"
name="dimTimeout"
min="0"
max="86400"
value=")HTML";

  html +=
      String(
          oledDimTimeout);

  html += R"HTML(">


<label>OLED OFF через, секунд</label>

<input
type="number"
name="offTimeout"
min="0"
max="86400"
value=")HTML";

  html +=
      String(
          oledOffTimeout);

  html += R"HTML(">


<p class="small">
0 вимикає відповідний таймер.
</p>


<button
class="gray"
type="submit">
Зберегти OLED
</button>

</form>

</div>


<div class="section">

<h2>Encoder</h2>

<p class="small">

Rotate — menu / value<br>
Press — select / save<br>
Long press — home

</p>

</div>


</div>

</body>
</html>
)HTML";

  return html;
}

// ======================================================
// HTTP ROOT
// ======================================================

void handleRoot()
{

  oledWake(
      true);

  server.sendHeader(
      "Cache-Control",
      "no-store");

  server.send(
      200,
      "text/html; charset=utf-8",
      makePage());
}

// ======================================================
// SAVE WIFI
// ======================================================

void handleSaveWiFi()
{

  if (
      !server.hasArg(
          "ssid"))
  {

    server.send(
        400,
        "text/plain",
        "SSID required");

    return;
  }

  String newSSID =
      server.arg(
          "ssid");

  String newPassword =
      server.arg(
          "password");

  newSSID.trim();

  if (
      newSSID.length() ==
      0)
  {

    server.send(
        400,
        "text/plain",
        "SSID empty");

    return;
  }

  if (
      newSSID ==
          savedSSID &&
      newPassword.length() ==
          0)
  {

    newPassword =
        savedPassword;
  }

  savedSSID =
      newSSID;

  savedPassword =
      newPassword;

  preferences.putString(
      "ssid",
      savedSSID);

  preferences.putString(
      "pass",
      savedPassword);

  server.send(
      200,
      "text/html; charset=utf-8",
      "<h2>Saved. Restarting...</h2>");

  delay(1000);

  ESP.restart();
}

// ======================================================
// FORGET WIFI
// ======================================================

void handleForgetWiFi()
{

  preferences.remove(
      "ssid");

  preferences.remove(
      "pass");

  server.send(
      200,
      "text/html; charset=utf-8",
      "<h2>WiFi removed. Restarting...</h2>");

  delay(1000);

  ESP.restart();
}

// ======================================================
// OLED WEB SETTINGS
// ======================================================

void handleDisplaySettings()
{

  uint32_t brightness, dimBrightness, dimTimeout, offTimeout;

  if (!OLEDSettings::parseNumber(server.arg("brightness").c_str(), 1, 255, brightness) ||
      !OLEDSettings::parseNumber(server.arg("dimBrightness").c_str(), 1, 255, dimBrightness) ||
      !OLEDSettings::parseNumber(server.arg("dimTimeout").c_str(), 0,
                                 OLEDSettings::MAX_TIMEOUT_SECONDS, dimTimeout) ||
      !OLEDSettings::parseNumber(server.arg("offTimeout").c_str(), 0,
                                 OLEDSettings::MAX_TIMEOUT_SECONDS, offTimeout))
  {
    server.send(400, "text/plain; charset=utf-8",
                "Invalid OLED settings: brightness must be 1-255 and timeouts 0-86400 seconds.");
    return;
  }

  oledBrightness = static_cast<uint8_t>(brightness);
  oledDimBrightness = static_cast<uint8_t>(dimBrightness);
  oledDimTimeout = dimTimeout;
  oledOffTimeout = offTimeout;
  OLEDSettings::normalize(oledBrightness, oledDimBrightness,
                          oledDimTimeout, oledOffTimeout);

  saveOLEDSettings();

  uiMode = UI_HOME;

  oledWake(
      true);

  server.sendHeader(
      "Location",
      "/",
      true);

  server.send(
      303,
      "text/plain",
      "");
}

// ======================================================
// CAPTIVE PORTAL
// ======================================================

void redirectPortal()
{

  if (!apRunning)
  {

    server.send(
        404,
        "text/plain",
        "Not found");

    return;
  }

  String url =
      "http://" +
      WiFi.softAPIP()
          .toString() +
      "/";

  server.sendHeader(
      "Location",
      url,
      true);

  server.send(
      302,
      "text/plain",
      "");
}

// ======================================================
// WEB SERVER INIT
// ======================================================

void setupWebServer()
{

  server.on(
      "/",
      HTTP_GET,
      handleRoot);

  server.on(
      "/save",
      HTTP_POST,
      handleSaveWiFi);

  server.on(
      "/forget",
      HTTP_POST,
      handleForgetWiFi);

  server.on(
      "/display",
      HTTP_POST,
      handleDisplaySettings);

  // Android

  server.on(
      "/generate_204",
      HTTP_GET,
      redirectPortal);

  server.on(
      "/gen_204",
      HTTP_GET,
      redirectPortal);

  // Apple

  server.on(
      "/hotspot-detect.html",
      HTTP_GET,
      redirectPortal);

  server.on(
      "/library/test/success.html",
      HTTP_GET,
      redirectPortal);

  // Windows

  server.on(
      "/connecttest.txt",
      HTTP_GET,
      redirectPortal);

  server.on(
      "/ncsi.txt",
      HTTP_GET,
      redirectPortal);

  server.on(
      "/fwlink",
      HTTP_GET,
      redirectPortal);

  server.on(
      "/favicon.ico",
      HTTP_GET,
      []()
      {
        server.send(
            204,
            "text/plain",
            "");
      });

  server.onNotFound(
      []()
      {
        if (apRunning)
        {

          redirectPortal();
        }
        else
        {

          server.send(
              404,
              "text/plain",
              "404");
        }
      });

  server.begin();

  Serial.println(
      "HTTP server started");
}

// ======================================================
// WIFI CONNECTED HANDLER
// ======================================================

void handleWiFiConnected()
{

  wifiLostAt =
      0;

  lastReconnectAttempt =
      0;

  startMDNS();

  oledWake(
      false);

  if (
      uiMode ==
      UI_HOME)
  {

    showOnline();
  }

  if (apRunning)
  {

    apCloseAt =
        millis() +
        AP_CLOSE_DELAY_MS;
  }
}

// ======================================================
// SETUP
// ======================================================

void setup()
{

  Serial.begin(
      115200);

  EncoderBuzzer::begin();

  delay(1200);

  Serial.println();

  Serial.println(
      "========================");

  Serial.println(
      "GILKA boot");

  Serial.println(
      "========================");

  // ==================================================
  // NVS
  // ==================================================

  /*
     Залишаємо namespace "wifi",
     щоб не втратити вже збережений SSID.
  */

  preferences.begin(
      "wifi",
      false);

  savedSSID =
      preferences.getString(
          "ssid",
          "");

  savedPassword =
      preferences.getString(
          "pass",
          "");

  oledBrightness =
      preferences.getUChar(
          "oledB",
          OLED_DEFAULT_BRIGHTNESS);

  oledDimBrightness =
      preferences.getUChar(
          "oledDimB",
          OLED_DEFAULT_DIM_BRIGHTNESS);

  oledDimTimeout =
      preferences.getUInt(
          "oledDimS",
          OLED_DEFAULT_DIM_TIMEOUT);

  oledOffTimeout =
      preferences.getUInt(
          "oledOffS",
          OLED_DEFAULT_OFF_TIMEOUT);

  if (
      oledBrightness == 0)
  {

    oledBrightness =
        OLED_DEFAULT_BRIGHTNESS;
  }

  if (
      oledDimBrightness == 0)
  {

    oledDimBrightness =
        OLED_DEFAULT_DIM_BRIGHTNESS;
  }

  OLEDSettings::normalize(oledBrightness, oledDimBrightness,
                          oledDimTimeout, oledOffTimeout);

  // ==================================================
  // OLED
  // ==================================================

  initOLED();

  showBoot();

  // ==================================================
  // ENCODER
  // ==================================================

  initEncoder();

  // ==================================================
  // WIFI STACK
  // ==================================================

  initWiFiStack();

  // ==================================================
  // WEB SERVER
  // ==================================================

  setupWebServer();

  // ==================================================
  // WIFI CONNECTION
  // ==================================================

  if (
      savedSSID.length() > 0)
  {

    bool connected =
        connectSavedWiFi();

    if (!connected)
    {

      startSetupAP();
    }
  }
  else
  {

    startSetupAP();
  }

  wasConnected =
      WiFi.status() ==
      WL_CONNECTED;

  oledLastActivity =
      millis();
}

// ======================================================
// LOOP
// ======================================================

void loop()
{

  // ==================================================
  // ENCODER
  // ==================================================

  pollEncoder();

  // ==================================================
  // CAPTIVE DNS
  // ==================================================

  if (apRunning)
  {

    dnsServer.processNextRequest();
  }

  // ==================================================
  // HTTP
  // ==================================================

  server.handleClient();

  // ==================================================
  // WIFI STATE
  // ==================================================

  bool connected =
      WiFi.status() ==
      WL_CONNECTED;

  // --------------------------------------------------
  // JUST CONNECTED
  // --------------------------------------------------

  if (
      connected &&
      !wasConnected)
  {

    Serial.println(
        "WiFi ONLINE");

    handleWiFiConnected();
  }

  // --------------------------------------------------
  // JUST LOST CONNECTION
  // --------------------------------------------------

  if (
      !connected &&
      wasConnected)
  {

    Serial.println(
        "WiFi LOST");

    stopMDNS();

    wifiLostAt =
        millis();

    lastReconnectAttempt =
        0;

    if (
        uiMode ==
        UI_HOME)
    {

      oledWake(
          false);

      showWiFiLost();
    }
  }

  // ==================================================
  // WIFI RECONNECT
  // ==================================================

  if (
      !connected &&
      savedSSID.length() > 0)
  {

    if (
        wifiLostAt == 0)
    {

      wifiLostAt =
          millis();
    }

    if (
        millis() -
            lastReconnectAttempt >=
        WIFI_RETRY_INTERVAL_MS)
    {

      lastReconnectAttempt =
          millis();

      Serial.println(
          "Trying WiFi reconnect...");

      WiFi.reconnect();
    }

    /*
       WiFi немає 30 секунд →
       відкриваємо GILKA-SETUP.
    */

    if (
        !apRunning &&
        millis() -
                wifiLostAt >=
            WIFI_LOST_AP_DELAY_MS)
    {

      startSetupAP();
    }
  }

  // ==================================================
  // CLOSE SETUP AP AFTER WIFI RECOVERY
  // ==================================================

  if (
      connected &&
      apRunning &&
      apCloseAt > 0)
  {

    if (
        (int32_t)(millis() -
                  apCloseAt) >= 0)
    {

      stopSetupAP();
    }
  }

  // ==================================================
  // HOME SCREEN REFRESH
  // ==================================================

  if (
      connected &&
      uiMode ==
          UI_HOME &&
      oledPowerState !=
          OLED_OFF &&
      millis() -
              lastOLEDRefresh >=
          5000)
  {

    lastOLEDRefresh =
        millis();

    showOnline();
  }

  // ==================================================
  // OLED DIM / OFF
  // ==================================================

  oledPowerManager();

  // ==================================================

  wasConnected =
      connected;

  delay(1);
}
