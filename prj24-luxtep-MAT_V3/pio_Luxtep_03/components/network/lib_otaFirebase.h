// lib_ota_firebase.h
// Version: Ver.1.1 (Neopixel Code Removed)
// Author: Provided by User & Refactored by AI
// Date: 2025-07-22
// 역할: 지정된 URL에서 펌웨어 바이너리를 다운로드하여 OTA 업데이트를 수행합니다.

#ifndef _LIB_OTA_FIREBASE_H_
#define _LIB_OTA_FIREBASE_H_

#include <WiFi.h>
#include <HTTPClient.h>
#include <Update.h>
#include "libPrintRaw.h"

// --- OTA 수행 함수 ---
void OTA_Firebase() {
    // 참고: HTTPS URL을 사용하므로, ESP32의 펌웨어에 해당 서버의 Root CA가 포함되어 있어야 할 수 있습니다.
    // 연결 실패 시, WiFiClientSecure와 setCACert()를 사용해야 할 수 있습니다.
    const char* firmwareUrl = "https://luxtep-ota-v02.web.app/firmware.bin";

    uart0_printf("[HTTP OTA] Fetching firmware from: %s\n", firmwareUrl);

    HTTPClient http;
    http.begin(firmwareUrl);
    int httpCode = http.GET();

    if (httpCode == HTTP_CODE_OK) {
        int contentLength = http.getSize();
        if (contentLength <= 0) {
            uart0_printf("[HTTP OTA] Error: Content-Length header missing or zero.\n");
            http.end();
            return;
        }

        bool canBegin = Update.begin(contentLength);

        if (canBegin) {
            uart0_printf("[HTTP OTA] Starting update. Firmware size: %d bytes.\n", contentLength);

            WiFiClient* stream = http.getStreamPtr();
            size_t written = Update.writeStream(*stream);

            if (written != contentLength) {
                 uart0_printf("[HTTP OTA] Error: Wrote %d of %d bytes.\n", written, contentLength);
                 http.end();
                 return;
            }

            if (Update.end()) {
                if (Update.isFinished()) {
                    delay(1000);
                    uart0_printf("[HTTP OTA] Success! Rebooting...\n");
                    ESP.restart();
                } else {
                    uart0_printf("[HTTP OTA] Error: Update not finished.\n");
                }
            } else {
                uart0_printf("[HTTP OTA] Error occurred: %s\n", Update.errorString());
            }
        } else {
            uart0_printf("[HTTP OTA] Error: Not enough space to begin OTA.\n");
        }
    } else {
        uart0_printf("[HTTP OTA] Error: HTTP GET failed, code: %d, error: %s\n", httpCode, http.errorToString(httpCode).c_str());
    }
    http.end();
}

#endif // _LIB_OTA_FIREBASE_H_