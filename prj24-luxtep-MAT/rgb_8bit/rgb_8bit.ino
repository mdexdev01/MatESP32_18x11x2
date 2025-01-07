#include <stdio.h>

// 팔레트 구조체 정의
typedef struct {
  unsigned char r;
  unsigned char g;
  unsigned char b;
} RGB;

void setup() {
  Serial.begin(961600, SERIAL_8N1);

  // 216가지 웹 안전 색상을 저장할 팔레트 배열 선언
  RGB palette[216];

  // 팔레트에 웹 안전 색상 채우기
  int index = 0;
  for (int r = 0; r <= 255; r += 51) {
    for (int g = 0; g <= 255; g += 51) {
      for (int b = 0; b <= 255; b += 51) {
        palette[index].r = r;
        palette[index].g = g;
        palette[index].b = b;
        index++;
      }
    }
  }

  // 팔레트 내용 출력 (확인용)
  for (int i = 0; i < 216; i++) {
    Serial.printf("index %d: R=%d, G=%d, B=%d\n", i, palette[i].r, palette[i].g, palette[i].b);
  }

  // return 0;
}


void setup2() {
  // put your setup code here, to run once:

}

void loop() {
  // put your main code here, to run repeatedly:

}

