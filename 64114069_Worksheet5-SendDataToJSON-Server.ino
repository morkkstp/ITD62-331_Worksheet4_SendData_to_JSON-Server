#include <ESP8266WiFi.h>
#include <DHT.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <time.h>
#include <WiFiUdp.h>
#include <NTPClient.h>

const char* ssid = "IR_Lab";
const char* password = "ccsadmin";

static unsigned long lastTime = 0;
unsigned long timerDelay = 5000;

float temperature;
float humidity;
DHT dht11(D4, DHT11);

WiFiClient client;
HTTPClient http;

// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected.");

  // setup DHT11
  temperature = 0.0;
  humidity = 0.0;
  dht11.begin();

  // Initialize a NTPClient to get time
  timeClient.begin();

  // Set offset time in seconds to adjust for your timezone: GMT +7 = 25200
  timeClient.setTimeOffset(25200);
}

void loop() {
  if ((millis() - lastTime) > timerDelay) {
    // Get Value on DHT11
    humidity = (dht11.readHumidity());
    temperature = (dht11.readTemperature());

    if (humidity && temperature) {
      Serial.println("Humidity: " + String(humidity));
      Serial.println("Temperature: " + String(temperature));

      //Get a timestamp
      timeClient.update();
      time_t epochTime = timeClient.getEpochTime();
      String formattedTime = timeClient.getFormattedTime();
      struct tm* ptm = gmtime((time_t*)&epochTime);
      int currentDay = ptm->tm_mday;
      int currentMonth = ptm->tm_mon + 1;
      int currentYear = ptm->tm_year + 1900;
      String currentDate = String(currentDay) + "-" + String(currentMonth) + "-" + String(currentYear) + " " + String(formattedTime);
      Serial.println("Current date: " + String(currentDate));

      // Create JSON object
      StaticJsonDocument<200> jsonDocument;
      jsonDocument["humidity"] = humidity;
      jsonDocument["temperature"] = temperature;
      jsonDocument["timestamp"] = currentDate;
      jsonDocument["time_counter"] = millis();

      // Serialize JSON data to String
      String jsonData;
      serializeJson(jsonDocument, jsonData);

      // Send HTTP POST request
      http.begin(client, "http://192.168.0.169:3000/dht11_value");
      http.addHeader("Content-Type", "application/json");
      int httpResponseCode = http.POST(jsonData);

      if (httpResponseCode > 0) {
        Serial.println("HTTP Response code: " + String(httpResponseCode));
        String payload = http.getString();
        Serial.println("Payload Data:");
        Serial.println(payload);
      } else {
        Serial.println("Failed to sending code: " + String(httpResponseCode));
      }
      http.end();
    } else {
      Serial.println("Failed to read from DHT11 sensor");
    }
    lastTime = millis();
  }
}
