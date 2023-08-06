/*
https://github.com/espressif/arduino-esp32/blob/master/libraries/BLE/src/BLEBeacon.h
https://github.com/espressif/arduino-esp32/blob/master/libraries/BLE/src/BLEAdvertising.h
*/
/*
   Based on 31337Ghost's reference code from https://github.com/nkolban/esp32-snippets/issues/385#issuecomment-362535434
   which is based on pcbreflux's Arduino ESP32 port of Neil Kolban's example for IDF: https://github.com/nkolban/esp32-snippets/blob/master/cpp_utils/tests/BLE%20Tests/SampleScan.cpp
*/

/*
   Create a BLE server that will send periodic iBeacon frames.
   The design of creating the BLE server is:
   1. Create a BLE Server
   2. Create advertising data
   3. Start advertising.
   4. wait
   5. Stop advertising.
*/
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <BLEBeacon.h>
#include "esp_bt_device.h"


#define DEVICE_NAME       "Grib"
#define SERVICE_UUID      "bdfa6743-eb30-4139-80f8-e016adffe2d1" // for Grib
#define PROPERTY_UUID     "bdfa6743-eb30-4139-80f8-e016adffe2d2"


// #define DEVICE_NAME            "Grib"
// #define SERVICE_UUID           "7A0247E7-8E88-409B-A959-AB5092DDB03E"
// #define BEACON_UUID            "2D7A9F0C-E0E8-4CC9-A71B-A21DB2D034A1"
// #define BEACON_UUID_REV        "A134D0B2-1DA2-1BA7-C94C-E8E00C9F7A2D"
// #define CHARACTERISTIC_UUID    "82258BAA-DF72-47E8-99BC-B73D7ECD08A5"

BLEServer *pServer;
BLECharacteristic *pCharacteristic;

bool bleConnected = false;
int32_t value = 0;

class MyServerCallbacks : public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
        bleConnected = true;
        Serial.println("bleConnected = true");
    };

    void onDisconnect(BLEServer* pServer) {
        bleConnected = false;
        Serial.println("bleConnected = false");

        // Restart advertising to be visible and connectable again
        BLEAdvertising* pAdvertising;
        pAdvertising = pServer->getAdvertising();
        pAdvertising->start();
        Serial.println("iBeacon advertising restarted");
    }
};

class MyCallbacks : public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic* pCharacteristic) {
        std::string rxValue = pCharacteristic->getValue();

        if (rxValue.length() > 0) {
            Serial.println("*********");
            Serial.print("Received Value: ");
            for (int i = 0; i < rxValue.length(); i++) {
                Serial.print(rxValue[i]);
            }
            Serial.println();
            Serial.println("*********");
        }
    }
};

void init_service() {
    BLEAdvertising* pAdvertising;
    pAdvertising = pServer->getAdvertising();
    pAdvertising->stop();

    // Create the BLE Service
    BLEService* pService = pServer->createService(BLEUUID(SERVICE_UUID));

    // Create a BLE Characteristic
    pCharacteristic = pService->createCharacteristic(
        PROPERTY_UUID,
        // BLECharacteristic::PROPERTY_WRITE  |
        BLECharacteristic::PROPERTY_NOTIFY |
            // BLECharacteristic::PROPERTY_INDICATE |
            BLECharacteristic::PROPERTY_READ);
    pCharacteristic->setCallbacks(new MyCallbacks());
    pCharacteristic->addDescriptor(new BLE2902()); // cccd, enable 'notify'

    pAdvertising->addServiceUUID(BLEUUID(SERVICE_UUID));

    // Start the service
    pService->start();

    pAdvertising->start();
}

void init_beacon() {
    BLEAdvertising* pAdvertising;
    pAdvertising = pServer->getAdvertising();
    pAdvertising->stop();
    // iBeacon
    // BLEBeacon myBeacon;
    // myBeacon.setManufacturerId(0x4c00);
    // myBeacon.setMajor(5);
    // myBeacon.setMinor(88);
    // myBeacon.setSignalPower(0xc5);
    // myBeacon.setProximityUUID(BLEUUID(BEACON_UUID_REV));

    BLEAdvertisementData advertisementData;
    advertisementData.setFlags(0x1A);

    // advertisementData.setManufacturerData(myBeacon.getData());
    {
        const uint8_t* point = esp_bt_dev_get_address();

        byte manufacturer_specific_data[6];
        int offset = 0;
        // manufacturer_specific_data[0] = 7;
        // manufacturer_specific_data[1] = 0xff;
        manufacturer_specific_data[offset++] = 0x52;
        manufacturer_specific_data[offset++] = 0x47;
        manufacturer_specific_data[offset++] = 0x04;
        manufacturer_specific_data[offset++] = point[3];
        manufacturer_specific_data[offset++] = point[4];
        manufacturer_specific_data[offset++] = point[5];

        // std::string((char*) &m_beaconData, sizeof(m_beaconData));

        advertisementData.setManufacturerData(std::string((char*)manufacturer_specific_data, sizeof(manufacturer_specific_data)));
    }

    pAdvertising->setAdvertisementData(advertisementData);

    pAdvertising->start();
}

void setup_advGrib() {
    Serial.println();
    Serial.println("Initializing...");
    Serial.flush();

    BLEDevice::init(DEVICE_NAME);
    pServer = BLEDevice::createServer();
    BLEDevice::setMTU(500);
    pServer->setCallbacks(new MyServerCallbacks());

    init_service();
    init_beacon();

    Serial.println("iBeacon + service defined and advertising!");
}

uint8_t value8 = 0;
void loop_advNotify() { /// only for testing function.
  if (bleConnected) {
      Serial.printf("*** NOTIFY: %d ***\n", value);
      pCharacteristic->setValue(&value8, 1);
      pCharacteristic->notify();
      value8++;
  }
  delay(2000);
}

#define TX_PACKET_SIZE (16 * 11 + 4)
uint8_t tx_data[TX_PACKET_SIZE];

void loop_advGrib() {
    if (bleConnected) {
    }
    else {
      
    }

}


void loop_advGrib_old() {
  if (bleConnected) {
      // Serial.printf("*** NOTIFY: %d ***\n", value);
      // pCharacteristic->setValue(&value, 1);

      uint16_t mtuSize = BLEDevice::getMTU();
      // Serial.printf("Negotiated MTU size: %d \n", mtuSize);

      memcpy(tx_data, &value, 4);
      for (int i = 4; i < TX_PACKET_SIZE; i++) {
          tx_data[i] = (i % 100);
      }

      if( (value % 2) == 0) {
        pCharacteristic->setValue(tx_data, (size_t)TX_PACKET_SIZE);
        pCharacteristic->notify();
      }
      else {
        pCharacteristic->setValue(tx_data, (size_t)mtuSize);
        pCharacteristic->notify();
      }

      value++;
  } else {
      // Serial.printf("*** Advertising: %d ***\n", value);
  }
}
