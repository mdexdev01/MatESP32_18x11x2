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
import processing.core.PApplet; // Import PApplet

// ====================================================================
// --- 전역 설정 및 상태 변수 ---
// ====================================================================
public static String brokerAddress = "385a0883b81e4fef92e75a06365fb8e6.s1.eu.hivemq.cloud";
public static int brokerPort = 8883;
public static String mqttUsername = "luxtep_mat_V1";
public static String mqttPassword = "Lluxtep8*";
public static String publishTopicCommand = "command";
public static String pubTopic_AppID = "marine3";
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
public static controlP5.Button UpdateAppID;
public static Textarea mqttMessageArea;
public static controlP5.Button mqttToggleButton;
public static controlP5.Button tcpSendButton;
public static Textfield osdWidthInput;
public static Textfield osdHeightInput;
public static Textfield targetClientIdInput; // [추가] 타겟 클라이언트 ID 입력 필드

public static Textfield osdBoard1;
public static Textfield osdBoard0;
public static controlP5.Button osd1PasteButton;
public static controlP5.Button osd0PasteButton;

public static PApplet currentPApplet; // Global PApplet instance

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

  currentPApplet = this; // Assign 'this' (PApplet instance) to the global variable
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
  final int TCP_UI_OFFSET_Y = 380;

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
void send_tcp_packet_to_all_clients() {
  int width = 28;
  int height = 35;
  int targetId = 1; // 기본값을 1로 변경
  try {
    width = constrain(Integer.parseInt(osdWidthInput.getText()), 0, 28);
    height = constrain(Integer.parseInt(osdHeightInput.getText()), 0, 35);
    targetId = Integer.parseInt(targetClientIdInput.getText());
  } catch (NumberFormatException e) {
    println("Invalid input. Using default values.");
  }
  
  int osd_data_size = width * height;
  int variable_data_len_encoded = (osd_data_size + 8 + 2);
  int total_packet_len = 8 + variable_data_len_encoded;

  byte[] tx_actual = new byte[total_packet_len];

  // 패킷 구성
  tx_actual[0] = (byte)0xFF;
  tx_actual[1] = (byte)0xFF;
  tx_actual[2] = 0x02;
  tx_actual[3] = 0;
  tx_actual[4] = 3; // 3 = group id is osd
  tx_actual[5] = (byte)(0 << 4 | targetId); // 패킷에 타겟 ID 설정
  tx_actual[6] = (byte)(variable_data_len_encoded / 100);
  tx_actual[7] = (byte)(variable_data_len_encoded % 100);
  tx_actual[8] = 0;
  tx_actual[9] = 0;
  tx_actual[10] = (byte)width;
  tx_actual[11] = (byte)height;
  tx_actual[12] = 0;
  tx_actual[13] = 0;
  tx_actual[14] = 0;
  tx_actual[15] = 0;
  
  // OSD 데이터 채우기
  if(targetId == 1) { // client id 1일 때 osdBoard1의 내용을 사용
    String osdDataString = osdBoard1.getText();
    String[] byteStrings = osdDataString.split(",");
    for (int i = 0; i < osd_data_size && i < byteStrings.length; i++) {
      try {
        tx_actual[16 + i] = (byte)Integer.parseInt(byteStrings[i].trim());
      } catch (NumberFormatException e) {
        tx_actual[16 + i] = (byte)0; // 파싱 오류 시 0으로 채움
      }
    }
  }
  else if(targetId == 0) { // client id 0일 때 osdBoard0의 내용을 사용
    String osdDataString = osdBoard0.getText();
    String[] byteStrings = osdDataString.split(",");
    for (int i = 0; i < osd_data_size && i < byteStrings.length; i++) {
      try {
        tx_actual[16 + i] = (byte)Integer.parseInt(byteStrings[i].trim());
      } catch (NumberFormatException e) {
        tx_actual[16 + i] = (byte)0; // 파싱 오류 시 0으로 채움
      }
    }
  }
  else { // 그 외의 경우 (기존 로직 유지)
    for (int i = 0; i < osd_data_size; i++) {
      tx_actual[16 + i] = (byte)mouseX;
    }
  }

  tx_actual[total_packet_len - 2] = (byte)tx_seq_num;
  tx_actual[total_packet_len - 1] = (byte)0xFE;

  // [수정] 타겟팅 로직
  if (targetId >= 0 && targetId < 8) {
    // 특정 클라이언트에게만 전송
    Socket targetSocket = clientIdToSocketMap.get(targetId);
    if (targetSocket != null && targetSocket.isConnected() && !targetSocket.isClosed()) {
      tx_actual[5] = (byte)(0 << 4 | targetId); // 패킷에 타겟 ID 설정
      try {
        OutputStream os = targetSocket.getOutputStream();

        print("  Actual TX bytes: ");
        for (int k = 0; k < min(total_packet_len, 20); k++) { // Print first 20 bytes for debug
          print(hex(tx_actual[k] & 0xFF, 2) + " ");
        }
        println();

        os.write(tx_actual);
        os.flush();
        println("Sent packet to Client " + targetId);
      } catch (IOException e) {
        println("Error sending to Client " + targetId + ": " + e.getMessage());
      }
    } else {
      println("Client " + targetId + " not connected or found.");
    }
  }
  else { // 전체 클라이언트에게 전송 (ID 8 또는 그 외)
    synchronized (clientSocketList) {
      for (Socket s : clientSocketList) {
        // [수정] 패킷의 client_id 부분을 0으로 설정하여 브로드캐스트임을 알림 (프로토콜 약속에 따라 변경 가능)
        tx_actual[5] = (byte)(0 << 4 | 8); 
        try {
          if (s.isConnected() && !s.isClosed()) {
            OutputStream os = s.getOutputStream();

            print("  Actual TX bytes: ");
            for (int k = 0; k < min(total_packet_len, 20); k++) { // Print first 20 bytes for debug
              print(hex(tx_actual[k] & 0xFF, 2) + " ");
            }
            println();
            
            os.write(tx_actual);
            os.flush();
          }
        } catch (IOException e) {
          // 에러 발생 시 해당 소켓은 다음 루프에서 정리될 것임
        }
      }
      println("Sent packet to all " + clientSocketList.size() + " clients." + "target id =" + targetId);
    }
  }

  tx_seq_num++;
}

String getLocalIPAddress() {
  try {
    Enumeration<NetworkInterface> interfaces = NetworkInterface.getNetworkInterfaces();
    while (interfaces.hasMoreElements()) {
      NetworkInterface current = interfaces.nextElement();
      if (!current.isLoopback() && current.isUp() && !current.isVirtual()) {
        Enumeration<InetAddress> addresses = current.getInetAddresses(); // Corrected line
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
