#ifndef LIB_BLE_OTA_H_
#define LIB_BLE_OTA_H_

/*
   MIT License

   Copyright (c) 2021 Felix Biego

   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software and associated documentation files (the "Software"), to deal
   in the Software without restriction, including without limitation the rights
   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
   copies of the Software, and to permit persons to whom the Software is
   furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included in all
   copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   SOFTWARE.
*/

//	https://github.com/fbiego/ESP32_BLE_OTA_Arduino
#include <BLE2902.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <Update.h>

#include "FFat.h"
#include "FS.h"
#include "SPIFFS.h"
#include "libPacket/packetBuffer.h"

extern int MY_BOARD_ID;  // 0: Main, 1: Sub

#define BUILTINLED 2
#define FORMAT_SPIFFS_IF_FAILED true
#define FORMAT_FFAT_IF_FAILED true

// #define USE_SPIFFS  // comment to use FFat

#ifdef USE_SPIFFS
#define FLASH SPIFFS
#define FASTMODE false  // SPIFFS write is slow
#else
#define FLASH FFat
#define FASTMODE true  // FFat is faster
#endif

#define NORMAL_MODE 0  // normal
#define UPDATE_MODE 1  // receiving firmware
#define OTA_MODE 2     // installing firmware

uint8_t updater[16384];
uint8_t updater2[16384];

#define SERVICE_UUID "fb1e4001-54ae-4a28-9f74-dfccb248601d"
#define CHARACTERISTIC_UUID_RX "fb1e4002-54ae-4a28-9f74-dfccb248601d"
#define CHARACTERISTIC_UUID_TX "fb1e4003-54ae-4a28-9f74-dfccb248601d"

static BLECharacteristic *pCharacteristicTX;
static BLECharacteristic *pCharacteristicRX;

static bool deviceConnected = false, sendMode = false, sendSize = true;
static bool writeFile = false, request = false;
static int writeLen = 0, writeLen2 = 0;
static bool current = true;
static int parts = 0, next = 0, cur = 0, MTU = 0;
static int MODE = NORMAL_MODE;
unsigned long rParts, tParts;

static void rebootEspWithReason(String reason) {
    uart0_println(reason);
    delay(1000);
    ESP.restart();
}

class MyServerCallbacks : public BLEServerCallbacks {
    void onConnect(BLEServer *pServer) {
        deviceConnected = true;
    }
    void onDisconnect(BLEServer *pServer) {
        deviceConnected = false;
    }
};

class MyCallbacks : public BLECharacteristicCallbacks {
    //    void onStatus(BLECharacteristic* pCharacteristic, Status s, uint32_t code) {
    //      Serial.print("Status ");
    //      Serial.print(s);
    //      Serial.print(" on characteristic ");
    //      Serial.print(pCharacteristic->getUUID().toString().c_str());
    //      Serial.print(" with code ");
    //      uart0_println(code);
    //    }

    void onNotify(BLECharacteristic *pCharacteristic) {
        uint8_t *pData;
        std::string value = pCharacteristic->getValue();
        int len = value.length();
        pData = pCharacteristic->getData();
        if (pData != NULL) {
            //        Serial.print("Notify callback for characteristic ");
            //        Serial.print(pCharacteristic->getUUID().toString().c_str());
            //        Serial.print(" of data length ");
            //        uart0_println(len);
            Serial.print("TX  ");
            for (int i = 0; i < len; i++) {
                uart0_printf("%02X ", pData[i]);
            }
            uart0_printf("\n");
        }
    }

    void onWrite(BLECharacteristic *pCharacteristic) {
        uint8_t *pData;
        std::string value = pCharacteristic->getValue();
        int len = value.length();
        pData = pCharacteristic->getData();
        if (pData != NULL) {
            //        Serial.print("Write callback for characteristic ");
            //        Serial.print(pCharacteristic->getUUID().toString().c_str());
            //        Serial.print(" of data length ");
            //        uart0_println(len);
            //        Serial.print("RX  ");
            //        for (int i = 0; i < len; i++) {         // leave this commented
            //          uart0_printf("%02X ", pData[i]);
            //        }
            //        uart0_println();

            if (pData[0] == 0xFB) {
                int pos = pData[1];
                for (int x = 0; x < len - 2; x++) {
                    if (current) {
                        updater[(pos * MTU) + x] = pData[x + 2];
                    } else {
                        updater2[(pos * MTU) + x] = pData[x + 2];
                    }
                }

            } else if (pData[0] == 0xFC) {
                if (current) {
                    writeLen = (pData[1] * 256) + pData[2];
                } else {
                    writeLen2 = (pData[1] * 256) + pData[2];
                }
                current = !current;
                cur = (pData[3] * 256) + pData[4];
                writeFile = true;
                if (cur < parts - 1) {
                    request = !FASTMODE;
                }
            } else if (pData[0] == 0xFD) {
                sendMode = true;
                if (FLASH.exists("/update.bin")) {
                    FLASH.remove("/update.bin");
                }
            } else if (pData[0] == 0xFE) {
                rParts = 0;
                tParts = (pData[1] * 256 * 256 * 256) + (pData[2] * 256 * 256) + (pData[3] * 256) + pData[4];

                uart0_printf("Available space: %d, File Size: %d", FLASH.totalBytes() - FLASH.usedBytes(), tParts);

            } else if (pData[0] == 0xFF) {
                parts = (pData[1] * 256) + pData[2];
                MTU = (pData[3] * 256) + pData[4];
                MODE = UPDATE_MODE;

            } else if (pData[0] == 0xEF) {
                FLASH.format();
                sendSize = true;
            }
        }
    }
};

static void writeBinary(fs::FS &fs, const char *path, uint8_t *dat, int len) {
    // uart0_printf("Write binary file %s\r\n", path);

    File file = fs.open(path, FILE_APPEND);

    if (!file) {
        uart0_printf("- failed to open file for writing \n");
        return;
    }
    file.write(dat, len);
    file.close();
    writeFile = false;
    rParts += len;
}

void sendOtaResult(String result) {
    pCharacteristicTX->setValue(result.c_str());
    pCharacteristicTX->notify();
    delay(200);
}

void performUpdate(Stream &updateSource, size_t updateSize) {
    char s1 = 0x0F;
    String result = String(s1);
    if (Update.begin(updateSize)) {
        size_t written = Update.writeStream(updateSource);
        if (written == updateSize) {
            uart0_printf("Written : %d successfully\n", written);
        } else {
            uart0_printf("Written only : %d/%d . Retry?\n", written, updateSize);
            // uart0_println("Written only : " + String(written) + "/" + String(updateSize) + ". Retry?");
        }
        result += "Written : " + String(written) + "/" + String(updateSize) + " [" + String((written / updateSize) * 100) + "%] \n";
        if (Update.end()) {
            uart0_printf("OTA done!\n");
            result += "OTA Done: ";
            if (Update.isFinished()) {
                uart0_printf("Update successfully completed. Rebooting...\n");
                result += "Success!\n";
            } else {
                uart0_printf("Update not finished? Something went wrong!\n");
                result += "Failed!\n";
            }

        } else {
            uart0_printf("Error Occurred. Error #: %s \n", String(Update.getError()));
            // uart0_println("Error Occurred. Error #: " + String(Update.getError()));
            result += "Error #: " + String(Update.getError());
        }
    } else {
        uart0_printf("Not enough space to begin OTA\n");
        // uart0_println("Not enough space to begin OTA");
        result += "Not enough space for OTA";
    }
    if (deviceConnected) {
        sendOtaResult(result);
        delay(5000);
    }
}

void updateFromFS(fs::FS &fs) {
    File updateBin = fs.open("/update.bin");
    if (updateBin) {
        if (updateBin.isDirectory()) {
            uart0_printf("Error, update.bin is not a file\n");
            updateBin.close();
            return;
        }

        size_t updateSize = updateBin.size();

        if (updateSize > 0) {
            uart0_println("Trying to start update");
            performUpdate(updateBin, updateSize);
        } else {
            uart0_println("Error, file is empty");
        }

        updateBin.close();

        // when finished remove the binary from spiffs to indicate end of the process
        uart0_println("Removing update file");
        fs.remove("/update.bin");

        rebootEspWithReason("Rebooting to complete OTA update");
    } else {
        uart0_println("Could not load update.bin from spiffs root");
    }
}

void initBLE() {
    char ble_device_name[32];
    sprintf(ble_device_name, "LUXTEP OTA - board[%d] Ver.0.1.1\0", MY_BOARD_ID);
    BLEDevice::init(ble_device_name);

    BLEServer *pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());

    BLEService *pService = pServer->createService(SERVICE_UUID);
    pCharacteristicTX = pService->createCharacteristic(CHARACTERISTIC_UUID_TX, BLECharacteristic::PROPERTY_NOTIFY);
    pCharacteristicRX = pService->createCharacteristic(CHARACTERISTIC_UUID_RX, BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_WRITE_NR);
    pCharacteristicRX->setCallbacks(new MyCallbacks());
    pCharacteristicTX->setCallbacks(new MyCallbacks());
    pCharacteristicTX->addDescriptor(new BLE2902());
    pCharacteristicTX->setNotifyProperty(true);
    pService->start();

    // BLEAdvertising *pAdvertising = pServer->getAdvertising();  // this still is working for backward compatibility
    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(0x06);  // functions that help with iPhone connections issue
    pAdvertising->setMinPreferred(0x12);
    BLEDevice::startAdvertising();
    uart0_println("Characteristic defined! Now you can read it in your phone!");
}

void setup_ota() {
    // Serial.begin(115200);
    uart0_println("Starting BLE OTA sketch");
    // pinMode(BUILTINLED, OUTPUT);

#ifdef USE_SPIFFS
    uart0_println("SPIFFS begins");
    if (!SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED)) {
        uart0_println("SPIFFS Mount Failed");
        return;
    }
#else
    uart0_println("FFAT begins");
    if (!FFat.begin()) {
        uart0_println("FFat Mount Failed");
        if (FORMAT_FFAT_IF_FAILED) FFat.format();
        return;
    }
#endif

    initBLE();
}

void loop_ota() {
    switch (MODE) {
        case NORMAL_MODE:
            if (deviceConnected) {
                // digitalWrite(BUILTINLED, HIGH);
                if (sendMode) {
                    uint8_t fMode[] = {0xAA, FASTMODE};
                    pCharacteristicTX->setValue(fMode, 2);
                    pCharacteristicTX->notify();
                    delay(50);
                    sendMode = false;
                }

                if (sendSize) {
                    unsigned long x = FLASH.totalBytes();
                    unsigned long y = FLASH.usedBytes();
                    uint8_t fSize[] = {0xEF, (uint8_t)(x >> 16), (uint8_t)(x >> 8), (uint8_t)x, (uint8_t)(y >> 16), (uint8_t)(y >> 8), (uint8_t)y};
                    pCharacteristicTX->setValue(fSize, 7);
                    pCharacteristicTX->notify();
                    delay(50);
                    sendSize = false;
                }

                // your loop code here
            } else {
                // digitalWrite(BUILTINLED, LOW);
            }

            // or here

            break;

        case UPDATE_MODE:

            if (request) {
                uint8_t rq[] = {0xF1, (cur + 1) / 256, (cur + 1) % 256};
                pCharacteristicTX->setValue(rq, 3);
                pCharacteristicTX->notify();
                delay(50);
                request = false;
            }

            if (cur + 1 == parts) {  // received complete file
                uint8_t com[] = {0xF2, (cur + 1) / 256, (cur + 1) % 256};
                pCharacteristicTX->setValue(com, 3);
                pCharacteristicTX->notify();
                delay(50);
                MODE = OTA_MODE;
            }

            if (writeFile) {
                if (!current) {
                    writeBinary(FLASH, "/update.bin", updater, writeLen);
                } else {
                    writeBinary(FLASH, "/update.bin", updater2, writeLen2);
                }
            }

            break;

        case OTA_MODE:

            if (writeFile) {
                if (!current) {
                    writeBinary(FLASH, "/update.bin", updater, writeLen);
                } else {
                    writeBinary(FLASH, "/update.bin", updater2, writeLen2);
                }
            }

            if (rParts == tParts) {
                uart0_println("Complete");
                delay(5000);
                updateFromFS(FLASH);
            } else {
                writeFile = true;
                uart0_println("Incomplete");
                uart0_printf("Expected: %d, Received: ", tParts, rParts);
                // Serial.print("Expected: ");
                // Serial.print(tParts);
                // Serial.print("Received: ");
                // uart0_println(rParts);
                delay(2000);
            }
            break;
    }
}

#endif  // LIB_BLE_OTA_H_