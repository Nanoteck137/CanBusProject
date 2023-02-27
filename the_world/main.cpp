#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "class/cdc/cdc_device.h"
#include "hardware/gpio.h"
#include "hardware/timer.h"
#include "pico/stdlib.h"
#include "pico/stdio/driver.h"

#include "bsp/board.h"
#include "tusb.h"

const uint LEDPIN = 25;

// NOTE(patrik):
//   - USB for Serial Communication
//   - UART for Debugging

const uint8_t PORT_CMD = 0;
const uint8_t PORT_DEBUG = 1;

static void debug_driver_output(const char* buf, int length)
{
    tud_cdc_n_write(PORT_DEBUG, buf, length);
    tud_cdc_n_write_flush(PORT_DEBUG);
}

stdio_driver_t debug_driver = {
    .out_chars = debug_driver_output,
};

int main()
{
    // stdio_init_all();

    board_init();
    tusb_init();

    stdio_set_driver_enabled(&debug_driver, true);

    uint64_t last = time_us_64();

    while (1)
    {
        tud_task();

        uint64_t current = time_us_64();
        if ((current - last) > 1000 * 1000)
        {
            printf("Testing %d\n", tud_cdc_n_connected(PORT_CMD));
            last = time_us_64();
        }
    }
}
