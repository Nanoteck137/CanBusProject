#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "class/cdc/cdc_device.h"
#include "hardware/gpio.h"
#include "hardware/timer.h"
#include "pico/stdlib.h"

#include "bsp/board.h"
#include "tusb.h"

const uint LEDPIN = 25;

// NOTE(patrik):
//   - USB for Serial Communication
//   - UART for Debugging

static void cdc_task(void)
{
    tud_cdc_n_write_str(0, "Hello World\r\n");
    tud_cdc_n_write_flush(0);

    tud_cdc_n_write_str(1, "Hello World from Other\r\n");
    tud_cdc_n_write_flush(1);

    // sleep_ms(0);
}

int main()
{
    // stdio_init_all();

    board_init();
    tusb_init();

    // typedef struct TU_ATTR_PACKED
    // {
    //     uint32_t bit_rate;
    //     uint8_t
    //         stop_bits;  ///< 0: 1 stop bit - 1: 1.5 stop bits - 2: 2 stop
    //         bits
    //     uint8_t parity; ///< 0: None - 1: Odd - 2: Even - 3: Mark - 4: Space
    //     uint8_t data_bits; ///< can be 5, 6, 7, 8 or 16
    // } cdc_line_coding_t;

    uint64_t last = time_us_64();

    while (1)
    {
        tud_task();

        uint64_t current = time_us_64();
        if ((current - last) > 1000 * 1000)
        {
            cdc_task();
            last = time_us_64();
        }
    }
}
