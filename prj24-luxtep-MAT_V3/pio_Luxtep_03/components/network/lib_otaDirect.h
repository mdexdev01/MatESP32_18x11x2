// lib_ota_direct.h
// Version: Ver.2.0 (Refactored for OTA only)
// Author: Original Author & Refactored by AI
// Date: 2025-07-22
// 역할: 웹 브라우저를 통한 펌웨어 업데이트 기능 제공

#ifndef _LIB_OTA_DIRECT_H_
#define _LIB_OTA_DIRECT_H_

#include <WebServer.h>
#include <Update.h>
#include "libPrintRaw.h"

// OTA 업데이트 페이지 HTML
const char* update_page_html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>ESP32 Firmware Update</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        body { font-family: Arial, sans-serif; margin: 20px; background-color: #333; color: #eee; }
        .container { background-color: #555; padding: 20px; border-radius: 8px; max-width: 500px; margin: auto; }
        input[type="file"] { margin-bottom: 10px; }
        input[type="submit"] {
            width: 100%; background-color: #f44336; color: white; padding: 14px 20px; margin: 8px 0;
            border: none; border-radius: 4px; cursor: pointer; font-size: 16px;
        }
        input[type="submit"]:hover { background-color: #da190b; }
        .progress-bar { width: 100%; background-color: #777; border-radius: 4px; }
        .progress { width: 0%; height: 20px; background-color: #4CAF50; border-radius: 4px; text-align: center; color: white; }
    </style>
</head>
<body>
    <div class="container">
        <h2>Firmware Update</h2>
        <form method='POST' action='/update' enctype='multipart/form-data'>
            <input type='file' name='update'>
            <input type='submit' value='Upload & Update'>
        </form>
        <div class="progress-bar">
            <div class="progress" id="progress">0%</div>
        </div>
    </div>
    <script>
        document.querySelector('form').addEventListener('submit', function(e) {
            e.preventDefault();
            var form = e.target;
            var data = new FormData(form);
            var xhr = new XMLHttpRequest();
            xhr.open('POST', form.action, true);
            xhr.upload.addEventListener('progress', function(e) {
                if (e.lengthComputable) {
                    var percentComplete = (e.loaded / e.total) * 100;
                    var progressBar = document.getElementById('progress');
                    progressBar.style.width = percentComplete.toFixed(2) + '%';
                    progressBar.textContent = percentComplete.toFixed(2) + '%';
                }
            }, false);
            xhr.onload = function() {
                alert('Upload Complete! The device will reboot.');
                window.location.href = '/';
            };
            xhr.send(data);
        });
    </script>
</body>
</html>
)rawliteral";


// OTA 셋업 함수
void OTA_Direct(WebServer &server) {
    // 업데이트 페이지 핸들러
    server.on("/update", HTTP_GET, [&]() {
        server.send(200, "text/html", update_page_html);
    });

    // 펌웨어 파일 업로드 및 업데이트 처리 핸들러
    server.on("/update", HTTP_POST, [&]() {
        server.sendHeader("Connection", "close");
        server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
        ESP.restart();
    }, [&]() {
        HTTPUpload& upload = server.upload();
        if (upload.status == UPLOAD_FILE_START) {
            uart0_printf("Update: %s\n", upload.filename.c_str());
            if (!Update.begin(UPDATE_SIZE_UNKNOWN)) { // 전체 플래시 크기만큼
                Update.printError(Serial);
            }
        } else if (upload.status == UPLOAD_FILE_WRITE) {
            if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
                Update.printError(Serial);
            }
        } else if (upload.status == UPLOAD_FILE_END) {
            if (Update.end(true)) { // true to set the size to the current progress
                uart0_printf("Update Success: %u bytes\n", upload.totalSize);
            } else {
                Update.printError(Serial);
            }
        }
    });
}

#endif // _LIB_OTA_DIRECT_H_