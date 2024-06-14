< 예제 시나리오 >
500ms (밀리세컨드) 로 시작
10초 후에 10um (마이크로 세컨드)로 전환
12초 후에 1000ms 로 전환

주의할 점은 최소 주기는 10us 로 되어 있습니다. 1us를 설정하면 에러 납니다.
클락이 80MHz로 되어 있는데 이걸 바꿔주면 1us가 가능할 수도 있습니다.


타이머 ISR 에서 카운트가 1씩 증가합니다.
void ARDUINO_ISR_ATTR onTimer() {
    isrCountTimer++;
    // ets_printf("isr[wdt] %d, (%d) \n", isrCountTimer, millis());
}

카운트가 증가한 숫자를 찍어보시면 제대로 동작 여부를 알수 있습니다.
카운트는 임의로 400ms 에 한번씩 찍어보게 되어 있습니다.
