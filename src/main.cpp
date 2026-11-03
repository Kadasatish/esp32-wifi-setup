#include <Arduino.h>
#include <WiFi.h>
#include <Preferences.h>

Preferences preferences;
String serialLine;

static void printHelp() {
  Serial.println();
  Serial.println("ESP32 Wi-Fi Station Setup");
  Serial.println("Send: WIFI,<SSID>,<PASSWORD>");
  Serial.println("Example: WIFI,MyHotspot,myPassword123");
  Serial.println("Send CLEAR to erase saved Wi-Fi credentials.");
  Serial.println();
}

static void connectToSavedWiFi() {
  preferences.begin("wifi-config", true);
  String ssid = preferences.getString("ssid", "");
  String password = preferences.getString("password", "");
  preferences.end();

  if (ssid.isEmpty()) {
    Serial.println("No Wi-Fi credentials saved.");
    printHelp();
    return;
  }

  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.begin(ssid.c_str(), password.c_str());

  Serial.print("Connecting to Wi-Fi: ");
  Serial.println(ssid);

  unsigned long startedAt = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startedAt < 20000) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("WIFI_CONNECTED");
    Serial.print("SSID: ");
    Serial.println(WiFi.SSID());
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
    Serial.print("RSSI: ");
    Serial.println(WiFi.RSSI());
  } else {
    Serial.print("Wi-Fi connection failed. Status: ");
    Serial.println(static_cast<int>(WiFi.status()));
    Serial.println("Check hotspot is ON, 2.4 GHz, and password is correct.");
    printHelp();
  }
}

static void handleCommand(String command) {
  command.trim();

  if (command.equalsIgnoreCase("HELP")) {
    printHelp();
    return;
  }

  if (command.equalsIgnoreCase("CLEAR")) {
    preferences.begin("wifi-config", false);
    preferences.clear();
    preferences.end();
    WiFi.disconnect(true, true);
    WiFi.mode(WIFI_STA);
    Serial.println("Saved Wi-Fi credentials cleared.");
    printHelp();
    return;
  }

  if (command.startsWith("WIFI,")) {
    int firstComma = command.indexOf(',');
    int secondComma = command.indexOf(',', firstComma + 1);
    if (secondComma < 0) {
      Serial.println("Invalid format. Use: WIFI,<SSID>,<PASSWORD>");
      return;
    }

    String ssid = command.substring(firstComma + 1, secondComma);
    String password = command.substring(secondComma + 1);
    ssid.trim();

    if (ssid.isEmpty() || password.length() < 8) {
      Serial.println("SSID is required and WPA2 password must be at least 8 characters.");
      return;
    }

    preferences.begin("wifi-config", false);
    preferences.putString("ssid", ssid);
    preferences.putString("password", password);
    preferences.end();

    Serial.println("Wi-Fi credentials saved in ESP32 NVS.");
    connectToSavedWiFi();
    return;
  }

  Serial.println("Unknown command.");
  printHelp();
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println();
  Serial.println("ESP32 Station Mode Firmware");
  Serial.println("Boot OK. Wi-Fi mode: STA (Station).");
  WiFi.mode(WIFI_STA);
  connectToSavedWiFi();
}

void loop() {
  while (Serial.available()) {
    char c = static_cast<char>(Serial.read());
    if (c == '\\n' || c == '\\r') {
      if (!serialLine.isEmpty()) {
        handleCommand(serialLine);
        serialLine = "";
      }
    } else if (serialLine.length() < 256) {
      serialLine += c;
    }
  }

  if (WiFi.status() == WL_CONNECTED) {
    static unsigned long lastReport = 0;
    if (millis() - lastReport >= 10000) {
      lastReport = millis();
      Serial.print("Wi-Fi OK | IP: ");
      Serial.println(WiFi.localIP());
    }
  }
  delay(10);
}
