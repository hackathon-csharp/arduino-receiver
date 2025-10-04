#include <Wire.h>
#include <Adafruit_TCS34725.h>
#include <MCUFRIEND_kbv.h>
#include <Adafruit_GFX.h>

// Создаем объект датчика TCS34725 (интеграция 24ms, усиление 4x)
Adafruit_TCS34725 tcs = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_24MS, TCS34725_GAIN_4X);

// Создаем объект TFT-экрана
MCUFRIEND_kbv tft;
uint16_t ID;

void setup() {
  Serial.begin(9600);
  while (!Serial) { } // Ждем инициализации Serial
  Serial.println("TCS34725 + TFT Test");

  // Инициализация датчика
  if (tcs.begin()) {
    Serial.println("TCS34725 sensor initialized!");
  } else {
    Serial.println("Error: TCS34725 not found. Check SDA/SCL connections.");
    while (1); // Остановка при ошибке
  }
  tcs.setInterrupt(true); // Включаем прерывание датчика

  // Инициализация TFT-экрана
  ID = tft.readID();
  Serial.print("TFT ID = 0x");
  Serial.println(ID, HEX);
  
  tft.begin(ID);
  tft.setRotation(1); // Горизонтальная ориентация
  tft.fillScreen(0x0000); // Черный фон (очищаем экран один раз)
  tft.setTextColor(0xFFFF); // Белый цвет текста
  tft.setTextSize(2); // Размер текста
}


void loop() {
  uint16_t r, g, b, c;
  float colorTemp, lux;

  // Считываем данные с датчика
  tcs.getRawDataOneShot(&r, &g, &b, &c);
  colorTemp = tcs.calculateColorTemperature(r, g, b);
  lux = tcs.calculateLux(r, g, b);

  // Нормализация RGB до 0-255
  r = (r / 65535.0) * 255;
  g = (g / 65535.0) * 255;
  b = (b / 65535.0) * 255;

  // Вывод в Serial для отладки
  Serial.print("R: "); Serial.print(r);
  Serial.print(" G: "); Serial.print(g);
  Serial.print(" B: "); Serial.print(b);
  Serial.print(" C: "); Serial.print(c);
  Serial.print(" Temp (K): "); Serial.print(colorTemp);
  Serial.print(" Lux: "); Serial.println(lux);

  tft.begin(ID);

  // Очистка только области текста на экране (чтобы избежать мерцания)
  tft.fillRect(10, 10, 300, 100, 0x0000); // Черный прямоугольник для очистки области текста
  tft.setCursor(10, 10); // Устанавливаем курсор в начало

  // Вывод данных на TFT-экран
  tft.print("R: "); tft.print(r);
  tft.print(" G: "); tft.print(g);
  tft.print(" B: "); tft.print(b);
  tft.setCursor(10, 30); // Следующая строка
  tft.print("Clear: "); tft.print(c);
  tft.setCursor(10, 50); // Следующая строка
  tft.print("Temp(K): "); tft.print(colorTemp, 0); // Без дробных
  tft.setCursor(10, 70); // Следующая строка
  tft.print("Lux: "); tft.print(lux, 0); // Без дробных

  tft.endWrite();
  // Задержка для читаемости
  delay(500);
}
