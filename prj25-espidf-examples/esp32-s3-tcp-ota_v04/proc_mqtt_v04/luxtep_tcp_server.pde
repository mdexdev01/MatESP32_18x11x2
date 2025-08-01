// luxtep_tcp_server.pde
// Version: Ver.2.1.0.6
// File: luxtep_tcp_server.pde
// Author: Original Author & Refactored by AI
// Date: 2025-07-26

import java.net.ServerSocket;
import java.net.Socket;
import java.net.InetAddress;
import java.io.InputStream;
import java.io.OutputStream;
import java.io.IOException;
import java.util.concurrent.*;
import java.util.HashMap;
import java.util.Iterator;
import java.util.ArrayList;

// ====================================================================
// --- TCP 서버 관련 전역 변수 ---
// ====================================================================
public static final int NUM_CLIENTS = 8;
public static final int TCP_RECV_PACKET_SIZE = 962;
public static ServerSocket tcpServerSocket;
public static Thread acceptorThreadInstance;
public static ArrayList<Socket> clientSocketList = new ArrayList<Socket>();
public static HashMap<String, Socket> latestClientSocketMap = new HashMap<String, Socket>();
public static SafeBuffer[] clientBuffers = new SafeBuffer[NUM_CLIENTS];
public static int[] lastSeq = new int[NUM_CLIENTS];
public static int[] receivedPackets = new int[NUM_CLIENTS];
public static long[] lastFpsCalcTime = new long[NUM_CLIENTS];
public static float[] clientFps = new float[NUM_CLIENTS];
public static long[] lastPacketTime = new long[NUM_CLIENTS];
public static final long CLIENT_TIMEOUT_MS = 2000;
public static final float MAX_EXPECTED_FPS = 400.0f;
public static int tx_seq_num = 0;

// ====================================================================
// --- TCP 서버 초기화 함수 ---
// ====================================================================
void setup_tcp_server() {
  println("Initializing TCP Server on port " + TCP_SERVER_PORT + "...");
  try {
    tcpServerSocket = new ServerSocket(TCP_SERVER_PORT);
  } catch (IOException e) {
    e.printStackTrace();
    return;
  }
  for (int i = 0; i < NUM_CLIENTS; i++) {
    clientBuffers[i] = new SafeBuffer();
    lastSeq[i] = -1;
    lastFpsCalcTime[i] = millis();
    receivedPackets[i] = 0;
    clientFps[i] = 0.0f;
    lastPacketTime[i] = 0;
  }
  acceptorThreadInstance = new Thread(new AcceptorThread(tcpServerSocket));
  acceptorThreadInstance.setDaemon(true);
  acceptorThreadInstance.start();
  println("TCP Server listener thread started.");
}

// ====================================================================
// --- TCP 클라이언트로 패킷 송신 함수 (수정됨) ---
// ====================================================================
void send_tcp_packet_to_all_clients() {
  int width = 28;
  int height = 35;
  int targetId = 8; // 기본값은 전체
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
  for (int i = 0; i < osd_data_size; i++) {
    tx_actual[16 + i] = (byte)mouseX;
    if (100 < i) tx_actual[16 + i] = (byte)0xEF;
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
  } else {
    // 전체 클라이언트에게 전송 (ID 8 또는 그 외)
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

  //println("[5]=" + tx_actual[5] + ", target id=" + targetId);
  //println("[5]*10=" + tx_actual[5] * 10 + ", target id * 10=" + targetId * 10);
  
  tx_seq_num++;
}

// ====================================================================
// --- 더블 버퍼 클래스 (변경 없음) ---
// ====================================================================
class SafeBuffer {
  private byte[][] buffer = new byte[2][];
  private volatile int currentIndex = 0;
  synchronized void write(byte[] data) { currentIndex = (currentIndex + 1) % 2; buffer[currentIndex] = data; }
  synchronized byte[] read() { return buffer[currentIndex]; }
}

// ====================================================================
// --- AcceptorThread (변경 없음) ---
// ====================================================================
class AcceptorThread implements Runnable {
  private ServerSocket serverSocket;
  AcceptorThread(ServerSocket ss) { this.serverSocket = ss; }
  public void run() {
    while (true) {
      try {
        Socket newClientSocket = serverSocket.accept();
        String ip = newClientSocket.getInetAddress().getHostAddress();
        println(millis() + "ms] 🆕 New client connected: " + ip);
        synchronized (clientSocketList) {
          Socket existingMappedSocket = latestClientSocketMap.get(ip);
          if (existingMappedSocket != null) {
            Iterator<Socket> listIt = clientSocketList.iterator();
            while (listIt.hasNext()) {
              if (listIt.next() == existingMappedSocket) {
                try { existingMappedSocket.close(); } catch (IOException e) {}
                listIt.remove();
                break;
              }
            }
          }
          latestClientSocketMap.put(ip, newClientSocket);
          clientSocketList.add(newClientSocket);
        }
        Thread handlerThread = new Thread(new ClientHandler(newClientSocket));
        handlerThread.setDaemon(true);
        handlerThread.start();
      } catch (IOException e) {
        if (serverSocket.isClosed()) break;
      }
    }
  }
}

// ====================================================================
// --- ClientHandler (수정됨) ---
// ====================================================================
class ClientHandler implements Runnable {
  private Socket clientSocket;
  private InputStream is;
  ClientHandler(Socket s) {
    this.clientSocket = s;
    try { this.is = clientSocket.getInputStream(); } catch (IOException e) {}
  }
  public void run() {
    String clientIp = clientSocket.getInetAddress().getHostAddress();
    byte[] rxBuf = new byte[TCP_RECV_PACKET_SIZE]; // 952
    int bytesRead; // bytesReceived 대신 bytesRead 사용

    while (clientSocket.isConnected() && !clientSocket.isClosed()) {
      try {
        // TCP_RECV_PACKET_SIZE (952) 바이트가 사용 가능할 때만 읽습니다.
        if (is.available() >= TCP_RECV_PACKET_SIZE) {
          bytesRead = is.read(rxBuf, 0, TCP_RECV_PACKET_SIZE);
          if (bytesRead == -1) break; // 스트림 끝

          // 정확히 TCP_RECV_PACKET_SIZE 만큼 읽었는지 확인
          if (bytesRead == TCP_RECV_PACKET_SIZE) {
            // 패킷 유효성 검사 (시작/끝 마커)
            // 끝 마커 위치는 고정된 952바이트 패킷의 마지막에서 두 번째 바이트 (인덱스 950)
            if (rxBuf[0] == (byte)0xFF && rxBuf[1] == (byte)0xFF && rxBuf[TCP_RECV_PACKET_SIZE - 1] == (byte)0xFE) {
              int clientId = rxBuf[3] & 0xFF; // ESP32에서 보낸 클라이언트 ID는 txBuffer[3]에 있음
              int sequenceNum = rxBuf[TCP_RECV_PACKET_SIZE - 2] & 0xFF; // 시퀀스 번호는 마지막에서 두 번째 바이트

              synchronized (clientBuffers) {
                if (clientId >= 0 && clientId < NUM_CLIENTS) {
                  // 클라이언트 ID와 소켓 매핑 업데이트
                  clientIdToSocketMap.put(clientId, this.clientSocket);

                  lastPacketTime[clientId] = millis();
                  lastSeq[clientId] = sequenceNum; // 시퀀스 번호 업데이트
                  if (clientId == 0) {
                    println("Client ID 0 packet: byte at index " + (TCP_RECV_PACKET_SIZE - 3) + " is " + (rxBuf[TCP_RECV_PACKET_SIZE - 3] & 0xFF));
                  }
                  receivedPackets[clientId]++;
                  long now = millis();
                  float dt = (now - lastFpsCalcTime[clientId]) / 1000.0f;
                  if (dt >= 2.0f) {
                    clientFps[clientId] = receivedPackets[clientId] / dt;
                    receivedPackets[clientId] = 0;
                    lastFpsCalcTime[clientId] = now;
                  }
                  // 수신된 패킷을 SafeBuffer에 저장 (전체 952바이트)
                  clientBuffers[clientId].write(rxBuf); // rxBuf는 이미 952바이트
                }
              }
              // println("[TCP RX] Valid packet. Total Size: " + TCP_RECV_PACKET_SIZE + " bytes, Client ID: " + clientId + ", Sequence: " + sequenceNum);
            } else {
              println("[TCP RX] Error: Invalid packet markers (Start: " + hex(rxBuf[0] & 0xFF, 2) + " " + hex(rxBuf[1] & 0xFF, 2) + ", End: " + hex(rxBuf[TCP_RECV_PACKET_SIZE - 1] & 0xFF, 2) + "). Discarding.");
            }
          } else {
            // 예상한 952바이트보다 적게 읽었을 경우 (부분 패킷)
            println("[TCP RX] Warning: Read " + bytesRead + " bytes, expected " + TCP_RECV_PACKET_SIZE + ". Discarding partial packet.");
          }
        }
      } catch (IOException e) { break; }
      delay(1);
    }
    // [추가] 연결 종료 시 매핑 제거
    synchronized (clientIdToSocketMap) {
        Iterator<Integer> it = clientIdToSocketMap.keySet().iterator();
        while(it.hasNext()){
            Integer key = it.next();
            if(clientIdToSocketMap.get(key) == this.clientSocket){
                it.remove();
                break;
            }
        }
    }
    try { if (!clientSocket.isClosed()) clientSocket.close(); } catch (IOException e) {}
    synchronized (clientSocketList) {
      clientSocketList.remove(clientSocket);
      if (latestClientSocketMap.get(clientIp) == clientSocket) {
        latestClientSocketMap.remove(clientIp);
      }
    }
  }
}
