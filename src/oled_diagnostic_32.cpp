#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Deliberately use a real 512-byte framebuffer and 1/32 multiplex scan.
// This tests panel geometry; it does not modify the normal application or NVS.
Adafruit_SSD1306 oled32(128, 32, &Wire, -1, 100000, 100000);

void setup()
{
  Serial.begin(115200);
  delay(1200);
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
  if (!address || !oled32.begin(SSD1306_SWITCHCAPVCC, address, false, false))
  {
    Serial.println("OLED initialization failed");
    return;
  }
  // The library selects COM 0x02 and multiplex 31 for 128x32.
  oled32.setRotation(2);
  oled32.ssd1306_command(SSD1306_SETCONTRAST);
  oled32.ssd1306_command(40);
  oled32.clearDisplay();
  oled32.setTextColor(SSD1306_WHITE);
  oled32.setTextSize(1);
  oled32.setTextWrap(false);
  const char *lines[] = {
      "GILKA 128x32 TEST", "0123456789 ABCDE",
      "gilka.local", "GILKA  ONLINE"};
  for (int row = 0; row < 4; ++row)
  {
    oled32.setCursor(0, row * 8);
    oled32.print(lines[row]);
  }
  oled32.display();
  Serial.println("STATIC 128x32 text test: COM 0x02, multiplex 31, rotation 2.");
}

void loop()
{
  delay(10);
}
