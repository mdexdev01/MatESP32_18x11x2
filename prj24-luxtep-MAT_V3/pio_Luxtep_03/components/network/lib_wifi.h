// lib_wifi.h
// Version: Ver.1.0
// Author: mdex.co.kr
// Date: 2025-07-21

#ifndef _LIB_WIFI_H_
#define _LIB_WIFI_H_

#include <WiFi.h>
#include "network_info.h"
#include "lib_nvs.h"
#include "libPrintRaw.h"

// --- Extern 변수 선언 ---
extern String myFormattedMacAddress;
extern unsigned long bootTimestamp;
extern bool shouldEnterSoftAP;
extern Preferences prefs;

// --- 함수 구현 ---
bool isSSID2_4GHz(const char* target_ssid) {
    int n = WiFi.scanNetworks();
    if (n == 0) {
        uart0_printf("[%8lu ms] [WiFi Scan] No networks found.\n", millis());
        return false;
    }
    for (int i = 0; i < n; ++i) {
        if (String(target_ssid) == WiFi.SSID(i)) {
            int channel = WiFi.channel(i);
            if (1 <= channel && channel <= 14) {
                uart0_printf("[%8lu ms] [WiFi Scan] %s is 2.4GHz (Channel %d)\n", millis(), target_ssid, channel);
                return true;
            } else {
                uart0_printf("[%8lu ms] [WiFi Scan] %s is 5.0GHz (Channel %d) - ESP32 cannot connect.\n", millis(), target_ssid, channel);
                return false;
            }
        }
    }
    uart0_printf("[%8lu ms] [WiFi Scan] SSID '%s' not found in scan results.\n", millis(), target_ssid);
    return false;
}

bool connectToWiFiFromNVS() {
    prefs.begin(NVS_NAMESPACE, true);
    String stored_ssid = prefs.getString(NVS_KEY_SSID, "");
    String stored_pass = prefs.getString(NVS_KEY_PASS, "");
    prefs.end();

    if (stored_ssid.length() == 0) {
        shouldEnterSoftAP = true;
        return false;
    }

    uart0_printf("[%8lu ms] [WiFi] Attempting to connect to stored WiFi: %s\n", millis(), stored_ssid.c_str());

    if (!isSSID2_4GHz(stored_ssid.c_str())) {
        shouldEnterSoftAP = true;
        return false;
    }

    WiFi.begin(stored_ssid.c_str(), stored_pass.c_str());

    unsigned long startAttemptTime = millis();
    while (WiFi.status() != WL_CONNECTED && (millis() - startAttemptTime) < 15000) {
        delay(500);
    }

    if (WiFi.status() == WL_CONNECTED) {
        uart0_printf("[%8lu ms] [OK] WiFi connected!, IP: %s\n", millis(), WiFi.localIP().toString().c_str());
        return true;
    } else {
        uart0_printf("[%8lu ms] [WiFi] Failed to connect. Entering SoftAP mode.\n", millis());
        shouldEnterSoftAP = true;
        return false;
    }
}

void WiFiEvent(WiFiEvent_t event) {
    switch (event) {
        case ARDUINO_EVENT_WIFI_STA_GOT_IP:
            uart0_printf("WiFi Got IP: %s\n", WiFi.localIP().toString().c_str());
            if (myFormattedMacAddress.length() == 0) {
                uint8_t mac[6];
                WiFi.macAddress(mac);
                char mac_cstr[18];
                sprintf(mac_cstr, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
                myFormattedMacAddress = String(mac_cstr);
                uart0_printf("My MAC Address: %s\n", myFormattedMacAddress.c_str());
            }
            break;
        case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
            uart0_printf("WiFi Disconnected. Retrying...\n");
            break;
        default:
            break;
    }
}

#endif // _LIB_WIFI_H_