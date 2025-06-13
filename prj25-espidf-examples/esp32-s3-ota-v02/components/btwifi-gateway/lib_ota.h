
Preferences prefs;
WebServer server(80);

const char* ap_ssid = "LUXTEP_mat_main";

bool shouldDoOTA = false;
bool inSoftAPMode = false;

bool isOTACommand = false;

void setup_ota();
void loop_ota();

void startSoftAP() {
  inSoftAPMode = true;
  WiFi.softAP(ap_ssid);
  IPAddress IP = WiFi.softAPIP();
  Serial.print("SoftAP IP: ");
  Serial.println(IP);

  server.on("/", HTTP_GET, []() {
    server.send(200, "text/html", R"rawliteral(
      <form action="/save" method="POST">
        SSID: <input name="ssid"><br>
        Password: <input name="password"><br>
        <input type="submit">
      </form>
    )rawliteral");
  });

  server.on("/save", HTTP_POST, []() {
    String ssid = server.arg("ssid");
    String password = server.arg("password");

    prefs.begin("wifi", false);
    prefs.putString("ssid", ssid);
    prefs.putString("password", password);
    prefs.end();

    server.send(200, "text/html", "Saved. Rebooting...");
    delay(1500);
    ESP.restart();
  });

  server.begin();
}

bool connectToWiFi() {
  prefs.begin("wifi", true);
  String ssid = prefs.getString("ssid", "");
  String password = prefs.getString("password", "");
  prefs.end();

  if (ssid == "") return false;

  WiFi.begin(ssid.c_str(), password.c_str());

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 10000) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  return WiFi.status() == WL_CONNECTED;
}

void doOTA() {
  HTTPClient http;
  http.begin("https://luxtep-ota-v02.web.app/firmware.bin");
  int httpCode = http.GET();

  if (httpCode == 200) {
    int contentLength = http.getSize();
    bool canBegin = Update.begin(contentLength);

    if (canBegin) {
      WiFiClient* stream = http.getStreamPtr();
      size_t written = Update.writeStream(*stream);
      if (Update.end()) {
        if (Update.isFinished()) {
          Serial.println("OTA Success. Rebooting...");
          ESP.restart();
        } else {
          Serial.println("OTA Incomplete.");
        }
      } else {
        Serial.printf("OTA Error: %s\n", Update.errorString());
      }
    } else {
      Serial.println("Not enough space for OTA.");
    }
  } else {
    Serial.printf("HTTP Error: %d\n", httpCode);
  }
  http.end();
}

void setup_ota() {
  // prefs.begin("wifi", false);
  // prefs.clear();
  // prefs.end();

  Serial.println("Starting... Ver.0.1 info");
}

void loop_ota() {
  if (!shouldDoOTA && isOTACommand == true) {
    shouldDoOTA = true;
    Serial.println("Button pressed. <0.1> Checking Wi-Fi...");

    if (connectToWiFi()) {
      Serial.println("Wi-Fi connected. Starting OTA...");
      doOTA();
    } else {
      Serial.println("Wi-Fi failed. Starting SoftAP...");
      startSoftAP();
    }
  }

  if (inSoftAPMode) {
    server.handleClient();
  }
}
