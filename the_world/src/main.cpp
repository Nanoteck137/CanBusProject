
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "FreeRTOS.h"
#include "task.h"

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

    stdio_uart_init();
    stdio_set_driver_enabled(&debug_driver, true);
}

const uint8_t PACKET_START = 0x4e;

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

void usb_thread(void* ptr)
{
    do
    {
        tud_task();
        vTaskDelay(1);
    } while (1);
}

enum class PacketState
{
    FoundStart,
};

void read_from_cmd(uint8_t* buffer, uint32_t len)
{
    while (tud_cdc_n_available(PORT_CMD) < len)
        ;

    tud_cdc_n_read(PORT_CMD, buffer, len);
}

static uint8_t buffer[256];

void test_thread(void* ptr)
{
    do
    {
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

        if (tud_cdc_n_available(PORT_CMD) > 0)
        {
            uint8_t start;
            // tud_cdc_n_read(PORT_CMD, &start, 1);
            read_from_cmd(&start, 1);

            if (start == PACKET_START)
            {
                printf("Got PACKET Start\n");

                uint8_t buf[3];
                read_from_cmd(buf, 3);

                uint8_t pid = buf[0];
                uint8_t typ = buf[1];
                uint8_t data_len = buf[2];

                printf("Got HEADER\n");

                // TODO(patrik): Read data

                read_from_cmd(buffer, (uint32_t)data_len);

                uint8_t checksum[2];
                read_from_cmd(checksum, sizeof(checksum));

                uint16_t check = checksum[1] << 8 | checksum[0];

                printf("PID: %d TYP: %d LEN: %d Checksum: 0x%x\n", pid, typ,
                       data_len, check);
            }
        }

        vTaskDelay(1);
    } while (1);
}

static TaskHandle_t usb_thread_handle;
static TaskHandle_t test_thread_handle;

int main()
{
    init_system();

    xTaskCreate(usb_thread, "USB Thread", configMINIMAL_STACK_SIZE, nullptr,
                tskIDLE_PRIORITY + 2, &usb_thread_handle);
    xTaskCreate(test_thread, "Test Thread", configMINIMAL_STACK_SIZE, nullptr,
                tskIDLE_PRIORITY + 1, &test_thread_handle);

    vTaskStartScheduler();
}

extern "C" void vApplicationTickHook() {}

extern "C" void vApplicationStackOverflowHook(TaskHandle_t Task,
                                              char* pcTaskName)
{
    panic("stack overflow (not the helpful kind) for %s\n", *pcTaskName);
}

extern "C" void vApplicationMallocFailedHook() { panic("Malloc Failed\n"); }
