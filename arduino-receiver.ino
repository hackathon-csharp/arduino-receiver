// для экрана
#include <LiquidCrystal.h>
#include "datapacklib.h"
#include <string.h>

// Adafruit TCS34725 - это добавить в либы
#include <Wire.h>
#include "Adafruit_TCS34725.h"

// Подключение в 8-битном режиме:
// LiquidCrystal(rs, rw, enable, d0, d1, d2, d3, d4, d5, d6, d7)
LiquidCrystal lcd(13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3);


// Создаем объект датчика (интеграция 600ms, усиление 1x)
Adafruit_TCS34725 tcs = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_24MS, TCS34725_GAIN_60X);

struct Ratio {
  datapack::LightLevel level;
  double ratio, value;
};

void print(const datapack::LightLevel& level) {
  Serial.print("level: ");
  if (level == datapack::LightLevel::Blue)
    Serial.println("Blue");
  else if (level == datapack::LightLevel::Red)
    Serial.println("Red");
  else if (level == datapack::LightLevel::Green)
    Serial.println("Green");
  else if (level == datapack::LightLevel::White)
    Serial.println("White");
  else Serial.println("Off");
}

void printToLCD(const datapack::LightLevel& level) {
  lcd.print("level:");
  if (level == datapack::LightLevel::Blue)
    lcd.print("Blue");
  else if (level == datapack::LightLevel::Red)
    lcd.print("Red");
  else if (level == datapack::LightLevel::Green)
    lcd.print("Green");
  else if (level == datapack::LightLevel::White)
    lcd.print("White");
  else lcd.print("Off");
}

void swap1(Ratio& r1, Ratio& r2) {
  Ratio r3 = r1;
  r1 = r2;
  r2 = r3;
}

double max1(double x1, double x2, double x3) {
  if (x1 >= x2 && x2 >= x3) return x1;
  if (x2 >= x3 && x2 >= x3) return x2;
  return x3;
}

double min1(double x1, double x2, double x3) {
  if (x1 <= x2 && x2 <= x3) return x1;
  if (x2 <= x3 && x2 <= x3) return x2;
  return x3;
}

double abs1(double x) {
  return x >= 0 ? x : -x;
}

datapack::LightLevel detectColor(uint16_t r, uint16_t g, uint16_t b, uint16_t lux) {
  if (lux < 1000)
    return datapack::LightLevel::Off;

  b = b * 4 / 5;
  if (r > g + b)
    return datapack::LightLevel::Red;
  if (g > r + b)
    return datapack::LightLevel::Green;
  if (b > r + g)
    return datapack::LightLevel::Blue;
  return datapack::LightLevel::White;

  double avgR = r, avgG = g, avgB = b;
  double brightness = (avgR + avgG + avgB) / 3;
  double total = avgR + avgG + avgB;

  if (total <= 0) return datapack::LightLevel::Off;
  double rRatio = avgR / total;
  double gRatio = avgG / total;
  double bRatio = avgB / total;

  Ratio r1 = {datapack::LightLevel::Red, rRatio, avgR};
  Ratio r2 = {datapack::LightLevel::Green, gRatio, avgG};
  Ratio r3 = {datapack::LightLevel::Blue, bRatio, avgB};

  if (r2.ratio < r3.ratio) swap1(r2, r3);
  if (r1.ratio < r2.ratio) swap1(r1, r2);
  if (r2.ratio < r3.ratio) swap1(r1, r2);

  double ratioGap = r1.ratio - r2.ratio;
  double channelSpread = max1(avgR, avgG, avgB) - min1(avgR, avgG, avgB);
  double whiteness = max1(
    abs(rRatio - gRatio),
    abs(rRatio - bRatio),
    abs(gRatio - bRatio)
  );
  double absoluteGap = r1.value - r2.value;

  datapack::LightLevel dominant = datapack::LightLevel::Off;
  if (brightness >= 45 && r1.value >= 70) {
    if (whiteness < 0.12 && brightness >= 65 && r2.value >= 80 && r1.value >= 80) {
      dominant = datapack::LightLevel::White;
    } else if (ratioGap >= 0.1 && absoluteGap >= 45) {
      dominant = r1.level;
    } else if (channelSpread < 25 && brightness >= 80) {
      dominant = datapack::LightLevel::White;
    }
  }

  if (dominant == datapack::LightLevel::Off && brightness >= 90 && whiteness < 0.15 && r1.value >= 95 && r2.value >= 90) {
    dominant = datapack::LightLevel::White;
  }

  return dominant;
}

uint8_t receivedData[512];
void myOnPacketReceived(datapack::UnpackedPackage pkg) {
  Serial.println("Пакет собрался!!!");
  size_t receivedLen = datapack::getReceivedData(receivedData);

  Serial.print("Pkg idx: ");
  Serial.println(pkg.index);

  String asciiMessage = "";
  bool started = false;
  for (size_t i = 0; i < receivedLen; ++i) {
    if (asciiMessage.length() >= 32) {
      break;
    }

    char ch = static_cast<char>(receivedData[i]);
    if (ch == '\0') {
      if (started) {
        break;
      }
      continue;
    }

    if (ch == '\r') {
      continue;
    }

    if (ch == '\n') {
      if (asciiMessage.length() < 32) {
        asciiMessage += ' ';
      }
      started = true;
      continue;
    }

    if (ch >= 32 && ch <= 126) {
      asciiMessage += ch;
      started = true;
    }
  }

  if (asciiMessage.length() == 0) {
    asciiMessage = "<no data>";
  }

  Serial.print("ASCII: ");
  Serial.println(asciiMessage);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(asciiMessage);

  lcd.setCursor(0, 1);
  if (asciiMessage.length() > 16) {
    int bottomEnd = asciiMessage.length();
    if (bottomEnd > 32) bottomEnd = 32;
    lcd.print(asciiMessage.substring(16, bottomEnd));
  } else {
    lcd.print("Idx:");
    lcd.print(pkg.index);
  }
  
}

void addPacket(datapack::LightLevel level, long duraction) {
  Serial.print("Packet duraction: ");
  Serial.print(duraction);
  Serial.print(" ");
  datapack::feed({level, duraction});
}

void setup() {
  datapack::onPacketReceived = myOnPacketReceived;
  datapack::min_duration = 110;
  lcd.begin(16, 2);    // 16 символов, 2 строки
  


  Serial.begin(9600);
  Serial.println("TCS34725 Color Sensor Test");

  if (tcs.begin()) {
    Serial.println("Датчик TCS34725 найден и инициализирован!");
  } else {
    Serial.println("Ошибка: датчик TCS34725 не найден. Проверь подключение SDA/SCL.");
    while (1) {
      Serial.println("Ошибка: датчик TCS34725 не найден. Проверь подключение SDA/SCL.");
    } // остановка
  }
}

datapack::LightLevel prevLevel = datapack::LightLevel::Off;
uint16_t r3 = 0, g3 = 0, b3 = 0;
int lstDisplayUpdateTime = 0, frames = 0;
int lstPacketCreateTime = 0;
void loop() {
  uint16_t r, g, b, c;
  float colorTemp, lux;

  // Считываем данные
  tcs.getRawData(&r, &g, &b, &c);
  colorTemp = tcs.calculateColorTemperature(r, g, b);
  lux = tcs.calculateLux(r, g, b);
  r = r / 65535.0 * 255;
  g = g / 65535.0 * 255;
  b = b / 65535.0 * 255;

  int time = millis();
  auto level = detectColor(r, g, b, lux);
  if (prevLevel != level) {
    print(level);
    addPacket(level, time - lstPacketCreateTime);
    lstPacketCreateTime = time;
    prevLevel = level;
  }
  frames++;
  
  if (lstDisplayUpdateTime < time && false) {
    // Вывод на LCD
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("R:"); 
    lcd.print(r);
    lcd.print(" G:"); 
    lcd.print(g);
    lcd.print(" B:"); 
    lcd.print(b);
    lcd.setCursor(0, 1);
    lcd.print("F:"); 
    lcd.print(frames);
    printToLCD(level);
    // lcd.print(" Lx:"); lcd.print((int)lux); // опционально освещенность


    // Вывод в монитор
    Serial.print("R: "); Serial.print(r);
    Serial.print("  G: "); Serial.print(g);
    Serial.print("  B: "); Serial.print(b);
    Serial.print("  C: "); Serial.print(c);
    Serial.print("  Temp (K): "); Serial.print(colorTemp);
    Serial.print("  Lux: "); Serial.println(lux);
    lstDisplayUpdateTime = time + 10000000000;
    frames = 0;
  }

  delay(10); 
}