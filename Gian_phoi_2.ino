#include <WiFi.h>
#include <PubSubClient.h>

// WiFi & MQTT Config
const char* ssid = "DucK";
const char* password = "Manngusidan";
const char* mqtt_server = "172.20.10.3"; // IP Node-RED hoặc MQTT Broker
const int mqtt_port = 1883;

WiFiClient espClient;
PubSubClient client(espClient);

int state = 0; // 1: Stop, 2: Forward, 3: Backward

const int stepPin = 13;   
const int dirPin = 23;    
const int enPin  = 14;    

int btnForward = 18;      
int btnBackward = 16;     
int btnStop = 17;         

int limitForward = 25;    
int limitBackward = 26;   

void setup() {
  Serial.begin(115200);

  // Setup motor pins
  pinMode(stepPin, OUTPUT);
  pinMode(dirPin, OUTPUT);
  pinMode(enPin, OUTPUT);
  digitalWrite(enPin, LOW); // bật driver

  // Setup buttons
  pinMode(btnForward, INPUT_PULLUP);
  pinMode(btnBackward, INPUT_PULLUP);
  pinMode(btnStop, INPUT_PULLUP);

  // Setup limit switches
  pinMode(limitForward, INPUT_PULLUP);
  pinMode(limitBackward, INPUT_PULLUP);

  // Setup WiFi
  setup_wifi();

  // Setup MQTT
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(mqttCallback);
}

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String message = "";
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  message.trim();
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  Serial.println(message);

  int command = message.toInt();  // chuyển chuỗi sang số nguyên

  if (command >= 1 && command <= 3) {
    state = command;
  }
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("ESP32Client")) {
      Serial.println("connected");
      client.subscribe("motor/control"); // đăng ký topic
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 2 seconds");
      delay(2000);
    }
  }
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  int valForward = digitalRead(btnForward);
  int valBackward = digitalRead(btnBackward);
  int valStop = digitalRead(btnStop);

  int valLimitFwd = digitalRead(limitForward);
  int valLimitBwd = digitalRead(limitBackward);

  // Ưu tiên nút nhấn vật lý
  if (valForward == LOW)  state = 2;
  if (valBackward == LOW) state = 3;
  if (valStop == LOW)     state = 1;

  // Ngắt quay khi chạm giới hạn
  if (state == 2 && valLimitFwd == LOW)  state = 1;
  if (state == 3 && valLimitBwd == LOW)  state = 1;

  // Điều khiển theo trạng thái
  switch (state) {
    case 1: // Dừng
      digitalWrite(enPin, HIGH);
      Serial.println("Dung");
      break;

    case 2: // Quay thuận
      Serial.println("Len");
      digitalWrite(enPin, LOW);
      digitalWrite(dirPin, HIGH);
      stepMotor();
      break;

    case 3: // Quay ngược
      Serial.println("Xuong");
      digitalWrite(enPin, LOW);
      digitalWrite(dirPin, LOW);
      stepMotor();
      break;
  }
  Serial.println("");
}

void stepMotor() {
  digitalWrite(stepPin, HIGH);
  delayMicroseconds(1000);
  digitalWrite(stepPin, LOW);
  delayMicroseconds(1000);
}

