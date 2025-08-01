// File : luxtep_mqtt.pde
// Project: Processing MQTT Client for HiveMQ - MQTT Logic Module
// Version: Ver.1.9 (Time unit change from ms to sec in messages/logs)
// Author: mdex.co.kr
// Date: 2025-07-14 (Current Date)

// luxtep_mqtt.pde는 메인 스케치 파일에서 호출되므로, 여기에 import 문을 중복해서 넣지 않습니다.

// ====================================================================
// --- MQTT 관련 동작 및 콜백 함수 ---
// ====================================================================

// 엘랩스드 타임을 공백 패딩된 형식으로 반환하는 헬퍼 함수
String formatMillis(long ms) {
  String s = str(ms);
  // 문자열을 8자리로 만들고 부족하면 앞에 공백을 채웁니다.
  while (s.length() < 8) {
    s = " " + s; // 앞에 공백 추가
  }
  return "[" + s + "ms]";
}

// --- 완전한 브로커 URI 문자열을 생성하는 헬퍼 함수 ---
String getFullBrokerURI() {
  return "ssl://" + mqttUsername + ":" + mqttPassword + "@" + brokerAddress + ":" + brokerPort;
}

// draw() 내의 MQTT 관련 로직을 분리
void draw_mqtt() {
  if (isMqttRunning) {
    if (!isMqttConnected) {
      mqttStatusText = "MQTT Status: Attempting to connect MQTT...";
      if (millis() - lastReconnectAttemptTime > reconnectInterval) {
        String fullBrokerURI = getFullBrokerURI();
        mqttClient.connect(fullBrokerURI, appIDInput.getText(), true);
        println(formatMillis(millis()) + " Reconnecting to MQTT broker with Client ID: " + appIDInput.getText() + " URI: " + fullBrokerURI);
        lastReconnectAttemptTime = millis();
      }
    }
  }

  // UI 표시 텍스트 업데이트 (draw_mqtt 내에서 MQTT 상태를 기반으로 업데이트)
  if (isMqttRunning) {
    if (isMqttConnected) {
      mqttStatusText = "MQTT Status: Connected to " + brokerAddress;
      
      // command 메시지를 주기적으로 전송합니다.
      if (millis() - lastPublishMsg > 3000) { // 3초마다 전송
        lastPublishMsg = millis();

        JSONObject doc = new JSONObject();
        doc.setString("AppID", appIDInput.getText());
        doc.setString("sender", "AppTCPServer");
        doc.setInt("msg_main", 1);
        doc.setInt("msg_sub", 0);
        doc.setString("tcp_server_ip", tcpServerIpInput.getText());
        doc.setString("payload", tcpServerIpInput.getText()); // payload 필드

        String output = doc.toString();

        mqttClient.publish(publishTopicCommand, output);
        println(formatMillis(millis()) + " MQTT Pub by AppTCPServer,\nPub Topic={" + publishTopicCommand + "}\n" + output);
      }
    } else {
    }
  } else {
    mqttStatusText = "MQTT Status: Disconnected (Stopped)";
  }
}


// JSON 형식의 명령 메시지 발행 함수
void sendJsonCommandMessage(String command) {
  JSONObject json = new JSONObject();
  json.setString("AppID", appIDInput.getText());
  json.setString("sender", "AppTCPServer");
  json.setString("command", command);
  json.setString("payload", command); // command 문자열을 payload로 사용

  String message = json.toString();
  mqttClient.publish(publishTopicCommand, message);
  println(formatMillis(millis()) + " MQTT Pub by AppTCPServer,\nPub Topic={" + publishTopicCommand + "}\n" + message);
}

// JSON 형식의 서버 정보 발행 함수
void sendJson_ServerInfo(String ipAddress) {
  JSONObject json = new JSONObject();
  json.setString("AppID", appIDInput.getText());
  json.setString("sender", "AppTCPServer");
  json.setString("tcp_server_ip", ipAddress);
  json.setString("payload", ipAddress); // payload 필드 (IP 주소)

  String message = json.toString();
  mqttClient.publish(pubTopic_AppID, message);
  println(formatMillis(millis()) + " MQTT Pub by AppTCPServer,\nPub Topic={" + pubTopic_AppID + "}\n" + message);
}


// --- MQTT 이벤트 콜백 함수들 ---

void clientConnected() {
  println(formatMillis(millis()) + " Connected to MQTT Broker!");
  isMqttConnected = true;
  mqttClient.subscribe(subsTopic_DeviceAlive);
  mqttClient.subscribe(pubTopic_AppID);
  mqttClient.subscribe(publishTopicCommand);
}

void connectionLost() {
  println(formatMillis(millis()) + " Disconnected from MQTT Broker.");
  isMqttConnected = false;
}

void messageReceived(String topic, byte[] payload) {
  String message = new String(payload);
  
  // JSON 파싱 시도
  try {
    JSONObject json = parseJSONObject(message);
    
    //String formattedMessage;
    String senderInfo = "";
    String payloadInfo = "";
    
    if (json != null) {
      //formattedMessage = json.toString();
      
      if (json.hasKey("sender")) {
          senderInfo = "MAC[" + json.getString("sender") + "]";
      } else {
          senderInfo = "[Unknown Sender]";
      }
      
      // payload 및 elapsed_sec 정보 추출 (콘솔 로그용)
      // 변경: "ms" 대신 "s"로 표시
      if (json.hasKey("payload") && json.hasKey("elapsed_sec")) {
          payloadInfo = "- " + json.getString("payload") + " for " + json.getInt("elapsed_sec") + "s";
      } else if (json.hasKey("payload")) {
          payloadInfo = "- " + json.getString("payload");
      } else {
          payloadInfo = "[No Payload Info]";
      }
      
    } else {
      //formattedMessage = message;
      senderInfo = "[Non-JSON]";
      payloadInfo = "";
    }
    
    println(formatMillis(millis()) + " MQTT Subs\nSubs Topic={" + topic + "} " + senderInfo + " " + payloadInfo);
    
    // --- Textarea에 메시지 추가 (로그용) ---
    String logMessageForArea = formatMillis(millis()) + " MQTT Subs: Subs Topic={" + topic + "} " + senderInfo + " " + payloadInfo;
    mqttMessageArea.append(logMessageForArea + "\n");

    if (json != null) {
      // Handler의 로그 (주석 처리)
      /*
      println("  Parsed JSON for topic: " + topic);
      // 모든 키-값 쌍을 동적으로 출력
      for (Object keyObj : json.keys()) {
        String key = keyObj.toString();
        String value = "";
        try {
          if (!json.isNull(key)) {
            value = json.get(key).toString();
          } else {
            value = "[null]";
          }
        } catch (Exception e) {
          value = "[Non-String/Unprintable Value or Error]";
        }
        println("    " + key + ": " + value);
      }

      // 특정 토픽에 대한 특별 처리 (새로운 JSON 구조에 맞춰 변경)
      if (topic.equals(subsTopic_DeviceAlive)) { // ESP32가 발행한 데이터를 수신
        println("    --- Specific Device Alive Data ---");
        if (json.hasKey("AppID")) println("    AppID: " + json.getString("AppID"));
        if (json.hasKey("sender")) println("    Sender: " + json.getString("sender"));
        if (json.hasKey("msg_main")) println("    Msg Main: " + json.getInt("msg_main"));
        if (json.hasKey("msg_sub")) println("    Msg Sub: " + json.getInt("msg_sub"));
        if (json.hasKey("payload")) println("    Payload: " + json.getString("payload"));
        if (json.hasKey("elapsed_sec")) println("    Elapsed (sec): " + json.getInt("elapsed_sec"));
        println("    -------------------------");
      } else if (topic.equals(pubTopic_AppID)) { // PC IP를 발행하는 토픽 (ESP32가 구독)
          if (json.hasKey("tcp_server_ip")) {
              String receivedIp = json.getString("tcp_server_ip");
              println("    Received TCP Server IP (from my own publish): " + receivedIp);
          }
      } else if (topic.equals(publishTopicCommand)) { // PC가 명령을 발행하는 토픽 (ESP32가 구독)
          if (json.hasKey("command")) {
              String command = json.getString("command");
              println("    Received Command: " + command);
          }
      }
      */
    } else {
      println(formatMillis(millis()) + " MQTT Subs\nSubs Topic={" + topic + "}\n" + message);
    }
  } catch (Exception e) {
    println(formatMillis(millis()) + " Error parsing JSON payload for topic: " + topic + " - " + e.getMessage());
    //e.printStackTrace();
  }
}
