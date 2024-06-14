
void loop_balanceWorks() {
    LED_Object cur_cop = getLastCOP();

    Serial.printf("COP Y = %d \n", cur_cop.y);

    int x_size = 30;
    int y_size = 30;

    int x_valid_start = 6;
    int x_valid_len = 16;

    if(cur_cop.y == -1)
        return;

    int pct_south = cur_cop.y * 100 / 28;
    int pct_north = 100 - pct_south;

    int level_south = pct_south * x_valid_len / 100;
    int level_north = x_valid_len - level_south;

    for(int x = x_valid_start ; x < (x_valid_start + level_north) ; x++) {
        inpol_buf[30 * 0 + x] = 240;
    }

    for(int x = x_valid_start ; x < (x_valid_start + level_south) ; x++) {
        inpol_buf[30 * 29 + x] = 240;
    }

}

