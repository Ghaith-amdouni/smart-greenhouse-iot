#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>

// Pins
#define DHTPIN 4
#define DHTTYPE DHT22
//#define SOIL_PIN 34
#define LIGHT_PIN 34
#define RELAY_PIN 26
#define RAIN_PIN 25
#define CO2_PIN 32

// LEDs pour indicateurs
#define LED_FAN        13
#define LED_LIGHT      14
#define LED_VENTILATION 27

// WiFi credentials
const char* ssid = "Wokwi-GUEST";
const char* password = ""; // pas de mot de passe

// MQTT broker
const char* mqtt_server = "broker.hivemq.com";
const int mqtt_port = 1883;

// MQTT Topics
const char* TOPIC_TEMPERATURE   = "serre/temperature";
const char* TOPIC_HUMIDITY      = "serre/humidity";
//const char* TOPIC_SOIL          = "serre/soilMoisture";
const char* TOPIC_LIGHT         = "serre/light";
const char* TOPIC_IRRIGATION    = "serre/irrigation";
const char* TOPIC_CO2           = "serre/co2";
const char* TOPIC_RAIN          = "serre/rain";
const char* TOPIC_FAN           = "serre/fan";
const char* TOPIC_LIGHT_CONTROL = "serre/lightControl";
const char* TOPIC_VENTILATION   = "serre/ventilation";

WiFiClient espClient;
PubSubClient client(espClient);
DHT dht(DHTPIN, DHTTYPE);

void callback(char* topic, byte* payload, unsigned int length) {
  String message;
  for (unsigned int i = 0; i < length; i++) message += (char)payload[i];

  if (String(topic) == TOPIC_IRRIGATION) {
    if (message == "1") digitalWrite(RELAY_PIN, HIGH);
    else if (message == "0") digitalWrite(RELAY_PIN, LOW);
    else digitalWrite(RELAY_PIN, LOW);
  }
}

void setup_wifi() {
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi connected");
}

void reconnect() {
  while (!client.connected()) {
    if (client.connect("ESP32_Wokwi")) client.subscribe(TOPIC_IRRIGATION);
    else delay(2000);
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(RAIN_PIN, INPUT);
  
  // LEDs
  pinMode(LED_FAN, OUTPUT);
  pinMode(LED_LIGHT, OUTPUT);
  pinMode(LED_VENTILATION, OUTPUT);

  dht.begin();
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) reconnect();
  client.loop();

  // Lire capteurs
  float temp = dht.readTemperature();
  float hum = dht.readHumidity();
  //int soil = analogRead(SOIL_PIN);
  int light = analogRead(LIGHT_PIN);
  int co2 = analogRead(CO2_PIN);
  int rain = digitalRead(RAIN_PIN);

  // Publier sur MQTT
  client.publish(TOPIC_TEMPERATURE, String(temp).c_str());
  client.publish(TOPIC_HUMIDITY, String(hum).c_str());
 // client.publish(TOPIC_SOIL, String(soil).c_str());
  client.publish(TOPIC_LIGHT, String(light).c_str());
  client.publish(TOPIC_CO2, String(co2).c_str());
  client.publish(TOPIC_RAIN, String(rain).c_str());

  // ------------------------
  // Automatisation
  // ------------------------

  // Irrigation automatique
  if (rain == 0 && hum < 50) {   // humidité < 50% et pas de pluie
    digitalWrite(RELAY_PIN, HIGH);
    client.publish(TOPIC_IRRIGATION, "1");
  } else {
    digitalWrite(RELAY_PIN, LOW);
    client.publish(TOPIC_IRRIGATION, "0");
  }

  // Ventilateur selon température
  if (temp > 30) {
    client.publish(TOPIC_FAN, "1");
    digitalWrite(LED_FAN, HIGH);
  } else {
    client.publish(TOPIC_FAN, "0");
    digitalWrite(LED_FAN, LOW);
  }

  // Lumière selon luminosité
  if (light < 1000) {
    client.publish(TOPIC_LIGHT_CONTROL, "1");
    digitalWrite(LED_LIGHT, HIGH);
  } else {
    client.publish(TOPIC_LIGHT_CONTROL, "0");
    digitalWrite(LED_LIGHT, LOW);
  }

  // Ventilation selon CO2
  if (co2 > 2000) {
    client.publish(TOPIC_VENTILATION, "1");
    digitalWrite(LED_VENTILATION, HIGH);
  } else {
    client.publish(TOPIC_VENTILATION, "0");
    digitalWrite(LED_VENTILATION, LOW);
  }

  delay(5000); // toutes les 5 secondes
}
