
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

// TODO(patrik):
//   - Import FreeRTOS
//   - Packets:
//      - Parse Packets
//      - Execute Packets
//      - Send back result
//   - Can Bus
//     - Get MCP2515 working

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

void init_system()
{
    board_init();
    tusb_init();

    stdio_set_driver_enabled(&debug_driver, true);
}

const uint8_t SYN = 0x01;
const uint8_t SYN_ACK = 0x02;
const uint8_t ACK = 0x03;
const uint8_t PING = 0x04;
const uint8_t PONG = 0x05;
const uint8_t UPDATE = 0x06;

struct Packet
{
    uint8_t typ;
};

static bool last_connected = false;

int recv_packet(Packet* packet) { return 0; }

int main()
{
    init_system();

    uint64_t last = time_us_64();

    while (1)
    {
        tud_task();

        bool connected = tud_cdc_n_connected(PORT_CMD);

        if (connected && !last_connected)
        {
            printf("User Connected\n");
        }

        if (!connected && last_connected)
        {
            printf("User Disconnect\n");
        }

        last_connected = connected;

        uint64_t current = time_us_64();
        if (current - last > 1000 * 1000)
        {
            printf("Test\n");
            last = current;
        }
    }
}
