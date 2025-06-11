#include <Wire.h>
#include <MQ135.h>
#include <DHT.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
#include <WiFi.h>
#include <PubSubClient.h>

// --- WiFi & MQTT config ---
const char* ssid = "UTC_A2";
const char* password = "";
const char* mqttServer = "10.90.110.183";
const int mqttPort = 1883;
const char* tempTopic = "temperature2";
const char* humTopic  = "humidity2";
const char* lpgTopic  = "lpg";

WiFiClient espClient;
PubSubClient client(espClient);

// --- Pin Config ---
const int DHTPIN = 25;
const int MQ135PIN = 32;
const int Buzzer = 33;
const int Relay_1 = 14;
const int Relay_2 = 27;

// --- DHT setup ---
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// --- MQ135 setup ---
const float Vcc = 5.0;
const float RL = 10.0;
float R0 = 76.63;

// --- OLED setup ---
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// --- Data struct ---
typedef struct {
  char phong[10];
  float nhiet_do;
  float do_am;
  float khi_gas;
} Data;

Data myData;

// --- MQTT callback ---
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message received on topic: ");
  Serial.println(topic);

  String message = "";
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  message.trim();
  message.toLowerCase();  // chuẩn hóa chuỗi

  Serial.print("Payload: '");
  Serial.print(message);

  if (strcmp(topic, "relay3/control") == 0) {
    if (message == "true") {
      digitalWrite(Relay_2, HIGH);
      client.publish("relay3/status", "true");
    } else if (message == "false") {
      digitalWrite(Relay_2, LOW);
      client.publish("relay3/status", "false");
    }
  }
}

// --- setup ---
void setup() {
  Serial.begin(115200);
  dht.begin();
  
  pinMode(Relay_1, OUTPUT);
  pinMode(Relay_2, OUTPUT);
  pinMode(Buzzer, OUTPUT);
  digitalWrite(Buzzer, LOW);
  digitalWrite(Relay_1, LOW);
  digitalWrite(Relay_2, LOW);

  // OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    while (true);
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);

  // WiFi
  WiFi.begin(ssid, password);
  Serial.print("Đang kết nối WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\nKết nối WiFi thành công!");

  // MQTT
  client.setServer(mqttServer, mqttPort);
  client.setCallback(callback);
  reconnect();
}

// --- reconnect MQTT ---
void reconnect() {
  static unsigned long lastRetry = 0;
  if (millis() - lastRetry < 5000) return;

  Serial.print("Connecting to MQTT...");
  if (client.connect("ESP32_Client")) {
    Serial.println("Connected!");
    client.subscribe("relay3/control");
  } else {
    Serial.print("Failed, rc=");
    Serial.print(client.state());
    Serial.println(" retrying in 5 seconds...");
  }
  lastRetry = millis();
}

// --- main loop ---
void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  strncpy(myData.phong, "Phong bep", sizeof(myData.phong));
  myData.nhiet_do = dht.readTemperature();
  myData.do_am = dht.readHumidity();

  int raw = analogRead(MQ135PIN);
  float voltage = raw * (Vcc / 4095.0);
  float RS = (Vcc - voltage) * RL / voltage;
  float ratio = RS / R0;
  myData.khi_gas = 116.6020682*pow(ratio, -2.769034857);  // ppm khí gas (HC)

  // Cảnh báo
  if (myData.khi_gas > 20000.0) {
    digitalWrite(Buzzer, HIGH);
    digitalWrite(Relay_1, HIGH);
  } else {
    digitalWrite(Buzzer, LOW);
    digitalWrite(Relay_1, LOW);
  }

  // Hiển thị OLED
  display.clearDisplay();
  display.setCursor(0, 0);
  display.print("Nhiet do: ");
  display.print(myData.nhiet_do);
  display.print((char)247);
  display.print("C");

  display.setCursor(0, 16);
  display.print("Do am: ");
  display.print(myData.do_am);
  display.print("%");

  display.setCursor(0, 32);
  display.print("Khi gas: ");
  display.print(myData.khi_gas);
  display.print(" ppm");

  if (myData.khi_gas > 20000.0) {
    display.setCursor(0, 48);
    display.print("CANH BAO KHI GAS!");
  }

  display.display();

  // Gửi MQTT
  char tempStr[10], humStr[10], gasStr[10];
  dtostrf(myData.nhiet_do, 4, 2, tempStr);
  dtostrf(myData.do_am, 4, 2, humStr);
  dtostrf(myData.khi_gas, 6, 2, gasStr);

  client.publish(tempTopic, tempStr);
  client.publish(humTopic, humStr);
  client.publish(lpgTopic, gasStr);
  delay(2000);
  
  Serial.println("📤 Gửi MQTT thành công");
}
