#include <Arduino.h>
#include <WiFi.h>
#include <Preferences.h>

Preferences preferences;

static const char *AP_SSID = "ESP32-Vending-Setup";
static const char *AP_PASSWORD = "VendingSetup123"; // Change before public deployment.
static const uint8_t AP_CHANNEL = 6;
static const uint8_t AP_MAX_CLIENTS = 4;

static String serialLine;
static unsigned long lastReconnectAttempt = 0;
static unsigned long lastStatusReport = 0;
static bool staCredentialsConfigured = false;

static void printHelp() {
  Serial.println();
  Serial.println("ESP32 AP + Station Mode");
  Serial.println("AP Wi-Fi: ESP32-Vending-Setup");
  Serial.println("AP password: VendingSetup123");
  Serial.println("AP address: http://192.168.4.1 (web page not added yet)");
  Serial.println();
  Serial.println("Serial commands (115200 baud):");
  Serial.println("  WIFI,<SSID>,<PASSWORD>  Save router/hotspot credentials");
  Serial.println("  CLEAR                   Erase saved Station credentials");
  Serial.println("  STATUS                  Show AP and Station status");
  Serial.println("  HELP                    Show this help");
  Serial.println();
}

static void startAccessPoint() {
  // AP and STA share the ESP32 radio. The AP remains available while STA
  // connects; its radio channel may follow the connected router's channel.
  if (!WiFi.softAP(AP_SSID, AP_PASSWORD, AP_CHANNEL, false, AP_MAX_CLIENTS)) {
    Serial.println("ERROR: Could not start ESP32 Access Point.");
    return;
  }

  Serial.println("AP_READY");
  Serial.print("AP SSID: ");
  Serial.println(AP_SSID);
  Serial.print("AP IP: ");
  Serial.println(WiFi.softAPIP());
  Serial.println("Connect the phone to this Wi-Fi to use the local network.");
}

static void loadAndConnectStation() {
  preferences.begin("wifi-config", true);
  String ssid = preferences.getString("ssid", "");
  String password = preferences.getString("password", "");
  preferences.end();

  staCredentialsConfigured = !ssid.isEmpty();
  if (!staCredentialsConfigured) {
    Serial.println("No Station credentials saved. AP mode is ready.");
    return;
  }

  WiFi.begin(ssid.c_str(), password.c_str());
  lastReconnectAttempt = millis();
  Serial.print("Station connecting to saved Wi-Fi: ");
  Serial.println(ssid);
}

static void showStatus() {
  Serial.println();
  Serial.println("----- ESP32 NETWORK STATUS -----");
  Serial.print("AP: ");
  Serial.println(WiFi.softAPgetStationNum() >= 0 ? "enabled" : "unknown");
  Serial.print("AP SSID: ");
  Serial.println(AP_SSID);
  Serial.print("AP IP: ");
  Serial.println(WiFi.softAPIP());
  Serial.print("AP clients: ");
  Serial.println(WiFi.softAPgetStationNum());

  Serial.print("STA status: ");
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("CONNECTED");
    Serial.print("STA SSID: ");
    Serial.println(WiFi.SSID());
    Serial.print("STA IP: ");
    Serial.println(WiFi.localIP());
    Serial.print("RSSI: ");
    Serial.println(WiFi.RSSI());
  } else {
    Serial.println(staCredentialsConfigured ? "NOT CONNECTED" : "NOT CONFIGURED");
  }
  Serial.println("--------------------------------");
}

static void handleCommand(String command) {
  command.trim();
  if (command.isEmpty()) return;

  if (command.equalsIgnoreCase("HELP")) {
    printHelp();
    return;
  }

  if (command.equalsIgnoreCase("STATUS")) {
    showStatus();
    return;
  }

  if (command.equalsIgnoreCase("CLEAR")) {
    preferences.begin("wifi-config", false);
    preferences.clear();
    preferences.end();

    WiFi.disconnect(false, true);
    staCredentialsConfigured = false;
    Serial.println("Saved Station credentials cleared. ESP32 AP remains active.");
    return;
  }

  if (command.startsWith("WIFI,")) {
    const int firstComma = command.indexOf(',');
    const int secondComma = command.indexOf(',', firstComma + 1);

    if (secondComma < 0) {
      Serial.println("ERROR: Use WIFI,<SSID>,<PASSWORD>");
      return;
    }

    String ssid = command.substring(firstComma + 1, secondComma);
    String password = command.substring(secondComma + 1);
    ssid.trim();

    if (ssid.isEmpty() || ssid.length() > 32) {
      Serial.println("ERROR: SSID must contain 1 to 32 characters.");
      return;
    }
    if (!password.isEmpty() && (password.length() < 8 || password.length() > 63)) {
      Serial.println("ERROR: Password must be empty for an open network or 8 to 63 characters.");
      return;
    }

    preferences.begin("wifi-config", false);
    preferences.putString("ssid", ssid);
    preferences.putString("password", password);
    preferences.end();

    staCredentialsConfigured = true;
    WiFi.disconnect(false, false);
    WiFi.begin(ssid.c_str(), password.c_str());
    lastReconnectAttempt = millis();

    Serial.println("Station credentials saved. Connecting...");
    Serial.println("ESP32 AP remains active during Station connection.");
    return;
  }

  Serial.println("ERROR: Unknown command. Send HELP.");
}

static void readSerialCommands() {
  while (Serial.available() > 0) {
    const char c = static_cast<char>(Serial.read());

    // Correctly handle CR, LF, and CRLF serial monitor line endings.
    if (c == '\n' || c == '\r') {
      if (!serialLine.isEmpty()) {
        handleCommand(serialLine);
        serialLine = "";
      }
    } else if (serialLine.length() < 256) {
      serialLine += c;
    } else {
      serialLine = "";
      Serial.println("ERROR: Command too long; line discarded.");
    }
  }
}

static void maintainStationConnection() {
  if (!staCredentialsConfigured || WiFi.status() == WL_CONNECTED) return;

  const unsigned long now = millis();
  if (now - lastReconnectAttempt >= 15000) {
    lastReconnectAttempt = now;
    Serial.println("Station disconnected; retrying saved Wi-Fi...");
    WiFi.reconnect();
  }
}

void setup() {
  Serial.begin(115200);
  delay(500);

  Serial.println();
  Serial.println("ESP32-WROOM AP + STA Firmware");
  WiFi.persistent(false);
  WiFi.setAutoReconnect(true);
  WiFi.mode(WIFI_AP_STA);

  startAccessPoint();
  loadAndConnectStation();
  printHelp();
}

void loop() {
  readSerialCommands();
  maintainStationConnection();

  if (millis() - lastStatusReport >= 10000) {
    lastStatusReport = millis();
    if (WiFi.status() == WL_CONNECTED) {
      Serial.print("STA_CONNECTED | IP: ");
      Serial.println(WiFi.localIP());
    }
  }

  delay(5);
}
