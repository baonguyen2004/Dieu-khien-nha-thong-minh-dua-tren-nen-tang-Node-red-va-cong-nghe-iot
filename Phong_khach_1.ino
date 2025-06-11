#include<DHT.h>
#include<ESP32Servo.h>
#include<Wire.h>
#include<BH1750.h>
#include<Adafruit_SSD1306.h>
#include<Adafruit_GFX.h>
#include <PubSubClient.h>
#include<WiFi.h>

const char* ssid = "UTC_A2";       // Thay bằng WiFi của bạn
const char* password = "";
const char* mqttServer = "10.90.110.183";  // Địa chỉ MQTT Broker (thay bằng IP của Broker)
const int mqttPort = 1883;
const char* tempTopic = "temperature1";  // Topic nhiệt độ
const char* humTopic = "humidity1";  // Topic độ ẩm
const char* lightTopic = "light1";  // Topic khí LPG (ppm)

WiFiClient espClient;
PubSubClient client(espClient);
// OLED
#define CHIEU_DAI 128
#define CHIEU_CAO  64

Adafruit_SSD1306 display(CHIEU_DAI, CHIEU_CAO, &Wire, -1);


// BH1750
BH1750 Lightmeter;

// Servo
const uint8_t Button_1 =12;
const uint8_t Button_2 = 13;

const uint8_t Servo_PIN = 26;
Servo servo;

// Relay+ SR505
const uint8_t SR505_PIN = 25;
const uint8_t Relay1 = 14;
const uint8_t Relay2 = 27;


// DHT11 
const uint8_t DHTPIN = 33;
const uint8_t DHTTYPE = DHT11;

DHT dht(DHTPIN, DHTTYPE);
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

  if(strcmp(topic, "servo1/angle") == 0){
    if(message == "on"){
      servo.write(180);
      Serial.println("Servo quay 180 (mo)");
      client.publish("servo1/status","on");
    }
    else if(message == "off"){
      servo.write(0);
      Serial.println("Servo quay 0 (dong)");
      client.publish("servo1/status", "off");
    }
  }
} 
void Dieu_khien_LED(){
  if(digitalRead(SR505_PIN) == 1){
    digitalWrite(Relay1, HIGH);
    digitalWrite(Relay2, HIGH);
    
  }
  else {
    digitalWrite(Relay1, LOW);
    digitalWrite(Relay2, LOW);

  }
}

void Dieu_khien_rem(){
  if(digitalRead(Button_1) ==LOW && digitalRead(Button_2) == HIGH){
    Serial.println("Mo rem");
    servo.write(180);
  }

  else if(digitalRead(Button_2) == LOW && digitalRead(Button_1 ) == HIGH){
    Serial.println("Dong rem");
    servo.write(0);
  }
}
void setup() {
  // put your setup code here, to run once:
Wire.begin();
dht.begin();
Serial.begin(115200);
WiFi.begin(ssid, password);
while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Đang kết nối WiFi...");
}

Serial.println("Kết nối WiFi thành công!");

client.setServer(mqttServer, mqttPort);
reconnect();
client.setCallback(callback);

// SR505 + Relay
pinMode(Relay1, OUTPUT);
pinMode(Relay2, OUTPUT);
pinMode(SR505_PIN, INPUT);
// Servo
pinMode(Button_1, INPUT_PULLUP);
pinMode(Button_2, INPUT_PULLUP);
servo.attach(Servo_PIN);
// BH1750
Lightmeter.begin();
// OLED
display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
display.clearDisplay();
display.setTextSize(1);
display.setTextColor(WHITE);

}
unsigned long Current = millis();
unsigned long Previous = 0;
unsigned long interval = 2000;


void loop() {
  // put your main code here, to run repeatedly:
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  
  float Nhiet_do = dht.readTemperature();
  float Do_am = dht.readHumidity();
  float Anh_sang = Lightmeter.readLightLevel();

  Serial.print("Gia tri SR505: ");
  Serial.println(digitalRead(SR505_PIN));
  // Hien thi tren OLED
  display.clearDisplay();
  display.setCursor(0, 10);
  display.print("Nhiet do: ");
  display.print(Nhiet_do);
  display.print(char(247));
  display.println("C");

  display.setCursor(0,20);
  display.print("Do am: ");
  display.print(Do_am);
  display.println("%");

  display.setCursor(0,30);
  display.print("Anh sang: ");
  display.print(Anh_sang);
  display.println("lux");
  
  display.display();
  
  delay(1000);

char tempMsg[10], humMsg[10], lightMsg[10];
dtostrf( Nhiet_do, 4, 2, tempMsg);
dtostrf( Do_am, 4, 2, humMsg);
dtostrf(Anh_sang, 6, 2, lightMsg);

client.publish(tempTopic, tempMsg);delay(2000);
client.publish(humTopic, humMsg);delay(2000);
client.publish(lightTopic, lightMsg);delay(2000);

Serial.print("Gửi nhiệt độ: "); Serial.println(tempMsg);
Serial.print("Gửi độ ẩm: "); Serial.println(humMsg);
Serial.print("Gửi Ánh sáng: "); Serial.println(lightMsg);

delay(2000);
Dieu_khien_LED();
Dieu_khien_rem();

}
void reconnect(){
  static unsigned long lastRetry = 0;
  if (millis() - lastRetry < 5000) return;

  Serial.print("Connecting to MQTT...");
  if (client.connect("ESP32_Client")) {
    Serial.println("Connected!");
    client.subscribe("servo1/angle");  // Sub topic servo
  } else {
    Serial.print("Failed, rc=");
    Serial.print(client.state());
    Serial.println(" retrying in 5 seconds...");
  }
  lastRetry = millis();
}

