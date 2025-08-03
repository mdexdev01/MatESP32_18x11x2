# 프로젝트 코드 개선 계획 (v6)

## 개요

`RequirementsHistory.md`에 명시된 작업 순서에 따라, 코드의 안정성, 성능, 유지보수성 향상을 위한 구체적인 리팩토링 계획을 아래와 같이 재정리합니다.

---

### 1. 명명 규칙 및 코드 스타일 리팩토링 (Naming & Style Refactoring)

가장 먼저 코드의 가독성을 높여 이후의 리팩토링 작업을 용이하게 만듭니다. 아래 제안은 `pio_Luxtep_03`의 `src/main.cpp` 및 관련 헤더 파일을 기준으로 실제 적용된 규칙을 반영합니다.

#### 1.1. 전역 변수 및 핸들 (Global Variables & Handles)

- **원칙**: `g_[타입]_[역할]` 또는 `g_[타입][역할]` 형식의 명명 규칙을 적용하여 변수의 종류와 역할을 명확히 합니다. 일반 상태 변수는 `g_is[상태]` 형식을 유지합니다.

| 분류 | 기존 (Before) | 실제 적용 (After) | 설명 |
| :--- | :--- | :--- | :--- |
| **Task Handles** | `Task_Core0` | `g_taskHandle_serial` | `g_[타입]_[역할]` 형식 적용 |
| | `hTask_UART0` | `g_taskHandle_uart0Event` | 일관성 유지 |
| | `hTask_UART1` | `g_taskHandle_uart1Event` | 일관성 유지 |
| | `receiverTaskHandle` | `g_taskHandle_networkReceiver` | 일관성 유지 |
| **Semaphore Handles** | `semaForceBuf_rd` | `g_semaphore_adcDataReady` | `g_[타입]_[역할]` 형식 적용 |
| | `semaSendPermit` | `g_semaphore_rs485SendPermit` | 일관성 유지 |
| | `semaTX1` | `g_semaphore_uart1Tx` | 일관성 유지 |
| **Event Queues** | `UART0_EventQueue` | `g_eventQueue_uart0` | `g_[타입]_[역할]` 형식 적용 |
| | `UART1_EventQueue` | `g_eventQueue_uart1` | 일관성 유지 |
| **Button States** | `TactButtons` | `g_buttons` | `g_[타입]` 형식 적용 (배열) |
| **State Variables** | `is_ManualMode` | `g_isManualMode` | `is/has` 접두사 유지 |
| | `isSensorDataFilled` | `g_isSensorDataReady` | `is/has` 접두사 유지 |
| | `isNoLEDMode` | `g_isLedDisabled` | `is/has` 접두사 유지 |
| | `isOTACommand` | `g_isOtaModeRequested` | `is/has` 접두사 유지 |
| | `isPermitGranted` | `g_isPermitGranted` | `is/has` 접두사 유지 |
| | `isBoard0_duringPermitCycle` | `g_isBoard0DuringPermitCycle` | `is/has` 접두사 유지 |
| **Value Variables** | `board_id_hardcoding` | `g_BoardIdHardcoded` | `g_[타입][역할]` 형식 적용 |
| | `loop_count` | `g_CounterMainLoop` | `g_[타입][역할]` 형식 적용 |
| | `g_maxBoardId` | `g_maxBoardId` | (유지) |
| | `pumpCount` | `g_pumpCount` | `g_[타입]` 형식 적용 |
| | `tickPermitLast` | `g_lastPermitTick` | `g_[타입][역할]` 형식 적용 |
| | `loop_network_count` | `g_networkLoopCounter` | `g_[타입][역할]` 형식 적용 |
| | `g_tx0Count` | `g_tx0Count` | (유지) |
| | `g_button1State` | `g_button1State` | (유지) |
| | `g_isButtonEventPending` | `g_isButtonEventPending` | (유지) |
| | `g_ledDrawingLoopCounter` | `g_ledDrawingLoopCounter` | (유지) |
| | `serialCommand` | `serialCommand` | (유지) |
| | `mqttClientId` | `mqttClientId` | (유지) |
| | `myFormattedMacAddress` | `myFormattedMacAddress` | (유지) |
| | `currentAppID` | `currentAppID` | (유지) |
| | `receivedTcpServerIpAddress` | `receivedTcpServerIpAddress` | (유지) |
| | `storedTcpServerIp` | `storedTcpServerIp` | (유지) |
| | `bootTimestamp` | `bootTimestamp` | (유지) |
| | `otaUpdateRequested` | `otaUpdateRequested` | (유지) |

#### 1.2. 함수 및 태스크 (Functions & Tasks)

- **원칙**: 태스크는 `task[역할]` 형식으로, 일반 함수는 `동사[목적어]` 형식의 `camelCase`를 적용합니다.

| 기존 (Before) | 실제 적용 (After) | 설명 |
| :--- | :--- | :--- |
| `loopADCRead` | `taskSensorAndLed` | `task[역할]` 형식 적용 |
| `loopDrawLED` | `taskLedDrawing` | 일관성 유지 |
| `pumpSerial` | `taskSerialCommunication` | 일관성 유지 |
| `uart0_event_task` | `taskUart0Event` | 일관성 유지 |
| `uart1_event_task` | `taskUart1Event` | 일관성 유지 |
| `receiverTask` | `taskNetworkReceiver` | 일관성 유지 |
| `readDipSwitch` | `readDipSwitches` | 일반 함수는 `동사[목적어]` 형식 유지 |
| `setup_gpioWork` | `setupHardwareButtons` | 일반 함수는 `동사[목적어]` 형식 유지 |
| `loop_gpioWork` | `handleHardwareButtons` | 일반 함수는 `동사[목적어]` 형식 유지 |
| `isr_Buttons` | `button_isr` | 일반 함수는 `동사[목적어]` 형식 유지 |
| `checkTimeLen` | `checkButtonPressDuration` | 일반 함수는 `동사[목적어]` 형식 유지 |
| `onRelease` | `handleButtonRelease` | 일반 함수는 `동사[목적어]` 형식 유지 |
| `disable_uart_rx_events` | `disableUartRxEvents` | 일반 함수는 `동사[목적어]` 형식 유지 |
| `enable_uart_rx_events` | `enableUartRxEvents` | 일반 함수는 `동사[목적어]` 형식 유지 |
| `setup_RS485` | `setupRs485` | 일반 함수는 `동사[목적어]` 형식 유지 |
| `wait_tx_done_async` | `waitForTxDoneAsync` | 일반 함수는 `동사[목적어]` 형식 유지 |
| `sendPacket0` | `sendPacket0` | (유지) |
| `setDE` | `setDE` | (유지) |
| `sendPacket1` | `sendPacket1` | (유지) |
| `processRX0` | `processRx0` | 일반 함수는 `동사[목적어]` 형식 유지 |
| `processRX1` | `processRx1` | 일반 함수는 `동사[목적어]` 형식 유지 |
| `printUart1` | `printUart1` | (유지) |
| `considerToPermit` | `considerToPermit` | (유지) |
| `osdSimulation` | `osdSimulation` | (유지) |
| `createAndSendTcpPacket` | `createAndSendTcpPacket` | (유지) |
| `loop_checkSafeTCP` | `loop_checkSafeTCP` | (유지) |
| `processTCP_RX` | `processTCP_RX` | (유지) |
| `reconnectMqtt` | `reconnectMqtt` | (유지) |
| `callback` | `callback` | (유지) |
| `runHttpOtaSequence` | `runHttpOtaSequence` | (유지) |
| `setup_wifi_and_nvs` | `setup_wifi_and_nvs` | (유지) |
| `setup_mqtt_client` | `setup_mqtt_client` | (유지) |
| `setup_tcp_client_task` | `setup_tcp_client_task` | (유지) |
| `handleSerialCommand` | `handleSerialCommand` | (유지) |
| `setup_network_tasks` | `setup_network_tasks` | (유지) |
| `loop_network_tasks` | `loop_network_tasks` | (유지) |
| `setup_softap_launch` | `setup_softap_launch` | (유지) |
| `loop_config_mode` | `loop_config_mode` | (유지) |
| `handleRoot` | `handleRoot` | (유지) |
| `handleSave` | `handleSave` | (유지) |
| `isSSID2_4GHz` | `isSSID2_4GHz` | (유지) |
| `connectToWiFiFromNVS` | `connectToWiFiFromNVS` | (유지) |
| `WiFiEvent` | `WiFiEvent` | (유지) |
| `saveTcpIpToNVS` | `saveTcpIpToNVS` | (유지) |
| `loadTcpIpFromNVS` | `loadTcpIpFromNVS` | (유지) |
| `saveBoardIdToNVS` | `saveBoardIdToNVS` | (유지) |
| `loadBoardIdFromNVS` | `loadBoardIdFromNVS` | (유지) |

#### 1.3. 상수 및 매크로 (Constants & Macros)

- **원칙**: `UPPER_SNAKE_CASE` 적용, "매직 넘버"를 이름 있는 상수로 대체.

| 기존 (Before) | 실제 적용 (After) | 설명 |
| :--- | :--- | :--- |
| `FIRMWARE_VERSION` | `FIRMWARE_VERSION_STR` | `const char*` 변수로 변경 |
| `CORE_0`, `CORE_1` | `PRO_CORE`, `APP_CORE` | ESP32의 공식적인 코어 이름으로 변경 |
| `NUM_strip` | `NUM_LED_STRIPS` | `strip`이 무엇인지(LED Strip) 명확히 하고 `UPPER_SNAKE_CASE` 적용 |
| `BoardID_MAX` | `MAX_BOARD_ID` | 단어 순서 변경으로 가독성 향상 |
| `(magic number) 10` | `DEFAULT_TASK_PRIORITY` | 태스크 우선순위 기본값 정의 |
| `(magic number) 20` | `UART0_EVENT_QUEUE_SIZE` | UART0 이벤트 큐 크기 정의 |
| `(magic number) 120` | `PERMIT_ACK_TIMEOUT_MS` | RS485 응답 타임아웃 정의 |
| `DEFAULT_APPID` | `DEFAULT_APPID` | `apple32`로 값 변경 |
| `pin_LED_WS2812C` | `INDICATOR_LED_PIN` | 명확한 이름으로 변경 |
| `pin_TACT0`, `pin_TACT1`, `pin_TACT2` | `TACT_SWITCH_0_PIN`, `TACT_SWITCH_1_PIN`, `TACT_SWITCH_2_PIN` | 명확한 이름으로 변경 |
| `TACT_NUM` | `NUM_BUTTONS` | 명확한 이름으로 변경 |
| `LONG_CLICK_MS` | `LONG_PRESS_DURATION_MS` | 명확한 이름으로 변경 |
| `VERY_LONG_CLICK_MS` | `VERY_LONG_PRESS_DURATION_MS` | 명확한 이름으로 변경 |

#### 1.4. 주석 (Comments)

- **원칙**: 코드의 동작(`What`)이 아닌, **"왜(Why)"** 그렇게 작성했는지를 설명하고, 모든 주석을 영문으로 통일.

| 기존 (Before) | 실제 적용 (After) |
| :--- | :--- |
| `// 세마포어 핸들 생성` | `// Create a semaphore to signal that ADC data is ready for processing.` |
| `// GPIO 47 비활성화 - 예외 처리` | `// WORKAROUND: Disable GPIO47 as it's incorrectly wired on the PCB.` |

---

### 2. `loopADCRead` 함수 분리 (Refactor `loopADCRead` Function)

- **문제점**: `loopADCRead` 함수가 ADC 측정, LED 출력, 버튼 처리 등 너무 많은 책임을 가지고 있어 단일 책임 원칙(Single Responsibility Principle)에 위배됩니다.
- **개선 방안**: 해당 함수를 역할에 따라 여러 개의 작은 함수로 분리합니다.
    - `readAdcSensorValues()`: ADC 스캔 및 데이터 버퍼링
    - `updateLedMatrix()`: 센서 값에 따른 LED 매트릭스 업데이트
    - `handleHardwareButtons()`: 물리적 버튼 입력 처리
    - `checkOtaStatus()`: OTA 모드 진입 조건 확인

---

### 3. 전역 변수 사용 억제 (Suppress Global Variables)

- **문제점**: 특히 PC 서버(`proc_tcp_mqtt_ota_v03`) 코드에서 수많은 전역 변수를 통해 상태를 공유하고 있어, 코드의 흐름을 추적하기 어렵고 잠재적인 버그의 원인이 됩니다.
- **개선 방안**:
    - **구조체/클래스 활용**: 관련된 데이터와 상태를 구조체나 클래스로 묶어 관리합니다. 예를 들어, ESP32 펌웨어에서는 `SystemStatus` 구조체를, PC 서버에서는 `ClientManager` 클래스를 만들어 관련 변수들을 멤버로 포함시킬 수 있습니다.
    - **함수 파라미터 전달**: 전역 변수를 직접 참조하는 대신, 필요한 데이터를 함수의 파라미터로 명시적으로 전달하는 방식을 사용합니다.

---

### 4. `delay()` 대신 `vTaskDelay()` 사용 (Use `vTaskDelay()`)

- **문제점**: `delay()` 함수는 태스크를 완전히 멈추게 만들어(Blocking), 다른 태스크의 실행을 막고 시스템 전체의 반응성을 저해합니다.
- **개선 방안**: `delay()`를 **`vTaskDelay()`**로 전면 교체합니다. `vTaskDelay()`는 대기하는 동안 RTOS 스케줄러가 다른 태스크를 실행할 수 있도록 CPU 제어권을 넘겨주므로, 멀티태스킹 환경에 필수적입니다.

---

### 5. 펌웨어 비효율성 해소 (Resolve Firmware Inefficiencies)

가장 중요하고 근본적인 개선 사항입니다.

- **문제점**:
    1.  **순차 처리**: ADC, RMT(LED), UART 통신이 데이터 깨짐 문제로 인해 순차적으로 실행되어 성능 병목이 발생합니다.
    2.  **Busy-Waiting (Polling)**: `if(g_isSensorDataReady == true) continue;` 와 같이 플래그 변수를 계속 확인하는 방식은 CPU 자원을 심각하게 낭비합니다.
- **개선 방안**:
    1.  **경쟁 상태(Race Condition) 해결**: 공유 데이터 버퍼(ADC 값, LED 버퍼 등)에 접근하는 코드 영역을 **뮤텍스(Mutex)** 또는 **세마포어(Semaphore)**로 보호하여 데이터 무결성을 보장합니다. 이를 통해 태스크들을 다시 안전하게 병렬로 실행할 수 있습니다.
    2.  **이벤트 기반(Event-driven) 설계로 전환**: Polling 방식 대신 FreeRTOS의 **태스크 알림(Task Notifications)**이나 **큐(Queue)**를 사용합니다. 예를 들어, 'ADC 측정 완료'라는 이벤트가 발생하면, 이 이벤트를 기다리던 'LED 업데이트 태스크'가 깨어나 작업을 수행하고 다시 대기 상태로 돌아가는 효율적인 구조로 변경합니다.