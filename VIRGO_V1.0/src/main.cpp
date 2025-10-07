#include <Arduino.h>
#include "DHT.h"
#include "ModbusMaster.h"

// RS485 Modbus
ModbusMaster myModbus;
float Humi = 0.0;
float Temp = 0.0;

// DHT22
#define DHTPIN 25
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

// Relay + Input Pins
#define Relay1_PIN26 26
#define Relay2_PIN27 27
#define INPUT18 18
#define INPUT19 19

unsigned long lastPublish = 0;
const unsigned long publishInterval = 5000; // 5 วินาที
unsigned long buttonPressTime18 = 0;
unsigned long buttonPressTime19 = 0;

// อ่านค่า RS485
void read_sensor_RS485() {
  uint8_t result = myModbus.readHoldingRegisters(0, 2); // อ่าน 2 ตัวพอ
  if (result == myModbus.ku8MBSuccess) {
    Humi = myModbus.getResponseBuffer(0) / 10.0;
    Temp = myModbus.getResponseBuffer(1) / 10.0;
  }
}

void setup() {
  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N1, 16, 17); // RS485

  dht.begin();

  pinMode(Relay1_PIN26, OUTPUT);
  pinMode(Relay2_PIN27, OUTPUT);
  pinMode(INPUT18, INPUT);
  pinMode(INPUT19, INPUT);

  // Relay Blink Test
  digitalWrite(Relay1_PIN26, LOW);
  digitalWrite(Relay2_PIN27, LOW);
  delay(250);
  digitalWrite(Relay1_PIN26, HIGH);
  digitalWrite(Relay2_PIN27, HIGH);
  delay(250);
  digitalWrite(Relay1_PIN26, LOW);
  digitalWrite(Relay2_PIN27, LOW);
  delay(250);
  digitalWrite(Relay1_PIN26, HIGH);
  digitalWrite(Relay2_PIN27, HIGH);
  delay(250);

  myModbus.begin(1, Serial2); // RS485 Slave ID = 1

  Serial.println("Setup done");
}

void loop() {
  unsigned long now = millis();

  // ✅ ตรวจปุ่ม 18
  if (digitalRead(INPUT18) == LOW) {
    if (buttonPressTime18 == 0) buttonPressTime18 = now; // เริ่มจับเวลา
    if (now - buttonPressTime18 >= 1000) { // กดค้าง >= 1 วิ
      digitalWrite(Relay1_PIN26, LOW);  // เปิด Relay1 (Active LOW)
    }
  } else {
    buttonPressTime18 = 0;
    digitalWrite(Relay1_PIN26, HIGH);   // ปล่อยปิด Relay1
  }

  // ✅ ตรวจปุ่ม 19
  if (digitalRead(INPUT19) == LOW) {
    if (buttonPressTime19 == 0) buttonPressTime19 = now;
    if (now - buttonPressTime19 >= 1000) {
      digitalWrite(Relay2_PIN27, LOW);  // เปิด Relay2
    }
  } else {
    buttonPressTime19 = 0;
    digitalWrite(Relay2_PIN27, HIGH);   // ปล่อยปิด Relay2
  }

  // ✅ อ่านค่า sensor ทุก 5 วินาที
  if (now - lastPublish >= publishInterval) {
    lastPublish = now;

    float h = dht.readHumidity();
    float t = dht.readTemperature();

    if (isnan(h) || isnan(t)) {
      Serial.println(F("Failed to read from DHT22!"));
    } else {
      read_sensor_RS485();

      Serial.print("DHT22 -> Humidity: ");
      Serial.print(h);
      Serial.print("%  Temp: ");
      Serial.print(t);
      Serial.println("°C");

      Serial.print("RS485 -> Humidity: ");
      Serial.print(Humi);
      Serial.print("%  Temp: ");
      Serial.print(Temp);
      Serial.println("°C");
      Serial.println(" ");
    }
  }
  
}
