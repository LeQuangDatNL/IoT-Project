#include "DHT.h"
#include <WiFi.h>
#include <PubSubClient.h>
#include <WiFiClientSecure.h>

// --- THÔNG TIN WIFI & MQTT ---
char ssid[] = "Wokwi-GUEST";
char pass[] = "";
const char* mqtt_server = "9a7277b731e1482883f8ff80cdc9a77f.s1.eu.hivemq.cloud";
const int mqtt_port = 8883;
const char* mqtt_username = "datle";
const char* mqtt_password = "Bopbop123123";

// --- ĐỊNH NGHĨA CHÂN KẾT NỐI ---
#define DHTPIN 23          
#define DHTTYPE DHT22      
#define PIR1_PIN 33        
#define PIR2_PIN 32        
#define GAS_PIN 34         
#define LED_RED 12         
#define LED_WHITE 17       

DHT dht(DHTPIN, DHTTYPE);
WiFiClientSecure espClient;
PubSubClient client(espClient);

unsigned long lastMsg = 0;

void setup_wifi() {
  Serial.println("\nConnecting WiFi...");
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected!");
}

void callback(char* topic, byte* payload, unsigned int length) {
  String message = "";
  for (int i = 0; i < length; i++) message += (char)payload[i];

  // In ngắn gọn: [Topic] Nội dung
  Serial.print("[");
  Serial.print(topic);
  Serial.print("] ");
  Serial.println(message);

  if (String(topic) == "esp/led") {
    if (message == "ON") digitalWrite(LED_WHITE, HIGH);
    else if (message == "OFF") digitalWrite(LED_WHITE, LOW);
  }
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Connecting MQTT...");
    String clientId = "ESP32_DatHUSC_";
    clientId += String(random(0xffff), HEX);

    if (client.connect(clientId.c_str(), mqtt_username, mqtt_password)) {
      Serial.println("Connected!");
      client.subscribe("esp/led"); 
    } else {
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  dht.begin();
  pinMode(PIR1_PIN, INPUT);
  pinMode(PIR2_PIN, INPUT);
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_WHITE, OUTPUT);
  
  digitalWrite(LED_RED, LOW);
  digitalWrite(LED_WHITE, LOW);

  setup_wifi();
  espClient.setInsecure(); 
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  float h = dht.readHumidity();
  float t = dht.readTemperature();
  int m1 = digitalRead(PIR1_PIN);
  int m2 = digitalRead(PIR2_PIN);
  int gas = analogRead(GAS_PIN);

  // Logic điều khiển LED tại chỗ
  if (gas > 500 || m2 == HIGH) digitalWrite(LED_RED, HIGH);
  else digitalWrite(LED_RED, LOW);

  if (m1 == HIGH) digitalWrite(LED_WHITE, HIGH);
  else digitalWrite(LED_WHITE, LOW); 

  // --- GỬI DỮ LIỆU JSON (Không kèm trạng thái LED) ---
  if (millis() - lastMsg > 5000) {
    lastMsg = millis();

    String payload = "{";
    payload += "\"temp\":" + String(t, 1) + ",";
    payload += "\"humi\":" + String(h, 1) + ",";
    payload += "\"gas\":" + String(gas) + ",";
    payload += "\"p1\":" + String(m1) + ",";
    payload += "\"p2\":" + String(m2);
    payload += "}";

    Serial.println("Publishing: " + payload);
    client.publish("esp/home", payload.c_str());
  }
}