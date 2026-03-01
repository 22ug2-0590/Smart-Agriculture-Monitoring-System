#include <WiFi.h>
#include <HTTPClient.h>
#include <DHT.h>
#include <LiquidCrystal_I2C.h>

#include "secrets.h" 

// import macros from secrets.h
const char* ssid = SECRET_SSID;
const char* password = SECRET_PASS;
String apiKey = SECRET_API_KEY;


// Pins
#define DHTPIN 15
#define DHTTYPE DHT22
#define MOISTURE_PIN 34
#define PUMP_PIN 5

// Variables
DHT dht(DHTPIN, DHTTYPE);
LiquidCrystal_I2C lcd(0x27, 16, 2);
const int DRY_SOIL_THRESHOLD = 40; 
const char* server = "http://api.thingspeak.com/update";

void setup() {
  Serial.begin(115200);
  
  // Initialize Hardware
  dht.begin();
  pinMode(PUMP_PIN, OUTPUT);
  lcd.init();
  lcd.backlight();

  // Connect to WiFi
  lcd.setCursor(0, 0);
  lcd.print("Connecting WiFi");
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  lcd.clear();
  lcd.print("WiFi Connected!");
  Serial.println("\nWiFi connected.");
  delay(1000);
  lcd.clear();
}

void loop() {
  // Read Sensors
  float temp = dht.readTemperature();
  float hum = dht.readHumidity();
  int rawMoisture = analogRead(MOISTURE_PIN);
  int moisturePercent = map(rawMoisture, 0, 4095, 0, 100);

  // Check for sensor errors
  if (isnan(temp) || isnan(hum)) {
    Serial.println("Sensor Read Error!");
    return;
  }

  // Logic Control
  int pumpState = 0; // 0 = OFF, 1 = ON
  if (moisturePercent < DRY_SOIL_THRESHOLD) {
    digitalWrite(PUMP_PIN, HIGH);
    pumpState = 1;
  } else {
    digitalWrite(PUMP_PIN, LOW);
    pumpState = 0;
  }

  // Display on LCD
  lcd.setCursor(0, 0);
  lcd.printf("T:%.1f H:%.0f%%", temp, hum);
  lcd.setCursor(0, 1);
  lcd.printf("Soil:%d%% P:%s", moisturePercent, (pumpState ? "ON" : "OFF"));

  // Upload to ThingSpeak
  if(WiFi.status() == WL_CONNECTED){
    HTTPClient http;
    
    String url = String(server) + "?api_key=" + apiKey 
                 + "&field1=" + String(temp) 
                 + "&field2=" + String(hum) 
                 + "&field3=" + String(moisturePercent)
                 + "&field4=" + String(pumpState);
                 
    http.begin(url);
    int httpCode = http.GET(); // Send request
    
    if (httpCode > 0) {
      Serial.printf("ThingSpeak Update: %d (Success)\n", httpCode);
    } else {
      Serial.printf("ThingSpeak Error: %s\n", http.errorToString(httpCode).c_str());
    }
    http.end();
  }

  delay(16000);  // 16s
}