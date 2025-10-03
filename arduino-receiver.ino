#include <MCUFRIEND_kbv.h>
#include <Adafruit_GFX.h>
#include "FontsRus/TimesNRCyr10.h"

MCUFRIEND_kbv tft;

// ---- Конвертер UTF-8 → cp1251 ----
String utf8ToCp1251(const char* utf8) {
  String out;
  while (*utf8) {
    uint8_t c = (uint8_t)*utf8++;
    if (c == 0xD0 || c == 0xD1) {
      uint8_t c2 = (uint8_t)*utf8++;
      if (c == 0xD0 && c2 >= 0x90 && c2 <= 0xBF) {
        out += char(c2 - 0x90 + 0xC0); // А–Я
      } else if (c == 0xD1 && c2 >= 0x80 && c2 <= 0x8F) {
        out += char(c2 - 0x80 + 0xE0); // а–п
      }
    } else {
      out += char(c); // латиница, цифры, знаки
    }
  }
  return out;
}

void setup() {
  Serial.begin(9600);
  while (!Serial) { }
  Serial.println("TFT Shield test (UTF-8)");

  uint16_t ID = tft.readID();
  Serial.print("readID = 0x");
  Serial.println(ID, HEX);

  tft.begin(ID);
  tft.setRotation(1);
  tft.fillScreen(0x0000);

  // включаем кириллический шрифт
  tft.setFont(&TimesNRCyr10pt8b);
  tft.setTextColor(0xFFFF); // белый
  tft.setCursor(10, 60);

  // печать текста в UTF-8
  String txt = "Привет, мир! Я Рома";
  tft.println(txt);

  // примеры фигур
  tft.fillRect(10, 100, 100, 50, 0xF800); // красный прямоугольник
  tft.drawCircle(180, 150, 30, 0x07E0);   // зелёный круг
}

void loop() {
}
