#include<WiFi.h>
#include<PubSubClient.h>
#include<ESP32Servo.h>

#define Relay1_PIN 17
#define Relay2_PIN 16
#define Servo_PIN 32
Servo servo;
const char* ssid ="UTC_A2";
const char* password = "";
const char* mqtt_server ="10.90.110.183";

WiFiClient espClient;
PubSubClient client(espClient);

void setup_wifi(){
  Serial.print("Connecting to WiFi... ");
  WiFi.begin(ssid, password);
  int retry = 0;
  while(WiFi.status() != WL_CONNECTED && retry <20){
    delay(500);
    Serial.print(".");
    retry++;
  }
  if(WiFi.status() == WL_CONNECTED){
    Serial.println("\nWiFi connected! ");

  }
  else {
    Serial.println("\n Failed to connect to WiFi");
  }
}

void callback(char* topic, byte* payload, unsigned int length){
  Serial.print("Message received on topic: ");
  Serial.println(topic);

  String message ="";
  for(unsigned int i=0; i<length; i++){
    message += (char)payload[i];
  }
  message.trim();
  message.toLowerCase();

  Serial.print("Payload: ");
  Serial.println(message);

  if(strcmp(topic,"relay1/control") == 0){
    if(message == "true"){
      Serial.println("Sang den");
      digitalWrite(Relay1_PIN, HIGH);
      client.publish("relay1/status", "true");
      
    } else if(message == "false"){
      Serial.println("Tat den");
      digitalWrite(Relay1_PIN, LOW);
      client.publish("relay1/status", "false");
    }
  }

  else if(strcmp(topic, "relay2/control") == 0){
    if(message == "true"){
      Serial.println("Den 2 sang");
      digitalWrite(Relay2_PIN, HIGH);
      client.publish("relay2/status", "true");
    }
    else if(message == "false"){
      Serial.println("Den 2 tat");
      digitalWrite(Relay2_PIN, LOW);
      client.publish("relay2/status", "false");

    }
  }
// dieu khien servo
  else if(strcmp(topic, "servo/angle") == 0){
    if(message == "on"){
      servo.write(180);
      Serial.println("Servo quay 180 (mo)");
      client.publish("servo/status","on");
    }
    else if(message == "off"){
      servo.write(0);
      Serial.println("Servo quay 0 (dong)");
      client.publish("servo/status", "off");
    }
  }
} 

void reconnect(){
  static unsigned long lastRetry = 0;
  if (millis() - lastRetry < 5000) return;

  Serial.print("Connecting to MQTT...");
  if (client.connect("ESP32_Client")) {
    Serial.println("Connected!");
    client.subscribe("relay1/control");
    client.subscribe("relay2/control");
    client.subscribe("servo/angle");  // Sub topic servo
  } else {
    Serial.print("Failed, rc=");
    Serial.print(client.state());
    Serial.println(" retrying in 5 seconds...");
  }
  lastRetry = millis();
}
void setup() {
  // put your setup code here, to run once:
Serial.begin(115200);
  
  pinMode(Relay1_PIN, OUTPUT);
  pinMode(Relay2_PIN, OUTPUT);
  digitalWrite(Relay1_PIN, LOW);
  digitalWrite(Relay2_PIN, LOW);
  // Servo
  servo.attach(Servo_PIN);
  servo.write(0);

  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void loop() {
  // put your main code here, to run repeatedly:
if(WiFi.status() != WL_CONNECTED){
  Serial.println("WiFI lost , reconnecting... ");
  setup_wifi();

}
if(!client.connected()){
  reconnect();
}
client.loop();
}
