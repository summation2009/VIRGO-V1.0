#include <Arduino.h>
#include <SIM76xx.h>
#include <GSMClient.h>
#include <PubSubClient.h>
#include "DHT.h"
#include "ModbusMaster.h"

ModbusMaster myModbus;

float Humi = 0.0;
float Temp = 0.0;

//Constants
#define DHTPIN 25     // what pin we're connected to
#define DHTTYPE DHT22   // DHT 22  (AM2302)
DHT dht(DHTPIN, DHTTYPE); //// Initialize DHT sensor for normal 16mhz Arduino

// Relay Pins
#define Relay1_PIN26 26
#define Relay2_PIN27 27

#define INPUT18 18
#define INPUT19 19

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i=0;i<length;i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

GSMClient gsm_client;
PubSubClient client(gsm_client);

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("arduinoClient")) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish("outTopic","hello world");
      // ... and resubscribe
      client.subscribe("inTopic");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void read_sensor()
{

  // digitalWrite(DE_RE_PIN, MODE_SEND);
  // delay(100);
  uint8_t result;

  myModbus.begin(1, Serial2); // Slave ID = 1 (ID ของตัวเซนเซอร์ที่ต้องการอ่านค่า), โดยใช้พอร์ต Serial2 เป็นพอร์ตสื่อสาร
                              // digitalWrite(DE_RE_PIN, MODE_SEND);

  result = myModbus.readHoldingRegisters(0, 4); // เริ่มอ่านค่าที่ตำแหน่งรีจิสเตอร์ 0, เป็นจำนวน 4 รีจิสเตอร์
  // digitalWrite(DE_RE_PIN, MODE_RECV);

  if (result == myModbus.ku8MBSuccess)
  {
    Humi = myModbus.getResponseBuffer(0) / 10.0;
    Temp = myModbus.getResponseBuffer(1) / 10.0;

    Serial.println("=============RS485===========");

    Serial.println(Humi);
    Serial.println(Temp);
  }

  // delay(250);
}

void setup() {
  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N1, 16, 17, false, 1000);

  dht.begin();

  while(!GSM.begin()) {
    Serial.println("GSM setup fail");
    delay(2000);
  }

  pinMode(Relay1_PIN26, OUTPUT);
  pinMode(Relay2_PIN27, OUTPUT);
  pinMode(INPUT18, INPUT);
  pinMode(INPUT19, INPUT);

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



  client.setServer("35.236.157.133", 1883);
  client.setCallback(callback);

}

void loop() {

    // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  float h = dht.readHumidity();
  // Read temperature as Celsius (the default)
  float t = dht.readTemperature();
  // Read temperature as Fahrenheit (isFahrenheit = true)
  float f = dht.readTemperature(true);

  // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(t) || isnan(f)) {
    Serial.println(F("Failed to read from DHT sensor!"));
    return;
  }

  // Compute heat index in Fahrenheit (the default)
  float hif = dht.computeHeatIndex(f, h);
  // Compute heat index in Celsius (isFahreheit = false)
  float hic = dht.computeHeatIndex(t, h, false);

    Serial.println("=============DHT22===========");


  Serial.print(F("Humidity: "));
  Serial.print(h);
  Serial.print(F("%  Temperature: "));
  Serial.print(t);
  Serial.print(F("°C "));
  Serial.print(f);
  Serial.print(F("°F  Heat index: "));
  Serial.print(hic);
  Serial.print(F("°C "));
  Serial.print(hif);
  Serial.println(F("°F"));


  // ปุ่มที่ 18 (กดค้าง = เปิด relay 1 / ปล่อย = ปิด)
  if (digitalRead(INPUT18) == LOW)
  {
    digitalWrite(Relay1_PIN26, LOW);
  }
  else
  {
    digitalWrite(Relay1_PIN26, HIGH);
  }

    // ปุ่มที่ 18 (กดค้าง = เปิด relay 1 / ปล่อย = ปิด)
  if (digitalRead(INPUT19) == LOW)
  {
    digitalWrite(Relay2_PIN27, LOW);
  }
  else
  {
    digitalWrite(Relay2_PIN27, HIGH);
  }

  if (!client.connected()) {
    reconnect();
  }
  client.loop();
read_sensor();
  delay(500);
}
