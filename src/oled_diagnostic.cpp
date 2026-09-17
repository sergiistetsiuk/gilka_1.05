#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

namespace
{
Adafruit_SSD1306 oled(128, 64, &Wire, -1, 100000, 100000);
bool ready = false;
uint8_t stage = 7;
uint8_t comPins = 0x12;
uint32_t stageStarted = 0;
bool paused = true;
constexpr uint32_t STAGE_MS = 8000;
constexpr uint8_t STAGE_COUNT = 8;

void showStage()
{
  // Explicitly establish the full 64-row scan, normal RAM output and origin.
  oled.ssd1306_command(SSD1306_DISPLAYOFF);
  oled.ssd1306_command(SSD1306_SETCOMPINS);
  oled.ssd1306_command(comPins);
  oled.ssd1306_command(SSD1306_SETMULTIPLEX);
  oled.ssd1306_command(63);
  oled.ssd1306_command(SSD1306_SETDISPLAYOFFSET);
  oled.ssd1306_command(0);
  oled.ssd1306_command(SSD1306_SETSTARTLINE);
  oled.ssd1306_command(SSD1306_NORMALDISPLAY);
  oled.ssd1306_command(SSD1306_DISPLAYALLON_RESUME);
  oled.clearDisplay();
  oled.setTextColor(SSD1306_WHITE);
  oled.setTextSize(1);
  oled.setTextWrap(false);

  const char *names[] = {
      "ALL PIXELS ON (controller, bypasses framebuffer)",
      "ALL PIXELS ON (framebuffer)",
      "EVEN logical rows only", "ODD logical rows only",
      "TEXT at Y=0", "SAME TEXT shifted down one pixel", "BLACK",
      "LEFT even / CENTER odd / RIGHT all; bottom double-height text"};
  Serial.printf("COM=0x%02X | test %u/%u | %s\n", comPins,
                stage + 1, STAGE_COUNT, names[stage]);

  if (stage == 1)
    oled.fillScreen(SSD1306_WHITE);
  else if (stage == 2 || stage == 3)
  {
    for (int y = stage - 2; y < 64; y += 2)
      oled.drawFastHLine(0, y, 128, SSD1306_WHITE);
  }
  else if (stage == 4 || stage == 5)
  {
    int shift = stage - 4;
    const char *lines[] = {"GILKA OLED TEST", "0123456789 ABCDE", "gilka.local", "HIMW 8888 XXXX"};
    for (int row = 0; row < 4; ++row)
    {
      oled.setCursor(0, row * 11 + shift);
      oled.print(lines[row]);
    }
    oled.drawFastHLine(0, 47, 128, SSD1306_WHITE);
    oled.setCursor(0, 52 + shift);
    oled.print("GILKA  ONLINE");
  }

  if (stage == 7)
  {
    for (int y = 0; y < 46; ++y)
    {
      oled.drawFastHLine((y % 2 == 0) ? 0 : 44, y, 40, SSD1306_WHITE);
      oled.drawFastHLine(88, y, 40, SSD1306_WHITE);
    }
    // Each font row occupies both row parities. This is a readability test,
    // not a repair of the underlying row-output problem.
    oled.setTextSize(1, 2);
    oled.setCursor(0, 48);
    oled.print("GILKA 012345");
  }

  oled.display();
  if (stage == 0)
    oled.ssd1306_command(SSD1306_DISPLAYALLON);
  oled.ssd1306_command(SSD1306_DISPLAYON);
  stageStarted = millis();
}
}

void setup()
{
  Serial.begin(115200);
  delay(1200);
  Serial.println("GILKA OLED diagnostic: starts PAUSED on row comparison.");
  Serial.println("When resumed: 8 seconds/test, then repeat.");
  Serial.println("Serial: n=next, p=pause/resume, a=COM 0x12, s=COM 0x02.");
  Wire.begin(1, 2);
  Wire.setClock(100000);
  uint8_t address = 0;
  const uint8_t candidates[] = {0x3C, 0x3D};
  for (uint8_t candidate : candidates)
  {
    Wire.beginTransmission(candidate);
    if (Wire.endTransmission() == 0)
    {
      address = candidate;
      break;
    }
  }
  if (!address || !oled.begin(SSD1306_SWITCHCAPVCC, address, false, false))
  {
    Serial.println("OLED not found or framebuffer allocation failed.");
    return;
  }
  ready = true;
  oled.setRotation(2);
  oled.ssd1306_command(SSD1306_SETCONTRAST);
  oled.ssd1306_command(40);
  showStage();
}

void loop()
{
  if (!ready)
  {
    delay(10);
    return;
  }
  if (Serial.available())
  {
    char command = Serial.read();
    if (command == 'p')
    {
      paused = !paused;
      stageStarted = millis();
      Serial.println(paused ? "PAUSED" : "RESUMED");
    }
    else if (command == 'n')
    {
      stage = (stage + 1) % STAGE_COUNT;
      showStage();
    }
    else if (command == 'a' || command == 's')
    {
      comPins = command == 'a' ? 0x12 : 0x02;
      showStage();
    }
  }
  if (!paused && millis() - stageStarted >= STAGE_MS)
  {
    stage = (stage + 1) % STAGE_COUNT;
    showStage();
  }
  delay(1);
}
