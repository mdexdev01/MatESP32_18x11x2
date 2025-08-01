#include <Arduino.h>

// esp32-s3-mqtt_v03.ino
// Version: Ver.2.2 (Final Naming)
// Author: mdex.co.kr
// Date: 2025-07-22

// 이 파일은 프로그램의 진입점 역할만 합니다.
// 모든 실질적인 로직은 network_task.h에서 시작됩니다.
#include <network_task.h>
int board_id_hardcoding = 7;

// setup()과 loop()는 아두이노 프레임워크의 요구사항이므로 남겨둡니다.
void setup()
{
    // 모든 네트워크 초기화 프로세스를 시작합니다.
    setup_network_tasks();
}

void loop()
{
    // 모든 네트워크 반복 작업을 실행합니다.
    loop_network_tasks();
}