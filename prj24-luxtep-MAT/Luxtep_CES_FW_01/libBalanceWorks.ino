
void loop_balanceWorks(int x_len, int y_len) {
    LED_Object cur_cop = getLastCOP();

    uart0_printf("COP Y = %d \n", cur_cop.y);

    int x_size = x_len;
    int y_size = y_len;

    int x_valid_start = 0;
    int x_valid_len = x_len;

    if (cur_cop.y == -1)
        return;

    int pct_south = cur_cop.y * 100 / 28;
    int pct_north = 100 - pct_south;

    int level_south = pct_south * x_valid_len / 100;
    int level_north = x_valid_len - level_south;

    for (int x = x_valid_start; x < (x_valid_start + level_north); x++) {
        ledBlurBuf[30 * 0 + x] = 240;  // rd
    }

    for (int x = x_valid_start; x < (x_valid_start + level_south); x++) {
        ledBlurBuf[30 * 29 + x] = 240;  // rd
    }
}
