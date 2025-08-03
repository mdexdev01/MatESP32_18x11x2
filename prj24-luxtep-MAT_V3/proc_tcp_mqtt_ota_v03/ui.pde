// File : ui.pde
// Project: Processing MQTT Client for HiveMQ - UI Module
// Version: Ver.2.1.0.6
// Author: mdex.co.kr & Refactored by AI
// Date: 2025-07-26

import controlP5.*;
import java.awt.Toolkit;
import java.awt.datatransfer.Clipboard;
import java.awt.datatransfer.DataFlavor;
import java.awt.datatransfer.UnsupportedFlavorException;
import java.io.IOException;

// ====================================================================
// --- UI 설정 및 이벤트 핸들러 함수 ---
// ====================================================================

// Helper function to get clipboard text
String getClipboardText() {
  String result = "";
  Clipboard clipboard = Toolkit.getDefaultToolkit().getSystemClipboard();
  //odd: the Object class in Processing is the superclass of all classes,
  //so it is not the same as java.lang.Object
  //if (clipboard.isDataFlavorAvailable(DataFlavor.stringFlavor)) {
  try {
    result = (String)clipboard.getData(DataFlavor.stringFlavor);
  } 
  catch (UnsupportedFlavorException ufe) {
    println("UnsupportedFlavorException: " + ufe.getMessage());
  }
  catch (IOException ioe) {
    println("IOException: " + ioe.getMessage());
  }
  return result;
}

void setup_ui(PApplet p) {
  cp5 = new ControlP5(p);

  int buttonHeight = 35;
  mqttToggleButton = cp5.addButton("mqttToggle")
    .setLabel("MQTT OFF")
    .setPosition(p.width/2 - 250, p.height - buttonHeight - 5)
    .setSize(150, buttonHeight)
    .setColorBackground(p.color(200, 80, 80))
    .setColorForeground(p.color(255, 100, 100))
    .setColorActive(p.color(150, 50, 50));

  int fieldWidth = 50;
  int fieldHeight = 30;
  int sendButtonX = WINDOW_WIDTH - 150;
  int fieldX = sendButtonX - fieldWidth - 10 - 50; // 필드들의 공통 X 좌표

  // [추가] 타겟 클라이언트 ID 입력 필드
  targetClientIdInput = cp5.addTextfield("targetClientIdInput")
    .setPosition(fieldX, 20)
    .setSize(fieldWidth, fieldHeight)
    .setFont(p.createFont("Arial", 16))
    .setText("1") // 기본값 1로 변경
    .setAutoClear(false)
    .setColorValue(p.color(255));
  targetClientIdInput.getCaptionLabel().setVisible(false);
  
  osdWidthInput = cp5.addTextfield("osdWidthInput")
    .setPosition(fieldX, 60)
    .setSize(fieldWidth, fieldHeight)
    .setFont(p.createFont("Arial", 16))
    .setText("28")
    .setAutoClear(false)
    .setColorValue(p.color(255));
  osdWidthInput.getCaptionLabel().setVisible(false);

  osdHeightInput = cp5.addTextfield("osdHeightInput")
    .setPosition(fieldX, 100)
    .setSize(fieldWidth, fieldHeight)
    .setFont(p.createFont("Arial", 16))
    .setText("35")
    .setAutoClear(false)
    .setColorValue(p.color(255));
  osdHeightInput.getCaptionLabel().setVisible(false);

  tcpSendButton = cp5.addButton("tcpSend")
    .setLabel("Send TCP Packet")
    .setPosition(sendButtonX, 20)
    .setSize(120, 110) // 버튼 높이 조정
    .setColorBackground(p.color(80, 120, 200))
    .setColorForeground(p.color(100, 150, 255))
    .setColorActive(p.color(50, 80, 150));

  osdBoard1 = cp5.addTextfield("osdBoard1")
    .setPosition(WINDOW_WIDTH - 400, 200)
    .setSize(180, 100)
    .setFont(p.createFont("Arial", 14))
    .setText("osd buffer - board 1")
    .setAutoClear(false)
    .setColorValue(p.color(255));
  osdBoard1.getCaptionLabel().setVisible(false);
  
  osd1PasteButton = cp5.addButton("osd1PasteButton")
    .setLabel("clear and paste") // Label changed
    .setPosition(WINDOW_WIDTH - 400, 310)
    .setSize(100, 30) // 버튼 높이 조정
    .setColorBackground(p.color(80, 120, 200))
    .setColorForeground(p.color(100, 150, 255))
    .setColorActive(p.color(50, 80, 150));
  
  osdBoard0 = cp5.addTextfield("osdBuf0")
    .setPosition(WINDOW_WIDTH - 200, 200)
    .setSize(180, 100)
    .setFont(p.createFont("Arial", 14))
    .setText("osd buffer - board 0")
    .setAutoClear(false)
    .setColorValue(p.color(255));
  osdBoard0.getCaptionLabel().setVisible(false);
  
  osd0PasteButton = cp5.addButton("osd0PasteButton")
    .setLabel("clear and paste") // Label changed
    .setPosition(WINDOW_WIDTH - 200, 310)
    .setSize(100, 30) // 버튼 높이 조정
    .setColorBackground(p.color(80, 120, 200))
    .setColorForeground(p.color(100, 150, 255))
    .setColorActive(p.color(50, 80, 150));
    
    
  // --- 기존 입력 필드들 ---
  int textFieldStartX = 200;
  int textFieldStartY = 50;
  int textFieldHeight = 30;
  int textFieldSpacing = 40;

  appIDInput = cp5.addTextfield("appIDInput")
    .setPosition(textFieldStartX, textFieldStartY)
    .setSize(140, textFieldHeight)
    .setFont(p.createFont("Arial", 16))
    .setText(pubTopic_AppID);
  appIDInput.getCaptionLabel().setVisible(false);

  String localIp = getLocalIPAddress();
  tcpServerIpInput = cp5.addTextfield("tcpServerIpInput")
    .setPosition(textFieldStartX, textFieldStartY + textFieldSpacing)
    .setSize(140, textFieldHeight)
    .setFont(p.createFont("Arial", 16))
    .setText(localIp);
  tcpServerIpInput.getCaptionLabel().setVisible(false);

  mqttClientIdDisplay = cp5.addTextfield("mqttClientIdDisplay")
    .setPosition(textFieldStartX, textFieldStartY + textFieldSpacing * 2)
    .setSize(140, textFieldHeight)
    .setFont(p.createFont("Arial", 16))
    .setText(appIDInput.getText())
    .setLock(true);
  mqttClientIdDisplay.getCaptionLabel().setVisible(false);
/*
  osd0PasteButton = cp5.addButton("osd0PasteButton")
    .setLabel("clear and paste") // Label changed
    .setPosition(WINDOW_WIDTH - 200, 310)
    .setSize(100, 30) // 버튼 높이 조정
    .setColorBackground(p.color(80, 120, 200))
    .setColorForeground(p.color(100, 150, 255))
    .setColorActive(p.color(50, 80, 150));
*/
  UpdateAppID = cp5.addButton("UpdateAppID")
    .setPosition(textFieldStartX + 160, textFieldStartY + textFieldSpacing * 2)
    .setSize(80, textFieldHeight)
    //.setFont(p.createFont("Arial", 16))
    .setLabel("AppID Update")
    .setColorBackground(p.color(80, 120, 200))
    .setColorForeground(p.color(100, 150, 255))
    .setColorActive(p.color(50, 80, 150));
    
  mqttMessageArea = cp5.addTextarea("mqttMessageArea")
    .setPosition(20, 250)
    .setSize(p.width/2 - 50, p.height - 300)
    .setFont(p.createFont("Arial", 12))
    .setLineHeight(14)
    .setColorBackground(p.color(50))
    .setColor(p.color(0, 255, 255));
}

// ====================================================================
// --- ControlP5 이벤트 핸들러 ---
// ====================================================================
public void controlEvent(ControlEvent theEvent) {
  if (theEvent.isController()) {
    String name = theEvent.getController().getName();
    
    if (name.equals("mqttToggle")) {
      if (isMqttRunning) {
        println("MQTT Control: OFF. Disconnecting via button.");
        mqttClient.disconnect();
        isMqttConnected = false;
        isMqttRunning = false;
        mqttToggleButton.setLabel("MQTT OFF").setColorBackground(color(200, 80, 80));
      } else {
        isMqttRunning = true;
        println("MQTT Control: ON. Attempting connection via button.");
        mqttClient.connect(getFullBrokerURI(), appIDInput.getText(), true);
        lastReconnectAttemptTime = millis();
        mqttToggleButton.setLabel("MQTT ON").setColorBackground(color(80, 200, 80));
      }
    } else if (name.equals("tcpSend")) {
      println("TCP Send button pressed.");
      send_tcp_packet_to_all_clients();
    } else if (name.equals("appIDInput")) {
      //mqttClientIdDisplay.setText(appIDInput.getText());
    } else if (name.equals("UpdateAppID")) {
      String newAppID = appIDInput.getText();
      try {
        mqttClient.unsubscribe(pubTopic_AppID);
        mqttClient.subscribe(appIDInput.getText());
        println("unsubscibe={" + pubTopic_AppID + "}, subscribe={" + newAppID + "} \n");
      } catch (Exception e) {
        println("mqtt unsubscribe error :" + e);
      } 
      pubTopic_AppID = newAppID; // update pubTopic_AppID
      mqttClientIdDisplay.setText(pubTopic_AppID);
    }else if (name.equals("osd1PasteButton")) {
      osdBoard1.setText(""); // Clear current text
      osdBoard1.setText(getClipboardText()); // Paste from clipboard
      targetClientIdInput.setText("1"); // Set targetId to 1
    } else if (name.equals("osd0PasteButton")) {
      osdBoard0.setText(""); // Clear current text
      osdBoard0.setText(getClipboardText()); // Paste from clipboard
      targetClientIdInput.setText("0"); // Set targetId to 0
    }
  }
}


// ====================================================================
// --- Input Handling ---
// ====================================================================
public void keyPressed() {
  // [수정] 새로운 입력 필드 포커스 체크 추가
  if (!appIDInput.isFocus() && !tcpServerIpInput.isFocus() && !osdWidthInput.isFocus() && !osdHeightInput.isFocus() && !targetClientIdInput.isFocus()) {
    if (isMqttConnected) {
      if (key == 'c' || key == 'C') {
        sendJsonCommandMessage("turn_on_led");
      } else if (key == 'a' || key == 'A') {
        sendJson_ServerInfo(tcpServerIpInput.getText());
      }
    } else {
      println("Not connected to MQTT broker. Cannot send message.");
    }
  }
}

public void mousePressed() {
}
