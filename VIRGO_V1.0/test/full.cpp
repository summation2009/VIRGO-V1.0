#include <Arduino.h>
#include <SIM76xx.h>
#include <GSMClient.h>
#include <PubSubClient.h>
#include "DHT.h"
#include "ModbusMaster.h"
#include "esp_system.h"   // ใช้ esp_read_mac

ModbusMaster myModbus;

// ค่าเซนเซอร์ RS485
float Humi = 0.0;
float Temp = 0.0;

// DHT11
#define DHTPIN 25
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// Relay Pins
#define Relay1_PIN26 26
#define Relay2_PIN27 27
#define INPUT18 18
#define INPUT19 19

// ✅ MQTT Config
const char *mqtt_server = "35.236.157.133"; 
const int mqtt_port = 1883;
const char *mqtt_user = "TON_TON";
const char *mqtt_pass = "123456aaff";

// Dynamic MQTT info
String mqtt_client_id;
String mqtt_publish_topic;
String mqtt_subscribe_topic;

GSMClient gsm_client;
PubSubClient client(gsm_client);

// ✅ ดึง MAC Address
String getDeviceMAC() {
  uint8_t mac[6];
  esp_read_mac(mac, ESP_MAC_WIFI_STA);  
  char buf[13];
  sprintf(buf, "%02X%02X%02X%02X%02X%02X",
          mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  return String(buf);
}

// ฟังก์ชัน callback สำหรับ MQTT
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

// ✅ ฟังก์ชัน reconnect ใช้ client_id + user/pass
void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect(mqtt_client_id.c_str(), mqtt_user, mqtt_pass)) {
      Serial.println("connected");
      client.subscribe(mqtt_subscribe_topic.c_str());
      Serial.print("Subscribed to: ");
      Serial.println(mqtt_subscribe_topic);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

// อ่านค่า RS485
void read_sensor_RS485() {
  uint8_t result = myModbus.readHoldingRegisters(0, 4); 
  if (result == myModbus.ku8MBSuccess) {
    Humi = myModbus.getResponseBuffer(0) / 10.0;
    Temp = myModbus.getResponseBuffer(1) / 10.0;
  }
}

// ✅ ฟังก์ชัน Publish ค่าไป MQTT
void publishData(float dhtH, float dhtT, float rsH, float rsT) {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  char msg[250];
  sprintf(msg,
          "{\"DHT_Humi\":%.1f,\"DHT_Temp\":%.1f,"
          "\"RS485_Humi\":%.1f,\"RS485_Temp\":%.1f}",
          dhtH, dhtT, rsH, rsT);

  client.publish(mqtt_publish_topic.c_str(), msg);
  Serial.print("MQTT Publish [");
  Serial.print(mqtt_publish_topic);
  Serial.print("]: ");
  Serial.println(msg);
}

void setup() {
  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N1, 16, 17); // RS485

  dht.begin();

  while (!GSM.begin()) {
    Serial.println("GSM setup fail");
    delay(2000);
  }

  pinMode(Relay1_PIN26, OUTPUT);
  pinMode(Relay2_PIN27, OUTPUT);
  pinMode(INPUT18, INPUT);
  pinMode(INPUT19, INPUT);

  myModbus.begin(1, Serial2); // RS485 Slave ID = 1

  // ✅ สร้าง client_id / topic จาก MAC
  String mac = getDeviceMAC();
  mqtt_client_id     = "ESP32_" + mac;
  mqtt_publish_topic = "device/" + mac + "/data";
  mqtt_subscribe_topic = "device/" + mac + "/cmd";

  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);

  Serial.println("Setup done");
  Serial.print("Client ID: "); Serial.println(mqtt_client_id);
  Serial.print("Publish Topic: "); Serial.println(mqtt_publish_topic);
  Serial.print("Subscribe Topic: "); Serial.println(mqtt_subscribe_topic);
}

unsigned long lastPublish = 0;
const unsigned long publishInterval = 5000; // 5 วินาที
unsigned long buttonPressTime18 = 0;
unsigned long buttonPressTime19 = 0;

void loop() {
  unsigned long now = millis();

  // ✅ ตรวจปุ่ม 18
  if (digitalRead(INPUT18) == LOW) {
    if (buttonPressTime18 == 0) buttonPressTime18 = now; // เริ่มจับเวลา
    if (now - buttonPressTime18 >= 1000) { // กดค้าง >= 1 วิ
      digitalWrite(Relay1_PIN26, LOW);  // เปิด Relay1
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

  // ✅ ส่งข้อมูลทุก 5 วินาที
  if (now - lastPublish >= publishInterval) {
    lastPublish = now;

    // อ่านค่า DHT11
    float h = dht.readHumidity();
    float t = dht.readTemperature();

    if (isnan(h) || isnan(t)) {
      Serial.println(F("Failed to read from DHT11!"));
    } else {
      // อ่าน RS485
      read_sensor_RS485();

      // ส่ง MQTT
      publishData(h, t, Humi, Temp);
    }
  }

  // 👉 ไม่มี delay() แล้ว ทำให้เช็คปุ่มได้ตลอดเวลา
}

