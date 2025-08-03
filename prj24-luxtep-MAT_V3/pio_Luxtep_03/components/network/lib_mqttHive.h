// lib_mqttHive.h
// Version: Ver.1.0
// Author: mdex.co.kr
// Date: 2025-07-21

#ifndef _LIB_MQTTHIVE_H_
#define _LIB_MQTTHIVE_H_

#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "network_info.h"
#include "lib_nvs.h"
#include "libPrintRaw.h"

// --- Extern 변수 선언 ---
extern String mqttClientId;
extern String myFormattedMacAddress;
extern String receivedTcpServerIpAddress;
extern WiFiClient tcpClient; // IP 수신 시 기존 TCP 연결을 끊기 위해 필요
extern String currentAppID;

// --- 변수/객체 정의 ---
WiFiClientSecure clientSecure;
PubSubClient client(clientSecure);

// --- 함수 구현 ---
void reconnectMqtt(){
  static unsigned long lastMqttReconnectAttempt = 0;
  if (WiFi.status() != WL_CONNECTED)
    return;

  if (!client.connected()){
    if (MQTT_RETRY_INTERVAL_MS <= millis() - lastMqttReconnectAttempt){
      if (mqttClientId.length() == 0 && 0 < myFormattedMacAddress.length()){
        char clientId_cstr[32];
        sprintf(clientId_cstr, "ESP32_%s", myFormattedMacAddress.c_str());
        mqttClientId = String(clientId_cstr);
      }

      if (0 < mqttClientId.length()){
        if (client.connect(mqttClientId.c_str(), MQTT_USERNAME, MQTT_PASSWORD)){
          uart0_printf("[%8lu ms] MQTT Connected. Subscribing to topics... {%s}\n", millis(), currentAppID.c_str());
          client.subscribe(currentAppID.c_str());
          client.subscribe(COMMAND_TOPIC);
          // client.subscribe(MQTT_PUB_TOPIC_DEVICE_ALIVE);
          for(int x = 0 ; x < SIZE_X ; x++) {
              setLedColor(x, 34 - 2, 100, 100, 100); // 34 : Y coord, 100: R, 100: G, 100: B
              setLedColor(x, 34 - 3, 100, 100, 100); // 34 : Y coord, 100: R, 100: G, 100: B
          }
          strip[3].Show();
          delay(1000);

        }else{
          uart0_printf("MQTT Failed, rc=%d, %s\n", client.state(), mqttClientId.c_str());
          for(int x = 0 ; x < SIZE_X ; x++) {
              setLedColor(x, 34 - 2, 100, 0, 0); // 34 : Y coord, 100: R, 100: G, 100: B
              setLedColor(x, 34 - 3, 100, 0, 0); // 34 : Y coord, 100: R, 100: G, 100: B
          }
          strip[3].Show();
          delay(1500);
          
        }
      }
      lastMqttReconnectAttempt = millis();
    }
  }
}

void callback(char *topic, byte *payload, unsigned int length){
  uart0_printf("[MQTT] Message received on topic: %s\n", topic);

  StaticJsonDocument<256> doc;
  deserializeJson(doc, payload, length);

  if (doc.containsKey("tcp_server_ip")){
    String receivedIp = doc["tcp_server_ip"];
    if (receivedIp != receivedTcpServerIpAddress) {

      uart0_printf("[MQTT] Payload: ");
      for (unsigned int i = 0; i < length; i++){
        uart0_printf("%c", (char)payload[i]);
      }
      uart0_printf("\n");

      receivedTcpServerIpAddress = receivedIp;
      uart0_printf("[MQTT] Received NEW App Server IP: %s\n", receivedTcpServerIpAddress.c_str());
      saveTcpIpToNVS(receivedTcpServerIpAddress);
      if (tcpClient.connected()){
        tcpClient.stop();
      }
    }
  }
}

#endif // _LIB_MQTTHIVE_H_