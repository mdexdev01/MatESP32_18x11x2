import controlP5.*;
import processing.serial.*;
import java.net.*;
import java.io.*;

ControlP5 cp5;

processing.serial.Serial port;
boolean isPortOpened = false;

Textfield ssidField, passField, ipField, comPort;
Button sendButton;
Textlabel statusLabel;
PFont font;

final int HEADER1 = 0xFF;
final int HEADER2 = 0xFF;
final int FOOTER  = 0xFE;
final int ACK_SUCCESS = 0x01;
final int ACK_FAIL    = 0x00;

void setup() {
  size(400, 400);
  surface.setTitle("LuxtepMat WiFi Config Sender");

  font = createFont("Arial", 14, true);
  textFont(font);

  //port = new processing.serial.Serial(this, "COM17", 921600);
  //port.clear();
  //delay(200);

  cp5 = new ControlP5(this);
  cp5.setFont(font);
  
  int pos_y = 40; 

  // SSID setting
  ssidField = cp5.addTextfield("SSID")
    .setPosition(20, pos_y)
    .setSize(360, 30)
    .setColorBackground(color(250, 250, 250))
    .setColor(color(0,0,0))
    .setFont(font)
    .setAutoClear(false)
    .setText(getCurrentSSID());
  ssidField.getCaptionLabel().setColor(color(0, 102, 204));

  // Password setting
  pos_y += 60;
  passField = cp5.addTextfield("Password")
    .setPosition(20, pos_y)
    .setSize(360, 30)
    .setColorBackground(color(250, 250, 250))
    .setColor(color(0,0,0))
    .setFont(font)
    .setAutoClear(false)
    .setText("fab8ezx4");
  passField.getCaptionLabel().setColor(color(0, 102, 204));

  // Local IP setting
  pos_y += 60;
  ipField = cp5.addTextfield("My PC IP (TCP Server IP)")
    .setPosition(20, pos_y)
    .setSize(360, 30)
    .setColorBackground(color(250, 250, 250))
    .setColor(color(0,0,0))
    .setFont(font)
    .setAutoClear(false)
    .setText(getLocalIPAddress());
  ipField.getCaptionLabel().setColor(color(0, 102, 204));

  // Com port
  pos_y += 60 + 30;
  comPort = cp5.addTextfield("ComPort")
    .setPosition(20, pos_y)
    .setSize(60, 20)
    .setColorBackground(color(250, 250, 250))
    .setColor(color(0,0,0))
    .setFont(font)
    .setText("17")
    .setAutoClear(false);
  comPort.getCaptionLabel().setColor(color(0, 102, 204));

  // Open button
  sendButton = cp5.addButton("Open_Serial")
    .setPosition(120, pos_y)
    .setSize(130, 40)
    .setFont(font);

  // Send button
  sendButton = cp5.addButton("Send_Info")
    .setPosition(260, pos_y)
    .setSize(120, 40)
    .setFont(font);

  // Log text window
  statusLabel = cp5.addTextlabel("status")
    .setPosition(20, 360)
    .setSize(360, 20)
    .setFont(font)
    .setColorValue(color(0, 102, 204))
    .setText("device status");
}

void draw() {
  background(240);
  fill(0);
  textAlign(LEFT, TOP);
  text("WiFi Config Sender for ESP32", 20, 10);
  
  if(false == isPortOpened)
    return;

  while (port.available() >= 2) {
    // Parsing
    byte[] peekHeader = new byte[2];
    port.readBytes(peekHeader);

    if ((peekHeader[0] & 0xFF) == HEADER1 && (peekHeader[1] & 0xFF) == HEADER2) {
      // check length
      while (port.available() < 8) {
        delay(1);
      }
      byte[] remaining = new byte[8];
      port.readBytes(remaining);
      byte[] fullPacket = new byte[10];
      fullPacket[0] = peekHeader[0];
      fullPacket[1] = peekHeader[1];
      arrayCopy(remaining, 0, fullPacket, 2, 8);

      if ((fullPacket[9] & 0xFF) == FOOTER) {
        int result = fullPacket[6] & 0xFF;
        if (result == ACK_SUCCESS) {
          statusLabel.setText("ESP32: WiFi connected");
        } else if (result == ACK_FAIL) {
          statusLabel.setText("ESP32: WiFi failed");
        } else {
          statusLabel.setText("ESP32: Unknown ACK");
        }
      }
    } else {
      // Not a packet. Just print it.
      print("\t>");
      print((char) peekHeader[0]);
      print((char) peekHeader[1]);
      while (port.available() > 0) {
        char c = (char) port.read();
        print(c);
        if (c == '\n') break;
      }
      //println();
    }
  }
}  

void Open_Serial() {
  String strBuf;
  String portName;

  strBuf = cp5.get(Textfield.class, "ComPort").getText();
  println("Button open Pressed : " + strBuf);
  
  portName = String.format("COM%s", strBuf);

  try {
    port = new processing.serial.Serial(this, portName, 921600); // 921600: baud rate
    isPortOpened = true;

    statusLabel.setText("Serial port opened: " + portName);
    println("Serial port opened: " + portName);
    
  } catch (Exception e) {
    e.printStackTrace();
    isPortOpened = false;
    port = null;
    
    statusLabel.setText("Failed to open serial port: " + portName);
    println("Failed to open serial port: " + portName);
  }
  
  delay(200);
}

void Send_Info() {
  String ssid = ssidField.getText();
  String pass = passField.getText();
  String ip   = ipField.getText();

  if (ssid.isEmpty() || ip.isEmpty()) {
    statusLabel.setText("Send failed: SSID or IP missing.");
    println("SSID and IP must be filled.");
    return;
  }

  try {
    String json = "{\"ssid\":\"" + ssid + "\",\"pass\":\"" + pass + "\",\"ip\":\"" + ip + "\"}";
    byte[] jsonBytes = json.getBytes();
    int totalLength = 8 + jsonBytes.length + 2;
    //int totalLength = jsonBytes.length + 2;

    byte[] packet = new byte[totalLength];
    packet[0] = (byte)HEADER1;
    packet[1] = (byte)HEADER2;
    packet[2] = 0x02;
    packet[3] = 0x09; // 9 : M_APP_OWNER
    
    packet[4] = 0x01; // 1 : G_APP_COMMAND
    packet[5] = 0x01; // 1 : M_AC_WIFI_AP_CONFIG_INFO
    packet[6] = (byte)((totalLength - 8)/100);
    packet[7] = (byte)((totalLength - 8)%100);
    //packet[7] = jsonBytes[0];

    for (int i = 0; i < jsonBytes.length; i++) {
      packet[8 + i] = jsonBytes[i];
    }

    packet[totalLength - 2] = 'E'; // reserved
    packet[totalLength - 1] = (byte)FOOTER;

    port.write(packet);
    
    statusLabel.setText("Sent to Luxtep Device. Waiting ACK...");
    println("Packet sent: " + json);
  } catch (Exception e) {
    statusLabel.setText("Send failed.");
    println("Send failed: " + e.getMessage());
  }
}

String getLocalIPAddress() {
  try {
    InetAddress local = InetAddress.getLocalHost();
    return local.getHostAddress();
  } catch (Exception e) {
    println("Failed to get local IP.");
    return "";
  }
}

String getCurrentSSID() {
  try {
    Process proc = Runtime.getRuntime().exec("netsh wlan show interfaces");
    BufferedReader reader = new BufferedReader(new InputStreamReader(proc.getInputStream()));
    String line;
    while ((line = reader.readLine()) != null) {
      line = line.trim();
      if (line.startsWith("SSID") && !line.startsWith("SSID name")) {
        String[] parts = line.split(":");
        if (parts.length > 1) return parts[1].trim();
      }
    }
  } catch (Exception e) {
    println("Failed to get SSID.");
  }
  return "";
}
