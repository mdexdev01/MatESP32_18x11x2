// lib_tcpClient.h
// Version: Ver.2.2 (Cleaned up)
// Author: mdex.co.kr
// Date: 2025-07-22

#ifndef _LIB_TCPCLIENT_H_
#define _LIB_TCPCLIENT_H_

#include <WiFi.h>
#include "network_info.h"
#include "lib_nvs.h"
#include "libPrintRaw.h"

// --- Extern 변수/객체 선언 ---
extern String storedTcpServerIp;
extern String receivedTcpServerIpAddress;
extern String currentAppID;
extern PubSubClient client;
extern int board_id_hardcoding; // .ino 파일에 정의된 전역 변수 참조

// --- Extern 함수 선언 ---
void reconnectMqtt();

// --- 변수/객체 정의 ---
WiFiClient tcpClient;
const int TCP_SEND_PACKET_SIZE = 962;
const int TCP_RECV_PACKET_SIZE = 998;
const unsigned long tcpSendInterval = 10;
const unsigned long tcpConnectInterval = 3000;

// --- 함수 구현 ---
void createAndSendTcpPacket()
{
    static uint32_t sequenceNumber = 0;
    static uint8_t txBuffer[TCP_SEND_PACKET_SIZE];
    txBuffer[0] = 0xFF;
    txBuffer[1] = 0xFF;
    txBuffer[2] = 0x02;
    txBuffer[TCP_SEND_PACKET_SIZE - 1] = 0xFE;

    // Client ID를 전역 변수 값으로 설정
    txBuffer[3] = board_id_hardcoding;

    if (tcpClient.connected())
    {
        txBuffer[TCP_SEND_PACKET_SIZE - 2] = sequenceNumber % 100;
        tcpClient.write(txBuffer, TCP_SEND_PACKET_SIZE);
    }
    sequenceNumber++;
}

void tcpReceiverTask(void *param)
{
    static uint8_t rxBuf[TCP_RECV_PACKET_SIZE];
    const int HEADER_LEN = 8;
    static int current_packet_len = 0; // 현재 처리 중인 패킷의 예상 총 길이
    static int bytes_received = 0;     // 현재까지 수신된 바이트 수

    while (true)
    {
        if (tcpClient.connected())
        {
            int available_bytes = tcpClient.available();
            if (available_bytes > 0)
            {
                // 1. 헤더를 기다리는 중이거나, 아직 헤더를 완전히 받지 못했을 때
                if (bytes_received < HEADER_LEN)
                {
                    int bytes_to_read = min(available_bytes, HEADER_LEN - bytes_received);
                    tcpClient.readBytes(&rxBuf[bytes_received], bytes_to_read);
                    bytes_received += bytes_to_read;
                    // uart0_printf("[TCP RX Debug] State: Reading Header. Avail: %d, Read: %d, Recv: %d\n", available_bytes, bytes_to_read, bytes_received);

                    // [DEBUG] Print rxBuf content immediately after reading header
                    // uart0_printf("[TCP RX Debug] Raw rxBuf after readBytes (partial): ");
                    // for (int i = 0; i < bytes_received; i++) {
                    //     uart0_printf("%02X ", rxBuf[i]);
                    // }
                    // uart0_printf("\n");

                    // 헤더를 완전히 수신했다면 패킷 길이를 파싱
                    if (bytes_received >= HEADER_LEN)
                    {
                        // 3. 헤더에서 가변 데이터의 길이를 추출합니다.
                        int variable_data_len = (rxBuf[6] & 0xFF) * 100 + (rxBuf[7] & 0xFF);
                        current_packet_len = HEADER_LEN + variable_data_len; // 헤더 8바이트 + 가변 데이터 (테일 포함)
                        // uart0_printf("[TCP RX Debug] Header complete. Parsed variable_data_len: %d, Total packet len: %d\n", variable_data_len, current_packet_len);

                        // 4. 전체 패킷 길이가 버퍼 크기를 초과하는지 확인
                        if (current_packet_len > TCP_RECV_PACKET_SIZE)
                        {
                            // uart0_printf("[TCP RX] Error: Packet size (%d) exceeds buffer (%d). Discarding current packet.\n", current_packet_len, TCP_PACKET_SIZE);
                            bytes_received = 0; // 버퍼 초기화
                            continue;
                        }
                    }
                }

                // 2. 헤더를 모두 받았고, 나머지 데이터를 기다리는 중일 때
                if (bytes_received >= HEADER_LEN && bytes_received < current_packet_len)
                {
                    int bytes_to_read = min(available_bytes, current_packet_len - bytes_received);
                    tcpClient.readBytes(&rxBuf[bytes_received], bytes_to_read);
                    bytes_received += bytes_to_read;
                    // uart0_printf("[TCP RX Debug] State: Reading Body. Avail: %d, Read: %d, Recv: %d\n", available_bytes, bytes_to_read, bytes_received);
                }

                // 3. 패킷을 완전히 수신했을 때
                if (bytes_received >= current_packet_len && current_packet_len > 0)
                {
                    // uart0_printf("[TCP RX] Fully received packet. Checking validity...\n");
                    // 패킷 유효성 검사 (시작/끝 마커)
                    if (rxBuf[0] == (byte)0xFF && rxBuf[1] == (byte)0xFF && rxBuf[current_packet_len - 1] == (byte)0xFE)
                    {
                        int received_client_id = rxBuf[5] & 0xFF;                // 클라이언트 ID (tx[5])
                        int sequence_num = rxBuf[current_packet_len - 2] & 0xFF; // 시퀀스 번호 (0xFE 바로 앞)

                        if (received_client_id == board_id_hardcoding || received_client_id == 8)
                        { // 8은 브로드캐스트 ID
                            uart0_printf("[TCP RX] Valid packet. Total Size: %d bytes, Client ID: %d, Sequence: %d\n", current_packet_len, received_client_id, sequence_num);
                            // uart0_printf("  Header (8 bytes): ");
                            // for (int i = 0; i < 8; i++) {
                            //     uart0_printf("%02X ", rxBuf[i]);
                            // }
                            // uart0_printf("\n");

                            // uart0_printf("  Sub Header (8 bytes): ");
                            // for (int i = 8; i < 16; i++) {
                            //     uart0_printf("%02X ", rxBuf[i]);
                            // }
                            // uart0_printf("\n");

                            // uart0_printf("  Tail (2 bytes): %02X %02X\n", rxBuf[current_packet_len - 2], rxBuf[current_packet_len - 1]);
                        }
                        else{
                            uart0_printf("[TCP RX] Packet received for other Client ID (%d). Discarding content. Total Size: %d\n", received_client_id, current_packet_len);
                        }
                    }
                    else
                    {
                        // uart0_printf("[TCP RX] Error: Invalid packet markers (Start: %02X %02X, End: %02X). Discarding.\n", rxBuf[0], rxBuf[1], rxBuf[current_packet_len - 1]);
                    }
                    bytes_received = 0;     // 패킷 처리 완료, 다음 패킷을 위해 초기화
                    current_packet_len = 0; // 패킷 길이 초기화
                }
            }
        }
        delay(1); // 다른 태스크에 시간 할당
    }
}

void loop_checkSafeTCP()
{
    static unsigned long lastTcpConnectAttempt = 0;

    if (WiFi.status() != WL_CONNECTED)
    {
        return;
    }

    reconnectMqtt();
    client.loop();

    if (!tcpClient.connected())
    {
        if (tcpConnectInterval <= millis() - lastTcpConnectAttempt)
        {
            if (0 < storedTcpServerIp.length())
            {
                if (tcpClient.connect(storedTcpServerIp.c_str(), TCP_SERVER_PORT))
                {
                    receivedTcpServerIpAddress = storedTcpServerIp;
                    tcpClient.setNoDelay(true); // Nagle 알고리즘 비활성화
                }
            }
            if (!tcpClient.connected() && 0 < receivedTcpServerIpAddress.length())
            {
                if (tcpClient.connect(receivedTcpServerIpAddress.c_str(), TCP_SERVER_PORT))
                {
                    tcpClient.setNoDelay(true); // Nagle 알고리즘 비활성화
                }
            }
            lastTcpConnectAttempt = millis();
        }
    }
}

#endif // _LIB_TCPCLIENT_H_