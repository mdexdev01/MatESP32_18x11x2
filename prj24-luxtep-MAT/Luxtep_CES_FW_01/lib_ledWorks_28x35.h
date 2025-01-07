
// How many NeoPixels are attached to the Arduino?
#define LED_COUNT 252
// #define LED_COUNT (28 * 2)
#define LED_COUNT_3 (224 + 2)  // 2 : not matrix but indicating LED

// Declare our NeoPixel strip object:
// Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);
// Adafruit_NeoPixel strip[4];
#define NUM_strip 4
#define NUM_LEDS 252

NeoPixelBus<NeoGrbFeature, NeoEsp32LcdX8Ws2812xMethod> strip[NUM_strip] = {
    // {NUM_LEDS, 47},
    {NUM_LEDS, 8},
    {NUM_LEDS, 21},
    {NUM_LEDS, 14},
    {226, 13}};

void readForce();
void interpolateForce();
void drawIndicator();
void drawPixels();

bool setLedColor(int x, int y, byte r, byte g, byte b) {
    if (((x < 0) || (SIZE_X < x)) || ((y < 0) || (SIZE_Y < y))) {
        return false;
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

int read_count = 0;
void reorderADC2LED() {
    int cell_index = 0;
    int led_index_south = 0;
    int led_index_north = 0;

    memset(force_buf, 0, SIZE_X * SIZE_Y);
    memset(inpol_buf, 0, SIZE_X * (SIZE_Y + 1));

    for (int y = 0; y < SIZE_Y; y++) {
        for (int x = 0; x < SIZE_X; x++) {
            cell_index = SIZE_X * y + x;
            force_buf[cell_index] = ForceData[x][y];

            // if (0 < ForceData[x][y])
            //     Serial.printf("gara 2 force =%d (X%d, Y%d) \n", ForceData[x][y], x, y);

            if (force_buf[cell_index] == 0)
                continue;

            led_index_south = SIZE_X * y + x;
            led_index_north = SIZE_X * (y + 1) + x;

            inpol_buf[led_index_south] = force_buf[cell_index];
            inpol_buf[led_index_north] = force_buf[cell_index];
        }
    }

    read_count++;
}

int rgb_led[2];

int indi_1_r = 200;
int indi_1_g = 0;
int indi_1_b = 200;

void drawIndicator() {
#define LED_IND_GROUP_ID 3

    // strip[group_index].setPixelColor(strip_index, strip[group_index].Color(r, g, b));
    strip[LED_IND_GROUP_ID].SetPixelColor(224, RgbColor(indi_1_r, indi_1_g, indi_1_b));
    strip[LED_IND_GROUP_ID].SetPixelColor(225, RgbColor(0, 200, 200));
}

void overwriteOSD() {
    byte r, g, b;
    for (int y = osd_start_y; y < (osd_start_y + osd_height); y++) {
        for (int x = osd_start_x; x < (osd_start_x + osd_width); x++) {
            int index = y * SIZE_X + x;

            if (osd_buf[index] == 0xEF)
                continue;

            rgb_8to24(osd_buf[index], r, g, b);  // convert 8bit to 24bit
            // Serial.printf("[x=%d, y=%d] rgb = %d, r=%d, g=%d, b=%d \n", x, y, osd_buf[index], r, g, b);  // convert 8bit to 24bit
            setLedColor(x, y, r, g, b);
        }
    }
}

long draw_count = 0;
void drawPixels() {
    int wait_ms = 2;

    int force = draw_count % 240;
    byte r, g, b;

    for (int y = 0; y < SIZE_Y + 1; y++) {
        for (int x = 0; x < SIZE_X; x++) {
            int led_index = y * SIZE_X + x;

            //  check OSD area
            bool is_osd_area = false;
            if ((osd_start_y <= y) && (y < osd_start_y + osd_height)) {
                if ((osd_start_x <= x) && (x < osd_start_x + osd_width)) {
                    is_osd_area = true;
                }
            }

            //  Draw OSD 1
            if ((is_osd_area == true) && (osd_buf[led_index] != 0xEF)) {
                rgb_8to24(osd_buf[led_index], r, g, b);  // convert 8bit to 24bit
                setLedColor(x, y, r, g, b);
                // Serial.printf("[x=%d, y=%d] rgb = %d, r=%d, g=%d, b=%d \n", x, y, osd_buf[index], r, g, b);  // convert 8bit to 24bit
            }
            //  Draw Force
            else {
                force = inpol_buf[led_index];
                convForce2RGB(force, r, g, b);
                setLedColor(x, y, r, g, b);  // C to Gled_index // temp
            }
        }
    }

    // overwriteOSD();

    // Serial.printf("[%08d] led show go\n", millis());

    for (int g = 0; g < 4; g++) {
        strip[g].Show();
    }
    // Serial.printf("[%08d] led show end\n", millis());

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

    if (0 < force_buf[cell_index])
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

    int blur_value = force_buf[center_index] >> 0;

    if (delta_x == -1) {
        inpol_buf[led_index_north] = blur_value;
        inpol_buf[led_index_south] = blur_value;
    } else if (delta_x == 1) {
        inpol_buf[led_index_north] = blur_value;
        inpol_buf[led_index_south] = blur_value;
    } else if (delta_x == 0) {
        switch (delta_y) {
            case -1:
                inpol_buf[led_index_south] = blur_value;
                break;
            case 1:
                inpol_buf[led_index_north] = blur_value;
                break;
        }
    }

    // Serial.printf("(x=%d, y=%d) to (x+delta=%d, y+delta=%d), adc=%d, blur=%d \r\n",
    //               x, y, (x+delta_x), (y+delta_y), force_buf[center_index], blur_value);

    return blur_value;
}

void blurObjectOutline() {
    int cell_index = 0;

    for (int y = 0; y < (SIZE_Y + 0); y++) {
        for (int x = 0; x < SIZE_X; x++) {
            cell_index = SIZE_X * y + x;

            if (force_buf[cell_index] == 0)
                continue;

            //  influencing to the neighbor cells of value 0.
            // blurLEDwithADCPos(int width, int x, int delta_x, int height, int y, int delta_y);
            blurLEDwithADCPos(SIZE_X, x, -1, SIZE_Y, y, 0);

            blurLEDwithADCPos(SIZE_X, x, 0, SIZE_Y, y, -1);
            // blurLEDwithADCPos(SIZE_X, x,  0, SIZE_Y, y,  0);
            blurLEDwithADCPos(SIZE_X, x, 0, SIZE_Y, y, 1);

            blurLEDwithADCPos(SIZE_X, x, 1, SIZE_Y, y, 0);
        }
    }
}
