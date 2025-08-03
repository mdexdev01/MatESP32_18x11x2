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
      if (millis() - lastPublishMsg > 2000) { // 2초마다 전송
        lastPublishMsg = millis();
        sendJson_ServerInfo(tcpServerIpInput.getText());
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

  //printMessgeMqtt(true, pubTopic_AppID, json);
}

// JSON 형식의 서버 정보 발행 함수
int sequence = 0;
void sendJson_ServerInfo(String ipAddress) {
  
  //pubTopic_AppID = appIDInput.getText();
  //mqttClientIdDisplay.setText(pubTopic_AppID);
  JSONObject json = new JSONObject();
  json.setString("AppID", pubTopic_AppID);
  json.setString("sender", "AppTCPServer");
  json.setString("tcp_server_ip", ipAddress);
  json.setString("payload", ipAddress); // payload 필드 (IP 주소)
  json.setInt("sequence", sequence); 

  String message = json.toString();
  mqttClient.publish(pubTopic_AppID, message);

  //printMessgeMqtt(true, pubTopic_AppID, json);
  
  sequence++;
}


// --- MQTT 이벤트 콜백 함수들 ---

void clientConnected() {
  pubTopic_AppID = appIDInput.getText();
  println(formatMillis(millis()) + " Connected to MQTT Broker!" + " AppID:" + pubTopic_AppID);
  isMqttConnected = true;
  mqttClient.subscribe(subsTopic_DeviceAlive);
  mqttClient.subscribe(pubTopic_AppID);
  mqttClient.subscribe(publishTopicCommand);

  mqttClientIdDisplay.setText(pubTopic_AppID);
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

    //println(formatMillis(millis()) + " MQTT Subs\nSubs Topic={" + topic + "} " + senderInfo + " " + payloadInfo);
    if (json == null) {
      println(formatMillis(millis()) + " [ERROR] MQTT Subs NULL message \n");
      return;
    }
    
    printMessgeMqtt(true, topic, json);

    // --- Textarea에 메시지 추가 (로그용) ---
    if(json.hasKey("sender") && json.hasKey("payload") && json.hasKey("sequence")) {
      String logMessageForArea = formatMillis(millis()) + " MQTT Subs: Subs Topic={" + topic + "} \n" 
                                + "    sender:" + json.getString("sender")  + " " + json.getString("payload");
      if (json.hasKey("sequence")) 
        logMessageForArea += (", seq: " + json.get("sequence").toString());
        
      mqttMessageArea.append(logMessageForArea + "\n");
    }


/*
    // 모든 키-값 쌍을 동적으로 출력

    println("  Parsed JSON for topic: " + topic);
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
        //if (json.hasKey("tcp_server_ip")) {
        //    String receivedIp = json.getString("tcp_server_ip");
        //    println("    Received TCP Server IP (from my own publish): " + receivedIp);
        //}
    } else if (topic.equals(publishTopicCommand)) { // PC가 명령을 발행하는 토픽 (ESP32가 구독)
        if (json.hasKey("command")) {
            String command = json.getString("command");
            println("    Received Command: " + command);
        }
    }
*/

  
  } catch (Exception e) {
    println(formatMillis(millis()) + " Error parsing JSON payload for topic: " + topic + " - " + e.getMessage());
    //e.printStackTrace();
  }
}

//  rx = true, tx=false
void printMessgeMqtt(boolean rx, String topic,String message) {
    JSONObject json = parseJSONObject(message);
    printMessgeMqtt(rx, topic, json);
}

//  rx = true, tx=false
void printMessgeMqtt(boolean rx, String topic,JSONObject message) {
  if(rx == true)
    print("Subs ");
  else
    print("Pubs "); 

  println("Topic={" + topic + "}\n"   + message);
}
