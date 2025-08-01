// lib_softap_webserver.h
// Version: Ver.1.0
// Author: mdex.co.kr & Refactored by AI
// Date: 2025-07-22
// 역할: SoftAP 기반의 설정 모드를 총괄. WiFi 설정 및 OTA 진입 UI 제공.

#ifndef _LIB_SOFTAP_WEBSERVER_H_
#define _LIB_SOFTAP_WEBSERVER_H_

#include <WebServer.h>
#include <Preferences.h>
#include "lib_nvs.h"
#include "lib_otaDirect.h"
#include "libPrintRaw.h"

// --- 전역 객체 및 변수 ---
WebServer server(80);
bool shouldEnterSoftAP = false;
extern bool otaUpdateRequested; // network_task.h에 정의될 플래그

// --- 상수 정의 ---
const char* ap_ssid = "LUXTEP_MAT_CONFIG";
const char* ap_password = "password";

// --- 웹 페이지 핸들러 ---
void handleRoot() {
    String html = R"rawliteral(
    <!DOCTYPE html>
    <html>
    <head>
      <title>ESP32 Configuration</title>
      <meta name="viewport" content="width=device-width, initial-scale=1">
      <style>
        body { font-family: Arial, sans-serif; margin: 20px; background-color: #333; color: #eee; }
        .container { background-color: #555; padding: 20px; border-radius: 8px; max-width: 400px; margin: auto; transition: opacity 0.5s; }
        input[type="text"], input[type="password"] {
          width: calc(100% - 20px); padding: 10px; margin: 8px 0; border: 1px solid #777; border-radius: 4px;
          box-sizing: border-box; background-color: #444; color: #eee;
        }
        input[type="submit"] { 
          width: 100%; background-color: #4CAF50; color: white; padding: 14px 20px; margin: 8px 0;
          border: none; border-radius: 4px; cursor: pointer; font-size: 16px; 
        }
        input[type="submit"]:hover { background-color: #45a049; }
        label { display: block; margin-bottom: 5px; color: #ccc; }
        .message { text-align: center; padding: 40px; }
        .message h2 { font-size: 22px; color: #4CAF50; }
        .message p { font-size: 14px; color: #ccc; }
      </style>
    </head>
    <body>
      <div class="container" id="main-container">
        <h2>WiFi & AppID Configuration</h2>
        <form id="config-form" action="/save" method="POST">
          <label for="ssid">WiFi SSID:</label>
          <input type="text" id="ssid" name="ssid" value="%SSID_VALUE%"><br>
          <label for="password">WiFi Password:</label>
          <input type="password" id="password" name="password" value="%PASS_VALUE%"><br>
          <label for="app_id">AppID:</label>
          <input type="text" id="app_id" name="app_id" value="%APPID_VALUE%"><br>
          <input type="submit" value="Save & Connect">
        </form>
      </div>
      <script>
        document.getElementById('config-form').addEventListener('submit', function(e) {
          e.preventDefault();
          var formData = new FormData(this);
          fetch('/save', { method: 'POST', body: formData });

          var container = document.getElementById('main-container');
          container.style.opacity = '0';
          setTimeout(function() {
            container.innerHTML = 
              '<div class="message">' +
              '<h2>Saved. Rebooting...</h2>' +
              '<p>Please disconnect from this network and connect to your regular WiFi.</p>' +
              '</div>';
            container.style.opacity = '1';
          }, 500);
        });
      </script>
    </body>
    </html>
    )rawliteral";

    prefs.begin(NVS_NAMESPACE, true);
    html.replace("%SSID_VALUE%", prefs.getString(NVS_KEY_SSID, DEFAULT_SSID));
    html.replace("%PASS_VALUE%", prefs.getString(NVS_KEY_PASS, DEFAULT_PASSWORD));
    html.replace("%APPID_VALUE%", prefs.getString(NVS_KEY_APPID, DEFAULT_APPID));
    prefs.end();

    html.replace("%OTA_BUTTON%", "");

    server.send(200, "text/html", html);
}

void handleSave() {
    prefs.begin(NVS_NAMESPACE, false);
    prefs.putString(NVS_KEY_SSID, server.arg("ssid"));
    prefs.putString(NVS_KEY_PASS, server.arg("password"));
    prefs.putString(NVS_KEY_APPID, server.arg("app_id"));
    prefs.end();

    server.send(200, "text/plain", "OK");
    delay(200); // Allow time for the response to be sent
    ESP.restart();
}

// --- 설정 모드 관리 함수 ---
void setup_softap_launch(bool isOTADirect = false) {
    uart0_printf("[%8lu ms] [Config Mode] Starting SoftAP %s...\n", millis(), ap_ssid);
    WiFi.softAP(ap_ssid, ap_password);
    IPAddress IP = WiFi.softAPIP();
    uart0_printf("[%8lu ms] [Config Mode] AP IP: %s\n", millis(), IP.toString().c_str());

    // case 1) direct ota. /update 페이지로 리디렉션
    // if (otaUpdateRequested) {
    if (isOTADirect) {
        server.on("/", HTTP_GET, [](){
            server.sendHeader("Location", "/update", true);
            server.send(302, "text/plain", "");
        });
        OTA_Direct(server); 
        uart0_printf("[%8lu ms] [Config Mode] Direct OTA enabled. Navigate to http://%s/update\n", millis(), IP.toString().c_str());
    } 
    // case 2) wifi ssid 설정 모드 (Wi-Fi 정보가 없을 때)
    else {
        server.on("/", HTTP_GET, handleRoot);
        server.on("/save", HTTP_POST, handleSave);
    }

    server.begin();
    uart0_printf("[%8lu ms] [Config Mode] Web server started.\n", millis());
}

void loop_config_mode() {
    server.handleClient();
}

#endif // _LIB_SOFTAP_WEBSERVER_H_
