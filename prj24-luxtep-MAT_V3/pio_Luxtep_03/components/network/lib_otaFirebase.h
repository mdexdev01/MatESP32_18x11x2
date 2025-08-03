// lib_ota_firebase.h
// Version: Ver.1.1 (Neopixel Code Removed)
// Author: Provided by User & Refactored by AI
// Date: 2025-07-22
// 역할: 지정된 URL에서 펌웨어 바이너리를 다운로드하여 OTA 업데이트를 수행합니다.

#ifndef _LIB_OTA_FIREBASE_H_
#define _LIB_OTA_FIREBASE_H_

#include <WiFiClientSecure.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <Update.h>
#include "libPrintRaw.h"

// 상단: 인증서 포인터 선언
const char* GTS_ROOT_R1_CA =
"-----BEGIN CERTIFICATE-----\n"
"MIIFVzCCAz+gAwIBAgINAgPlk28xsBNJiGuiFzANBgkqhkiG9w0BAQwFADBHMQsw\n"
"CQYDVQQGEwJVUzEiMCAGA1UEChMZR29vZ2xlIFRydXN0IFNlcnZpY2VzIExMQzEU\n"
"MBIGA1UEAxMLR1RTIFJvb3QgUjEwHhcNMTYwNjIyMDAwMDAwWhcNMzYwNjIyMDAw\n"
"MDAwWjBHMQswCQYDVQQGEwJVUzEiMCAGA1UEChMZR29vZ2xlIFRydXN0IFNlcnZp\n"
"Y2VzIExMQzEUMBIGA1UEAxMLR1RTIFJvb3QgUjEwggIiMA0GCSqGSIb3DQEBAQUA\n"
"A4ICDwAwggIKAoICAQC2EQKLHuOhd5s73L+UPreVp0A8of2C+X0yBoJx9vaMf/vo\n"
"27xqLpeXo4xL+Sv2sfnOhB2x+cWX3u+58qPpvBKJXqeqUqv4IyfLpLGcY9vXmX7w\n"
"Cl7raKb0xlpHDU0QM+NOsROjyBhsS+z8CZDfnWQpJSMHobTSPS5g4M/SCYe7zUjw\n"
"TcLCeoiKu7rPWRnWr4+wB7CeMfGCwcDfLqZtbBkOtdh+JhpFAz2weaSUKK0Pfybl\n"
"qAj+lug8aJRT7oM6iCsVlgmy4HqMLnXWnOunVmSPlk9orj2XwoSPwLxAwAtcvfaH\n"
"szVsrBhQf4TgTM2S0yDpM7xSma8ytSmzJSq0SPly4cpk9+aCEI3oncKKiPo4Zor8\n"
"Y/kB+Xj9e1x3+naH+uzfsQ55lVe0vSbv1gHR6xYKu44LtcXFilWr06zqkUspzBmk\n"
"MiVOKvFlRNACzqrOSbTqn3yDsEB750Orp2yjj32JgfpMpf/VjsPOS+C12LOORc92\n"
"wO1AK/1TD7Cn1TsNsYqiA94xrcx36m97PtbfkSIS5r762DL8EGMUUXLeXdYWk70p\n"
"aDPvOmbsB4om3xPXV2V4J95eSRQAogB/mqghtqmxlbCluQ0WEdrHbEg8QOB+DVrN\n"
"VjzRlwW5y0vtOUucxD/SVRNuJLDWcfr0wbrM7Rv1/oFB2ACYPTrIrnqYNxgFlQID\n"
"AQABo0IwQDAOBgNVHQ8BAf8EBAMCAYYwDwYDVR0TAQH/BAUwAwEB/zAdBgNVHQ4E\n"
"FgQU5K8rJnEaK0gnhS9SZizv8IkTcT4wDQYJKoZIhvcNAQEMBQADggIBAJ+qQibb\n"
"C5u+/x6Wki4+omVKapi6Ist9wTrYggoGxval3sBOh2Z5ofmmWJyq+bXmYOfg6LEe\n"
"QkEzCzc9zolwFcq1JKjPa7XSQCGYzyI0zzvFIoTgxQ6KfF2I5DUkzps+GlQebtuy\n"
"h6f88/qBVRRiClmpIgUxPoLW7ttXNLwzldMXG+gnoot7TiYaelpkttGsN/H9oPM4\n"
"7HLwEXWdyzRSjeZ2axfG34arJ45JK3VmgRAhpuo+9K4l/3wV3s6MJT/KYnAK9y8J\n"
"ZgfIPxz88NtFMN9iiMG1D53Dn0reWVlHxYciNuaCp+0KueIHoI17eko8cdLiA6Ef\n"
"MgfdG+RCzgwARWGAtQsgWSl4vflVy2PFPEz0tv/bal8xa5meLMFrUKTX5hgUvYU/\n"
"Z6tGn6D/Qqc6f1zLXbBwHSs09dR2CQzreExZBfMzQsNhFRAbd03OIozUhfJFfbdT\n"
"6u9AWpQKXCBfTkBdYiJ23//OYb2MI3jSNwLgjt7RETeJ9r/tSQdirpLsQBqvFAnZ\n"
"0E6yove+7u7Y/9waLd64NnHi/Hm3lCXRSHNboTXns5lndcEZOitHTtNCjv0xyBZm\n"
"2tIMPNuzjsmhDYAPexZ3FL//2wmUspO8IFgV6dtxQ/PeEMMA3KgqlbbC1j+Qa3bb\n"
"bP6MvPJwNQzcmRk13NfIRmPVNnGuV/u3gm3c\n"
"-----END CERTIFICATE-----\n";



void OTA_Firebase() {
    const char* firmwareUrl = "https://luxtep-ota-v02.web.app/firmware.bin";

    uart0_printf("[%8lu ms] [OTA] Requesting firmware: %s\n", millis(), firmwareUrl);
    {
        setLedColor(0, 34 - 2, 0, 100, 0); // 34 : Y coord, 0: R, 100: G, 0: B
        setLedColor(0, 34 - 2, 0, 100, 0); // 34 : Y coord, 0: R, 100: G, 0: B
        strip[3].Show();
        delay(1000);
    }

    WiFiClientSecure client;
    client.setCACert(GTS_ROOT_R1_CA); // 아래에서 정의됨

    HTTPClient http;
    http.begin(client, firmwareUrl);

    int httpCode = http.GET();
    if (httpCode != HTTP_CODE_OK) {
        uart0_printf("[OTA] HTTP GET failed. Code: %d\n", httpCode);
        http.end();
        return;
    }

    int contentLength = http.getSize();
    if (contentLength <= 0) {
        uart0_printf("[OTA] Invalid content length: %d\n", contentLength);
        http.end();
        return;
    }

    bool canBegin = Update.begin(contentLength);
    if (!canBegin) {
        uart0_printf("[OTA] Not enough space for OTA\n");
        http.end();
        return;
    }

    uart0_printf("[%8lu ms] [OTA] Starting update. Total size: %d bytes\n", millis(), contentLength);
    for(int x = 0 ; x < 3 ; x++) {
        setLedColor(x, 34 - 2, 0, 100, 0); // 34 : Y coord, 0: R, 100: G, 0: B
        setLedColor(x, 34 - 3, 0, 100, 0); // 34 : Y coord, 0: R, 100: G, 0: B
    }
    strip[3].Show();
    delay(1000);

    WiFiClient* stream = http.getStreamPtr();
    uint8_t buff[1024];
    size_t written = 0;
    int update_percent = 0;

    while (http.connected() && written < contentLength) {
        //  fill LED during OTA
        update_percent = (written*100)/contentLength;
        int led_fill_percent = update_percent/4 + 4; // 24 + 4 = 28
        for(int x = 0 ; x < led_fill_percent ; x++) {
            setLedColor(x, 34 - 2, 0, 100, 0); // 34 : Y coord, 0: R, 100: G, 0: B
            setLedColor(x, 34 - 3, 0, 100, 0); // 34 : Y coord, 0: R, 100: G, 0: B
        }
        strip[3].Show();

        size_t len = stream->available();
        if (len) {
            int c = stream->readBytes(buff, (len > sizeof(buff)) ? sizeof(buff) : len);
            if (Update.write(buff, c) != c) {
                uart0_printf("[OTA] Update.write failed at byte %d (%d%%)\n", written, update_percent);
                Update.abort();
                http.end();
                return;
            }
            written += c;

            uart0_printf("[%8lu ms] [OTA] Written %d bytes (%d%%)\n", millis(), written, update_percent);
        }
    }

    if (!Update.end()) {
        uart0_printf("[] [OTA] Update.end failed: %s\n", Update.errorString());
        http.end();
        return;
    }

    if (!Update.isFinished()) {
        uart0_printf("[OTA] Update incomplete.\n");
        http.end();
        return;
    }

    uart0_printf("[%8lu ms] [OTA] Update successful. Rebooting...\n", millis());
    http.end();
    delay(1000);
    ESP.restart();
}


// --- OTA 수행 함수 ---
void OTA_Firebase2() {
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