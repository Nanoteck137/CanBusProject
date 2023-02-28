
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
    uint8_t pid;
    uint8_t typ;
    uint8_t data_len;
    uint16_t checksum;
};

void usb_thread(void* ptr)
{
    do
    {
        tud_task();
        vTaskDelay(1);
    } while (1);
}

void read(uint8_t* buffer, uint32_t len)
{
    for (uint32_t i = 0; i < len; i++)
    {
        while (tud_cdc_n_available(PORT_CMD) < 1)
            ;

        tud_cdc_n_read(PORT_CMD, buffer + i, 1);
    }
}

uint8_t read_u8()
{
    uint8_t res;
    read(&res, sizeof(res));

    return res;
}

uint16_t read_u16()
{
    uint8_t res[2];
    read(res, sizeof(res));

    return (uint16_t)res[1] << 8 | (uint16_t)res[0];
}

static uint8_t data_buffer[256];

Packet parse_packet()
{
    uint8_t pid = read_u8();
    uint8_t typ = read_u8();
    uint8_t data_len = read_u8();

    read(data_buffer, (uint32_t)data_len);

    uint16_t checksum = read_u16();

    Packet packet;
    packet.pid = pid;
    packet.typ = typ;
    packet.data_len = data_len;
    packet.checksum = checksum;

    return packet;
}

void test_thread(void* ptr)
{
    do
    {
        if (tud_cdc_n_available(PORT_CMD) > 0)
        {
            uint8_t b = read_u8();
            if (b == PACKET_START)
            {
                Packet packet = parse_packet();
                printf("Got Packet: 0x%x %d\n", packet.typ, packet.data_len);
                for (int i = 0; i < packet.data_len; i++)
                {
                    if (i % 8 == 0)
                        printf("\n");
                    printf("0x%x ", data_buffer[i]);
                }
                printf("\n");
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
