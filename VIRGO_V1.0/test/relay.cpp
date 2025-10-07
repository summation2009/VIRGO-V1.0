#include <Arduino.h>

#define Relay1_PIN26 26
#define Relay2_PIN27 27
#define INPUT18 18
#define INPUT19 19

void setup() {
  Serial.begin(115200);

  pinMode(Relay1_PIN26, OUTPUT);
  pinMode(Relay2_PIN27, OUTPUT);
  pinMode(INPUT18, INPUT_PULLUP);
  pinMode(INPUT19, INPUT_PULLUP);

  digitalWrite(Relay1_PIN26, HIGH);  // ปิด Relay ตอนเริ่ม
  digitalWrite(Relay2_PIN27, HIGH);
}

void loop() {
  // ปุ่ม 18 → Relay 1
  if (digitalRead(INPUT18) == LOW) {
    digitalWrite(Relay1_PIN26, LOW);  // กดปุ่ม → เปิด Relay
  } else {
    digitalWrite(Relay1_PIN26, HIGH); // ปล่อยปุ่ม → ปิด Relay
  }

  // ปุ่ม 19 → Relay 2
  if (digitalRead(INPUT19) == LOW) {
    digitalWrite(Relay2_PIN27, LOW);
  } else {
    digitalWrite(Relay2_PIN27, HIGH);
  }
}
