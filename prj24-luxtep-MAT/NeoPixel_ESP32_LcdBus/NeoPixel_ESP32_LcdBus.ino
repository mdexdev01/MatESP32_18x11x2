#include <NeoPixelBus.h>

#define NUM_strip 4
#define NUM_LEDS 252

NeoPixelBus<NeoGrbFeature, NeoEsp32LcdX8Ws2812xMethod> strip[NUM_strip] = {
    {NUM_LEDS, 47},
    {NUM_LEDS, 21},
    {NUM_LEDS, 14},
    {226, 13}
};

//  ADC Buffer
#define NUM_PWR 28  // X
#define NUM_SEN 34  // Y

#define NUM_COL NUM_PWR  // X, 28
#define NUM_ROW NUM_SEN  // Y, 34

#define SIZE_X NUM_COL
#define SIZE_Y NUM_ROW

bool setLedColor(int x, int y, byte r, byte g, byte b) {
    if (((x < 0) || (SIZE_X < x)) || ((y < 0) || (SIZE_Y < y))) {
        return false;
    }

    int pcb_led_index = x + y * SIZE_X;
    int group_index = 0;
    int strip_index = 0;

    group_index = pcb_led_index / 252;
    strip_index = pcb_led_index % 252;


    if(group_index == 0)
      strip[group_index].SetPixelColor(strip_index-1, RgbColor(g, b, r)); // g, b, r
    else 
      strip[group_index].SetPixelColor(strip_index, RgbColor(r, g, b)); // r, g, b

    return true;
}


uint32_t RGBCode(byte r, byte g, byte b) {
    uint32_t rgb = 0;

    rgb = r << 16 | g << 8 | b;

    return rgb;
}

void convForce2RGB(int force_val, byte &r_val, byte &g_val, byte &b_val) {
    // G(0, 255, 0) ==> Yellow(255, 255, 0) ==> Red(255, 0, 0)
    //  force_val range : probably 0~180

    int div_scale = 4;  // color range is divided 3 (G~Y, Y~O, O~R)

    if (240 < force_val)
        force_val = 240;

    if (force_val < 1) {
        r_val = 0;
        g_val = 0;
        b_val = 0;                // Zero
    } else if (force_val < 80) {  // G to Y
        r_val = force_val * 3;
        g_val = 160;
        b_val = 0;                 // C to G
    } else if (force_val < 160) {  // Y to R
        r_val = 255;
        g_val = 255 - (force_val - 80) * 3;
        b_val = 0;  // C to G
    } else {        // O to R
        r_val = 255;
        g_val = 0;
        b_val = 0;
    }
}

void setup() {
    Serial.begin(921600);

    // 모든 스트립 초기화
    for (int i = 0; i < NUM_strip; i++) {
        strip[i].Begin();
        strip[i].ClearTo(RgbColor(0, 0, 0));
        strip[i].Show();
    }

    Serial.println("All strip initialized.");
}

void loop() {
  drawPixels();

}

long draw_count = 0;
void drawPixels() {
    int wait_ms = 2;

    int force = draw_count % 240;
    byte r, g, b;

    for (int y = 0; y < SIZE_Y + 1; y++) {
        for (int x = 0; x < SIZE_X; x++) {
            int led_index = y * SIZE_X + x;

            if((2 < x) && (x < 16)) {
              if((0 <= y) && (y < 13)) {
                // force = inpol_buf[led_index];
                force = (y * 147) % 180 + 5;
                convForce2RGB(force, r, g, b);
                setLedColor(x, y, r, g, b);  // C to Gled_index // temp
              }
            }


        }
    }

    // overwriteOSD();

    // Serial.printf("[%08d] led show go\n", millis());

    for (int g = 0; g < 4; g++) {
        strip[g].Show();
    }
    // Serial.printf("[%08d] led show end\n", millis());

    delay(200);

    offPixels();
    delay(1200);

    draw_count++;
}

void offPixels() {
    for (int i = 0; i < NUM_strip; i++) {
        strip[i].ClearTo(RgbColor(0, 0, 0));
        strip[i].Show();
        delay(1); // 스트립 간 대기 시간
    }

}



void loop_old() {
    // 모든 스트립에 색상 설정
    for (int i = 0; i < NUM_strip; i++) {
        for (int j = 0; j < strip[i].PixelCount(); j++) {
          if(i == 0)
            strip[i].SetPixelColor(j-1, RgbColor(38, 9, 45)); // g, b, r
          // else
          //   strip[i].SetPixelColor(j, RgbColor(45, 38, 9)); // r, g, b
        }
    }

    // 모든 스트립 업데이트
    for (int i = 0; i < NUM_strip; i++) {
        strip[i].Show();
        delay(1); // 스트립 간 대기 시간
    }

    delay(200);

    // 모든 스트립 끄기
    for (int i = 0; i < NUM_strip; i++) {
        strip[i].ClearTo(RgbColor(0, 0, 0));
        strip[i].Show();
        delay(1); // 스트립 간 대기 시간
    }

    delay(1000);
}
