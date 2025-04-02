#ifndef _LIB_LEDWORKS_28x35_H_
#define _LIB_LEDWORKS_28x35_H_

#include <Arduino.h>
#include <NeoPixelBus.h>
// #include <gpio_types.h>


// How many NeoPixels are attached to the Arduino?
#define LED_COUNT 252
// #define LED_COUNT (28 * 2)
#define LED_COUNT_3 (224 + 2)  // 2 : not matrix but indicating LED

#define NUM_strip 4
#define NUM_LEDS 252

// Declare our NeoPixel strip object:
const gpio_num_t led_gpio_pins[4] = {
    static_cast<gpio_num_t>(8),
    static_cast<gpio_num_t>(21),
    static_cast<gpio_num_t>(14),
    static_cast<gpio_num_t>(13)};

NeoPixelBus<NeoGrbFeature, NeoEsp32LcdX8Ws2812xMethod> strip[NUM_strip] = {
    // {NUM_LEDS, 47},
    {NUM_LEDS, 8},
    {NUM_LEDS, 21},
    {NUM_LEDS, 14},
    {226, 13}};

//  LED Buffer
#define NUM_LED_IN_1_BOARD SIZE_X *(SIZE_Y + 1)
byte ledSrcBuf[NUM_LED_IN_1_BOARD];   // interpolation buffer
byte ledBlurBuf[NUM_LED_IN_1_BOARD];  // interpolation buffer

//  OSD Buffer
byte osdBuffer[NUM_LED_IN_1_BOARD];
byte *pMyOSDBuf;

void readForce();
void interpolateForce();
void drawIndicator();
void drawPixels();

void setup_LEDBuffers() {
    memset(ledBlurBuf, 0, NUM_LED_IN_1_BOARD);
    memset(ledSrcBuf, 0, NUM_LED_IN_1_BOARD);

    memset(osdBuffer, 0, NUM_LED_IN_1_BOARD);
    pMyOSDBuf = osdBuffer;
}

int read_count = 0;
// fill the ledSrcBuf[NUM_LED_IN_1_BOARD] from the forceBuffer_rd[SIZE_X * SIZE_Y]
void fillADC2LED() {
    memcpy(ledSrcBuf, forceBuffer_rd, SIZE_X * SIZE_Y);
    memcpy(ledSrcBuf + SIZE_X * SIZE_Y, forceBuffer_rd - SIZE_X, SIZE_X);

    read_count++;
}

bool setLedColor(int x, int y, byte r, byte g, byte b) {
    if (((x < 0) || (SIZE_X < x)) || ((y < 0) || (SIZE_Y < y))) {
        return false;
    }

    // REMOVE IT !!! ==>  MAKE LED WEAKER WHILE TESTING
    {
        int div_scale = 2;  // 0: relase-max brightness, 3: testing-min brightness
        r >>= div_scale;
        g >>= div_scale;
        b >>= div_scale;
    }

    int pcb_led_index = x + y * SIZE_X;
    int group_index = 0;
    int strip_index = 0;

    group_index = pcb_led_index / 252;
    strip_index = pcb_led_index % 252;

    // if (group_index == 0)
    //     strip[group_index].SetPixelColor(strip_index - 1, RgbColor(g, b, r));  // g, b, r
    // else
    strip[group_index].SetPixelColor(strip_index, RgbColor(r, g, b));  // r, g, b

    // strip[group_index].setPixelColor(strip_index, strip[group_index].Color(r, g, b));

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

int rgb_led[2];

int indi_1_r = 100;
int indi_1_g = 0;
int indi_1_b = 100;

void drawIndicator() {
#define LED_IND_GROUP_ID 3

    // strip[group_index].setPixelColor(strip_index, strip[group_index].Color(r, g, b));
    strip[LED_IND_GROUP_ID].SetPixelColor(224, RgbColor(indi_1_r, indi_1_g, indi_1_b));
    strip[LED_IND_GROUP_ID].SetPixelColor(225, RgbColor(50, 50, 50));
}

long draw_count = 0;
void drawPixels() {
    int wait_ms = 2;

    byte r, g, b;

    for (int y = 0; y < SIZE_Y + 1; y++) {
        for (int x = 0; x < SIZE_X; x++) {
            int led_index = y * SIZE_X + x;

            //  check if OSD1 area
            bool has_osd_to_draw = false;
            if ((osd_start_y <= y) && (y < osd_start_y + osd_height)) {
                if ((osd_start_x <= x) && (x < osd_start_x + osd_width)) {
                    has_osd_to_draw = true;
                }
            }

            //  Draw OSD1
            if ((has_osd_to_draw == true) && (pMyOSDBuf[led_index] != 0xEF)) {
                rgb_8to24(pMyOSDBuf[led_index], r, g, b);  // convert 8bit to 24bit
                setLedColor(x, y, r, g, b);
                // uart0_printf("[x=%d, y=%d] rgb = %d, r=%d, g=%d, b=%d \n", x, y, pMyOSDBuf[index], r, g, b);  // convert 8bit to 24bit
            }
            //  Draw Force
            else {
                // int force2led_value = draw_count % 240;

                int force2led_value = ledBlurBuf[led_index];  // rd

                convForce2RGB(force2led_value, r, g, b);
                setLedColor(x, y, r, g, b);  // C to Gled_index // temp
            }
        }
    }

    //  LED DMA TRIGGER ON
    for (int g = 0; g < 4; g++) {
        // uart0_printf("[%8d]strip[%d].Show(); \n", millis(), g);
        strip[g].Show();

        for (int i = 0; i < 7; i++) {
            vTaskDelay(1);
        }
    }

    draw_count++;
}

void clearPixels() {
    for (int g = 0; g < 4; g++) {
        strip[g].ClearTo(RgbColor(0, 0, 0));
    }
}

int blurLEDwithADCPos(int width, int x, int delta_x, int height, int y, int delta_y) {
    //  check size limit
    if (((x + delta_x) < 0) || ((y + delta_y) < 0))
        return -1;
    if ((width <= (x + delta_x)) || (height <= (y + delta_y)))
        return -1;

    int cell_index = 0;

    cell_index = SIZE_X * (y + delta_y) + (x + delta_x);

    if (0 < ledSrcBuf[cell_index])
        return -1;

    /*
                            * north
                            [North cell]

        north west          my LED              north east
        [West cell]         [This cell]         [East cell]
        south west          my LED              south east

                            [South cell]
                            south

    */
    /*
        * north LED
        [This cell]
        * south LED

    */
    int led_index_south = 0;
    int led_index_north = 0;

    led_index_south = SIZE_X * (y + delta_y) + (x + delta_x);
    led_index_north = SIZE_X * (y + delta_y + 1) + (x + delta_x);

    int center_index = 0;
    center_index = SIZE_X * y + x;

    int blur_value = ledSrcBuf[center_index] >> 0;

    if (delta_x == -1) {
        ledBlurBuf[led_index_north] = blur_value;  // rd
        ledBlurBuf[led_index_south] = blur_value;  // rd
    } else if (delta_x == 1) {
        ledBlurBuf[led_index_north] = blur_value;  // rd
        ledBlurBuf[led_index_south] = blur_value;  // rd
    } else if (delta_x == 0) {
        switch (delta_y) {
            case -1:
                ledBlurBuf[led_index_south] = blur_value;  // rd
                break;
            case 1:
                ledBlurBuf[led_index_north] = blur_value;  // rd
                break;
        }
    }

    // uart0_printf("(x=%d, y=%d) to (x+delta=%d, y+delta=%d), adc=%d, blur=%d \r\n",
    //               x, y, (x+delta_x), (y+delta_y), ledSrcBuf[center_index], blur_value);

    return blur_value;
}

void blurObjectOutline() {
    int cell_index = 0;

    for (int y = 0; y < (SIZE_Y + 0); y++) {
        for (int x = 0; x < SIZE_X; x++) {
            cell_index = SIZE_X * y + x;

            if (ledSrcBuf[cell_index] == 0)
                continue;

            //  influencing to the neighbor cells of force buffer value.
            // blurLEDwithADCPos(int width, int x, int delta_x, int height, int y, int delta_y);
            blurLEDwithADCPos(SIZE_X, x, -1, SIZE_Y, y, 0);

            blurLEDwithADCPos(SIZE_X, x, 0, SIZE_Y, y, -1);
            // blurLEDwithADCPos(SIZE_X, x,  0, SIZE_Y, y,  0);
            blurLEDwithADCPos(SIZE_X, x, 0, SIZE_Y, y, 1);

            blurLEDwithADCPos(SIZE_X, x, 1, SIZE_Y, y, 0);

            taskYIELD();
        }
    }
}

void draw_ledObjects() {
    uart0_printf("L");

    memset(ledBlurBuf, 0, NUM_LED_IN_1_BOARD);

    blurObjectOutline();  // blur outline, 46us~48us typically. And 259, 100us exceptioanlly.

    taskYIELD();
    // int64_t cur_snap = esp_timer_get_time();  // 마이크로초 단위로 타이머 초기화
    // int64_t duration_us = esp_timer_get_time() - cur_snap;
    // uart0_printf("[%8d] draw_ledObjects end, dur= %5lld\n", millis(), duration_us);

    // clearPixels(); // suspicious if it's redundant.

    // loop_fsrled(SIZE_X, SIZE_Y);  // draw COP
    // loop_balanceWorks(SIZE_X, SIZE_Y);

    drawIndicator();

    drawPixels();  // 픽셀 그려주는 작업
    uart0_printf("l");
}

#endif  // _LIB_LEDWORKS_28x35_H_
