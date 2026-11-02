#include <Arduino.h>
#include <WiFi.h>

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_AP);
  const bool started = WiFi.softAP("ESP32-SETUP", "12345678");
  if (started) {
    Serial.println("Wi-Fi AP started");
    Serial.print("SSID: ");
    Serial.println("ESP32-SETUP");
    Serial.print("IP: ");
    Serial.println(WiFi.softAPIP());
  } else {
    Serial.println("Wi-Fi AP start failed");
  }
}

void loop() {
  delay(1000);
}
