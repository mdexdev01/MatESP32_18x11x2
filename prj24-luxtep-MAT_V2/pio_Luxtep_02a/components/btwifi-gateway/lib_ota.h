#ifndef _LIB_OTA_H_
#define _LIB_OTA_H_

#include "lib_gpio.h"
#include "libPrintRaw.h"
#include "lib_wifiConfig.h"

WebServer server(80);


bool shouldDoOTA = false;
bool inSoftAPMode = false;

bool isOTACommand = false;

void setup_ota();
void loop_ota();

void startSoftAP() {
  inSoftAPMode = true;
  WiFi.softAP(MY_SOFTAP_SSID);
  IPAddress IP = WiFi.softAPIP();
  uart0_printf("SoftAP IP: %s\n", IP.toString().c_str());
  // Serial.println(IP);

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

    gPrefrences.begin("wifi", false);
    gPrefrences.putString("ssid", ssid);
    gPrefrences.putString("password", password);
    gPrefrences.end();

    server.send(200, "text/html", "Saved. Rebooting...");
    delay(1500);
    ESP.restart();
  });

  server.begin();
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
          setNeopixelColor(pin_LED_WS2812C, 0, 100, 0);
          delay(1000);
          uart0_printf("OTA Success. Rebooting... \n");
          ESP.restart();
        } else {
          setNeopixelColor(pin_LED_WS2812C, 100, 0, 0);
          uart0_printf("OTA Incomplete. \n");
        }
      } else {
        setNeopixelColor(pin_LED_WS2812C, 100, 0, 0);
        uart0_printf("OTA Error: %s\n", Update.errorString());
      }
    } else {
      setNeopixelColor(pin_LED_WS2812C, 100, 0, 0);
      uart0_printf("Not enough space for OTA. \n");
    }
  } else {
    setNeopixelColor(pin_LED_WS2812C, 100, 0, 0);
    uart0_printf("HTTP Error: %d\n", httpCode);
  }
  http.end();
}

void setup_ota() {
  // gPrefrences.begin("wifi", false);
  // gPrefrences.clear();
  // gPrefrences.end();

  uart0_printf("Starting... Ver.0.1 info \n");
}

void loop_ota() {
  if (!shouldDoOTA && isOTACommand == true) {
    shouldDoOTA = true;
    uart0_printf("Button pressed. <0.1> Checking Wi-Fi... \n");

    setNeopixelColor(pin_LED_WS2812C, 0, 0, 100);
    if (connectToWiFi()) {
      uart0_printf("Wi-Fi connected. Starting OTA... \n");
      doOTA();
    } else {
      setNeopixelColor(pin_LED_WS2812C, 100, 0, 100);
      uart0_printf("Wi-Fi failed. Starting SoftAP... \n");
      startSoftAP();
    }
  }

  if (inSoftAPMode) {
    server.handleClient();
  }
}

#endif  // _LIB_OTA_H_
