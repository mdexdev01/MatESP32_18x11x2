//  multi thread, rx double buffer, visualize, send button, reconnection

import processing.net.*;
import java.net.*;
import java.io.*;
import java.util.concurrent.*;
import java.util.HashMap;
import java.util.Iterator;

final int NUM_CLIENTS = 8;
int PACKET_SIZE = 998 + 952;

Server server;
ArrayList<Client> clientList = new ArrayList<Client>();
SafeBuffer[] buffers = new SafeBuffer[NUM_CLIENTS];
HashMap<String, Integer> clientIpToId = new HashMap<String, Integer>();
HashMap<String, Client> latestClientMap = new HashMap<String, Client>();

// 시퀀스 추적용
int[] lastSeq = new int[NUM_CLIENTS];
int[] receivedPackets = new int[NUM_CLIENTS];
long[] lastTime = new long[NUM_CLIENTS];
float[] fps = new float[NUM_CLIENTS];

final float MAX_EXPECTED_FPS = 400.0;

Button sendButton;

void setup() {
  size(600, 400);
  surface.setTitle("LuxtepMat WiFi TCP Server Test");

  server = new Server(this, 9000);

  for (int i = 0; i < NUM_CLIENTS; i++) {
    buffers[i] = new SafeBuffer();
    lastSeq[i] = -1;
    lastTime[i] = millis();
  }

  sendButton = new Button("Send Packet", 450, 20, 120, 30);

  Thread reader = new Thread(new ClientReader());
  reader.start();
}

void draw() {
  background(30);
  textSize(15);
  textAlign(LEFT);

    fill(255, 120, 0);
    text("Server IP: " + getLocalIPAddress() + "  Port: 9000", 10, 15);

  for (int i = 0; i < NUM_CLIENTS; i++) {
    byte[] data = buffers[i].read();
    if (data != null && data.length == PACKET_SIZE) {
      int seq = data[PACKET_SIZE - 2] & 0xFF;

      int baseY = 20 + 45 + i * 45;
      fill(255);
      text("Client " + i + " | Seq: " + seq + " | FPS: " + nf(fps[i], 0, 1), 10, baseY);

      float fpsBarWidth = map(min(fps[i], MAX_EXPECTED_FPS), 0, MAX_EXPECTED_FPS, 0, 200);
      fill(0, 200, 0);
      rect(200, baseY - 8, fpsBarWidth, 8);
      fill(255);
      text("[FPS]", 200 + fpsBarWidth + 5, baseY);
    }
  }

  sendButton.display();
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

int seq_send = 0;
void mousePressed() {
  if (sendButton.isClicked(mouseX, mouseY)) {
    int width = 28;
    int height = 35;
    int osd_data_size = width * height;
    int OSD_PACKET_SIZE = 8 + 8 + osd_data_size + 2; // = 998. 28 * 35 = 980
    int PAYLOAD_SIZE = 8 + osd_data_size + 2; // = 990. 28 * 35 = 980

    // OSD packet -------------------------
    byte[] tx = new byte[PACKET_SIZE];
    tx[0] = (byte)0xFF;
    tx[1] = (byte)0xFF;
    tx[2] = 0x02;
    tx[3] = 0; // tx ID
    
    tx[4] = 3; // 3: G_OSD_COMMAND
    tx[5] = byte(0 << 4 | (seq_send % 2)) ; // osd_id << 4 | rx_board_id
    tx[6] = byte(PAYLOAD_SIZE / 100);
    tx[7] = byte(PAYLOAD_SIZE % 100);
    
    //  OSD Sub header -------------------------
    tx[8] = 0; // start_x
    tx[9] = 0; // start_y
    tx[10] = 28; // width
    tx[11] = 35; // height
    tx[12] = 0; // duration
    tx[13] = 0; // brightness
    tx[14] = 0; // res 0
    tx[15] = 0; // res 1

    // 0xEF : transparent
    for (int i = 0; i < osd_data_size ; i++) {
      tx[16 + i] = (byte)mouseX; // 임시 payload
      
      if(100 < i) {
        tx[16 + i] = (byte)0xEF;
      }
    }
   
    //tx[OSD_PACKET_SIZE - 2] = (byte)frameCount; // seq
    tx[OSD_PACKET_SIZE - 2] = (byte)seq_send; // seq
    tx[OSD_PACKET_SIZE - 1] = (byte)0xFE;

    Iterator<Client> it = clientList.iterator();
    while (it.hasNext()) {
      Client c = it.next();
      try {
        if (c.active()) {
          c.write(tx);
        } else {
          println("Inactive client removed before TX. size: " + clientList.size());
          it.remove();
        }
      } catch (Exception e) {
        println("Error sending to client: " + e);
        it.remove();
      }
    }
    
    println("TX attempted to " + clientList.size() + " clients, seq: " + seq_send);
    seq_send++;
  }
}

class SafeBuffer {
  byte[][] buffer = new byte[2][];
  int index = 0;

  synchronized void write(byte[] data) {
    index ^= 1;
    buffer[index] = data;
  }

  synchronized byte[] read() {
    return buffer[index];
  }
}

class ClientReader implements Runnable {
  public void run() {
    while (true) {
      Client newClient = server.available();

      if(newClient == null)
        continue;
      if (newClient != null && newClient.ip() != null) {
        String ip = newClient.ip();
        Client existing = latestClientMap.get(ip);
        if (existing == null || !existing.active()) {
          latestClientMap.put(ip, newClient);
          clientList.add(newClient);
          println("🆕 New client connected: " + ip + " size:" + clientList.size());
        }
      }

      while (newClient.available() >= PACKET_SIZE) {
        byte[] buf = new byte[PACKET_SIZE];
        newClient.readBytes(buf);
        int id = buf[3] & 0xFF;
        int seq = buf[PACKET_SIZE - 2] & 0xFF;

        clientIpToId.put(newClient.ip(), id);

        if (lastSeq[id] != -1) {
          int delta = (seq - lastSeq[id] + 100) % 100;
        }
        lastSeq[id] = seq;
        receivedPackets[id]++;

        long now = millis();
        float dt = (now - lastTime[id]) / 1000.0;
        if (dt >= 1.0) {
          fps[id] = receivedPackets[id] / dt;
          receivedPackets[id] = 0;
          lastTime[id] = now;
        }

        buffers[id].write(buf);
      }
      delay(1);
    }
  }
}

class Button {
  String label;
  int x, y, w, h;

  Button(String label, int x, int y, int w, int h) {
    this.label = label;
    this.x = x;
    this.y = y;
    this.w = w;
    this.h = h;
  }

  void display() {
    fill(100);
    rect(x, y, w, h);
    fill(255);
    textAlign(CENTER, CENTER);
    text(label, x + w/2, y + h/2);
  }

  boolean isClicked(int mx, int my) {
    return (mx >= x && mx <= x + w && my >= y && my <= y + h);
  }
}
