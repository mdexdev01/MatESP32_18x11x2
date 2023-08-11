/*
BLE Lib : C:\Users\mdex.sec01\AppData\Local\Arduino15\packages\esp32\hardware\esp32\2.0.8\libraries\BLE\
1. adv 
2. 측정
3. ble 전송
4. slave 이어짐, 끊어짐 인식 
*/

void setup() {
  Serial.begin(230400);
  setup_advGrib();
}


void loop() {
  loop_advGrib();

  //rle_test();
}
