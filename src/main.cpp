#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>

// WiFi credentials
const char* ssid = "*******";
const char* password = "********";

// MQTT Broker settings (use your Windows IP address)
const char* mqtt_server = "192.100.100.145"; // CHANGE THIS to your PC's IP

// Define pins
#define DHTPIN 4
#define LED_PIN 5

// Define sensor type
#define DHTTYPE DHT22

// Initialize components
WiFiClient espClient;
PubSubClient client(espClient);
DHT dht(DHTPIN, DHTTYPE);

// Variables
unsigned long lastMsg = 0;
#define MSG_DELAY 5000 // Send data every 5 seconds

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
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  
  String messageTemp;
  for (int i = 0; i < length; i++) {
    messageTemp += (char)payload[i];
  }
  messageTemp.trim(); // Remove any trailing whitespace
  Serial.println(messageTemp);

  // Check if this message is for LED control
  if (String(topic) == "home/actuators/led1") {
    if (messageTemp == "on") {
      digitalWrite(LED_PIN, HIGH);
      Serial.println("LED turned ON");
    } else if (messageTemp == "off") {
      digitalWrite(LED_PIN, LOW);
      Serial.println("LED turned OFF");
    }
  }
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    
    if (client.connect("ESP32Client")) {
      Serial.println("connected");
      // Subscribe to the LED control topic
      client.subscribe("home/actuators/led1");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void setup() {
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW); // Start with LED off
  
  Serial.begin(115200);
  dht.begin();
  
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  unsigned long now = millis();
  if (now - lastMsg > MSG_DELAY) {
    lastMsg = now;
    
    // Read temperature and humidity
    float temperature = dht.readTemperature();
    float humidity = dht.readHumidity();

    // Check if any reads failed
    if (isnan(temperature) || isnan(humidity)) {
      Serial.println("Failed to read from DHT sensor!");
      return;
    }

    // Create JSON payload
    String payload = "{";
    payload += "\"temperature\":"; payload += temperature;
    payload += ",\"humidity\":"; payload += humidity;
    payload += "}";

    // Publish to MQTT
    client.publish("home/sensors/temperature", payload.c_str());
    Serial.println("Data published: " + payload);
  }
}