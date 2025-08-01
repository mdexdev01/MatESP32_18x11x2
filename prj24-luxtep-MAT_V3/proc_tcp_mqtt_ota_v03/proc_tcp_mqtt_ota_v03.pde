// File : proc_mqtt_v03.pde
// Project: Processing MQTT Client for HiveMQ
// Version: Ver.2.1.0.6
// Author: mdex.co.kr & Refactored by AI
// Date: 2025-07-26

import mqtt.*;
import processing.data.*;
import java.net.InetAddress;
import java.net.NetworkInterface;
import java.util.Enumeration;
import java.net.Socket;
import java.util.HashMap;
import controlP5.*;

// ====================================================================
// --- 전역 설정 및 상태 변수 ---
// ====================================================================
public static String brokerAddress = "385a0883b81e4fef92e75a06365fb8e6.s1.eu.hivemq.cloud";
public static int brokerPort = 8883;
public static String mqttUsername = "luxtep_mat_V1";
public static String mqttPassword = "Lluxtep8*";
public static String publishTopicCommand = "command";
public static String pubTopic_AppID = "banana7";
public static String subsTopic_DeviceAlive = "device-alive";
public static MQTTClient mqttClient;
public static boolean isMqttConnected = false;
public static long lastReconnectAttemptTime = 0;
public static long reconnectInterval = 5000;
public static long lastPublishMsg = 0;
public static ControlP5 cp5;
public static Textfield appIDInput;
public static Textfield tcpServerIpInput;
public static Textfield mqttClientIdDisplay;
public static Textarea mqttMessageArea;
public static controlP5.Button mqttToggleButton;
public static controlP5.Button tcpSendButton;
public static Textfield osdWidthInput;
public static Textfield osdHeightInput;
public static Textfield targetClientIdInput; // [추가] 타겟 클라이언트 ID 입력 필드

public static boolean isMqttRunning = false;
public static String mqttStatusText = "MQTT Status: Disconnected";
public static final int WINDOW_WIDTH = 900;
public static final int WINDOW_HEIGHT = 800;
public static final int TCP_SERVER_PORT = 9000;

// [추가] 클라이언트 ID와 소켓을 매핑하기 위한 HashMap
public static HashMap<Integer, Socket> clientIdToSocketMap = new HashMap<Integer, Socket>();

// ====================================================================
// --- setup() ---
// ====================================================================
void settings() {
  size(WINDOW_WIDTH, WINDOW_HEIGHT);
}

void setup() {
  background(0);
  textAlign(CENTER, CENTER);
  textSize(16);
  fill(0, 255, 255);
  text("Initializing...", width/2, height/2);

  setup_ui(this);
  mqttClient = new MQTTClient(this);
  setup_tcp_server();
  lastReconnectAttemptTime = millis();
}

// ====================================================================
// --- draw() ---
// ====================================================================
void draw() {
  background(0);
  fill(0, 255, 255);

  textAlign(LEFT, TOP);
  textSize(16);
  text(mqttStatusText, 50, 10);

  draw_mqtt();
  draw_tcp_clients_status();

  textAlign(LEFT, TOP);
  textSize(16);
  if (isMqttConnected) {
    text("Press 'C' to send command", 50, 180);
    text("Press 'A' to send " + appIDInput.getText() + " IP", 50, 200);
  } else {
    text("MQTT is OFF or Disconnected", 50, 180);
  }

  // --- 수동 라벨 그리기 ---
  textAlign(RIGHT, CENTER);
  textSize(16);
  int labelXOffset = 10;
  text("AppID:", appIDInput.getPosition()[0] - labelXOffset, appIDInput.getPosition()[1] + appIDInput.getHeight()/2);
  text("TCP Server IP:", tcpServerIpInput.getPosition()[0] - labelXOffset, tcpServerIpInput.getPosition()[1] + tcpServerIpInput.getHeight()/2);
  text("MQTT Client ID(= AppID):", mqttClientIdDisplay.getPosition()[0] - labelXOffset, mqttClientIdDisplay.getPosition()[1] + mqttClientIdDisplay.getHeight()/2);

  textAlign(RIGHT, CENTER);
  text("Target ID:", targetClientIdInput.getPosition()[0] - labelXOffset, targetClientIdInput.getPosition()[1] + targetClientIdInput.getHeight()/2);
  text("Width:", osdWidthInput.getPosition()[0] - labelXOffset, osdWidthInput.getPosition()[1] + osdWidthInput.getHeight()/2);
  text("Height:", osdHeightInput.getPosition()[0] - labelXOffset, osdHeightInput.getPosition()[1] + osdHeightInput.getHeight()/2);
  
  textAlign(LEFT, CENTER);
  textSize(12);
  fill(180);
  text("(0-7, 8:All)", targetClientIdInput.getPosition()[0] + targetClientIdInput.getWidth() + 5, targetClientIdInput.getPosition()[1] + targetClientIdInput.getHeight()/2);
  text("(0-28)", osdWidthInput.getPosition()[0] + osdWidthInput.getWidth() + 5, osdWidthInput.getPosition()[1] + osdWidthInput.getHeight()/2);
  text("(0-35)", osdHeightInput.getPosition()[0] + osdHeightInput.getWidth() + 5, osdHeightInput.getPosition()[1] + osdHeightInput.getHeight()/2);


  stroke(125);
  line(width / 2, 0, width/2, height);
}

// ====================================================================
// --- TCP 클라이언트 상태 시각화 함수 ---
// ====================================================================
void draw_tcp_clients_status() {
  final int TCP_UI_OFFSET_X = 600;
  final int TCP_UI_OFFSET_Y = 80;

  fill(255, 120, 0);
  textAlign(LEFT, TOP);
  textSize(15);
  text("Server IP: " + getLocalIPAddress() + "  Port: " + TCP_SERVER_PORT, TCP_UI_OFFSET_X + 10, 60 + TCP_UI_OFFSET_Y);

  for (int i = 0; i < NUM_CLIENTS; i++) {
    boolean isActive = (millis() - lastPacketTime[i]) < CLIENT_TIMEOUT_MS;

    if (isActive) {
      byte[] data = clientBuffers[i].read();
      if (data != null && data.length == TCP_RECV_PACKET_SIZE) {
        int seq = data[TCP_RECV_PACKET_SIZE - 2] & 0xFF;
        int baseY = 100 + i * 25 + TCP_UI_OFFSET_Y;
        
        fill(255);
        textAlign(LEFT);
        textSize(14);
        text("Client " + i + " | Seq: " + seq + " | FPS: " + nf(clientFps[i], 0, 1), TCP_UI_OFFSET_X + 10, baseY);

        float fpsBarWidth = map(min(clientFps[i], MAX_EXPECTED_FPS), 0, MAX_EXPECTED_FPS, 0, 100);
        fill(0, 200, 0);
        rect(TCP_UI_OFFSET_X + 180, baseY - 8, fpsBarWidth, 8);
        fill(255);
        text("[FPS]", TCP_UI_OFFSET_X + 180 + fpsBarWidth + 5, baseY);
      }
    } else {
      clientFps[i] = 0;
    }
  }
}

// ====================================================================
// --- Utility Function ---
// ====================================================================
String getLocalIPAddress() {
  try {
    Enumeration<NetworkInterface> interfaces = NetworkInterface.getNetworkInterfaces();
    while (interfaces.hasMoreElements()) {
      NetworkInterface current = interfaces.nextElement();
      if (!current.isLoopback() && current.isUp() && !current.isVirtual()) {
        Enumeration<InetAddress> addresses = current.getInetAddresses();
        while (addresses.hasMoreElements()) {
          InetAddress addr = addresses.nextElement();
          if (addr instanceof java.net.Inet4Address) {
            return addr.getHostAddress();
          }
        }
      }
    }
    return "127.0.0.1";
  } catch (Exception e) {
    return "Error Getting IP";
  }
}
