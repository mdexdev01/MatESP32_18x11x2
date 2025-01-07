
void setup_fsrled() {
}

int cop_led_x = 0;
int cop_led_y = 0;
int cop_led_value = 0;

void loop_fsrled(int range_width, int range_height) {
    find_COP(range_width, range_height);
    fill_COP100();
}

void find_COP(int range_width, int range_height) {
    byte r, g, b;

    float cop_x = 0.0f;
    float cop_y = 0.0f;

    int cop_led_x = 0;
    int cop_led_y = 0;

    long sum_force = 0;
    long sum_force_pos_x = 0;
    long sum_force_pos_y = 0;

    for (int y = 0; y < range_height; y++) {
        for (int x = 0; x < range_width; x++) {
            int led_index = y * range_width + x;

            sum_force += inpol_buf[led_index];
            sum_force_pos_x += inpol_buf[led_index] * x;
            sum_force_pos_y += inpol_buf[led_index] * y;
        }
    }

    if (0 < sum_force) {
        cop_x = sum_force_pos_x / sum_force;
        cop_y = sum_force_pos_y / sum_force;

        cop_led_x = (int)cop_x;
        cop_led_y = (int)cop_y;

        COP_fifo_in(cop_led_x, cop_led_y, 240);

        // inpol_buf[30 * cop_led_y + cop_led_x] = 240;
    } else {
        COP_fifo_in(-1, -1, 0);
    }

    // Serial.printf("COP = (%2.2f, %2.2f) \r\n", cop_x, cop_y);
    // convForce2RGB(force, r, g, b);
    // setLedColor(cop_led_x, 28, 220, 0, 0);  // C to Gled_index
}

void COP_fifo_in(int x, int y, int strenth) {
    {
        // Serial.printf("[%3d]", fifo_next_index);
        for (int j = 0; j < cop_fifo_len; j++) {
            // Serial.printf("%3d,", COP_RingBuf[j].z);
        }
        // Serial.printf("\r\n");
    }

    fifo_in_index = fifo_next_index;
    COP_RingBuf[fifo_in_index] = LED_Object(x, y, strenth);

    fifo_next_index = fifo_in_index + 1;

    if (fifo_next_index == cop_fifo_len)
        fifo_next_index = 0;
}

void fill_COP100() {
    int fifo_offset = fifo_next_index;  // in fact, fifo_next_index is the offset of the next one. it means the oldest one.

    // Serial.printf("fifo offset = %d \r\n", fifo_offset);

    for (int i = 0; i < cop_fifo_len; i++) {
        int cop_led_x = COP_RingBuf[fifo_offset].x;
        int cop_led_y = COP_RingBuf[fifo_offset].y;
        int level = COP_RingBuf[fifo_offset].z;

        fifo_offset++;
        if (fifo_offset == cop_fifo_len)
            fifo_offset = 0;

        if (level == 0)
            continue;

        inpol_buf[30 * cop_led_y + cop_led_x] = i * int(240 / cop_fifo_len);
    }
}
