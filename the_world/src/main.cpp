
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "tusk.h"

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

static uint8_t data_buffer[256];
static size_t current_data_offset = 0;

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

uint8_t read_u8_from_data()
{
    uint8_t val = data_buffer[current_data_offset];
    current_data_offset++;

    return val;
}
uint16_t read_u16_from_data()
{
    uint16_t val = (uint16_t)data_buffer[current_data_offset + 1] << 8 |
                   (uint16_t)data_buffer[current_data_offset];
    current_data_offset += 2;

    return val;
}

uint32_t read_u32_from_data()
{
    uint32_t val = (uint32_t)data_buffer[current_data_offset + 3] << 24 |
                   (uint32_t)data_buffer[current_data_offset + 2] << 16 |
                   (uint32_t)data_buffer[current_data_offset + 1] << 8 |
                   (uint32_t)data_buffer[current_data_offset + 0];
    current_data_offset += 4;
    return val;
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

void write(uint8_t* data, uint32_t len)
{

    // TODO(patrik): Use this to check if the write buffer is full and
    // then flush it
    // uint32_t avail = tud_cdc_n_write_available(PORT_CMD);
    tud_cdc_n_write(PORT_CMD, data, len);
}

void write_u8(uint8_t value) { write(&value, 1); }

void write_u16(uint16_t value)
{
    write_u8(value & 0xff);
    write_u8((value >> 8) & 0xff);
}

Packet parse_packet()
{
    uint8_t pid = read_u8();
    uint8_t typ = read_u8();
    uint8_t data_len = read_u8();

    read(data_buffer, (uint32_t)data_len);
    current_data_offset = 0;

    uint16_t checksum = read_u16();

    Packet packet;
    packet.pid = pid;
    packet.typ = typ;
    packet.data_len = data_len;
    packet.checksum = checksum;

    return packet;
}

void send_packet(uint8_t typ, uint8_t* data, uint8_t len)
{
    write_u8(PACKET_START);
    write_u8(0);
    write_u8(typ);

    // Data Length
    if (data && len > 0)
    {
        write_u8(len);
        write(data, len);
    }
    else
    {
        write_u8(0);
    }

    // Checksum
    write_u16(0);

    tud_cdc_n_write_flush(PORT_CMD);
}

void send_empty_packet(uint8_t typ) { send_packet(typ, nullptr, 0); }

static bool send_updates = false;

void handle_packets()
{
    if (tud_cdc_n_available(PORT_CMD) > 0)
    {
        uint8_t b = read_u8();
        if (b == PACKET_START)
        {
            Packet packet = parse_packet();

            switch (packet.typ)
            {
                case Packet_Ping: {
                    uint8_t buf[] = {Ack_Pong};
                    send_packet(Packet_Ack, buf, sizeof(buf));
                }
                break;

                case Packet_Set: {
                    uint8_t var = read_u8_from_data();

                    uint32_t value = read_u32_from_data();
                    printf("Setting '%d' = 0x%x\n", var, value);

                    uint8_t buf[] = {Ack_Success};
                    send_packet(Packet_Ack, buf, sizeof(buf));
                }
                break;

                case Packet_Get: {
                    uint8_t var = read_u8_from_data();
                    printf("Getting '%d'\n", var);

                    uint32_t value = 0xffddeecc;

                    uint8_t buf[5] = {Ack_Success};
                    buf[1] = value & 0xff;
                    buf[2] = value >> 8 & 0xff;
                    buf[3] = value >> 16 & 0xff;
                    buf[4] = value >> 24 & 0xff;

                    send_packet(Packet_Ack, buf, sizeof(buf));
                }
                break;

                case Packet_Configure: {
                    send_updates = read_u8_from_data() > 0 ? true : false;

                    uint8_t buf[] = {Ack_Success};
                    send_packet(Packet_Ack, buf, sizeof(buf));
                }
                break;

                default:
                    uint8_t buf[] = {Ack_ErrorUnknownPacketType};
                    send_packet(Packet_Ack, buf, sizeof(buf));
                    printf("Error: Unknown Packet Type: 0x%x\n", packet.typ);
                    break;
            }
        }
    }
}

void send_update() {}

void test_thread(void* ptr)
{
    while (1)
    {
        handle_packets();

        if (send_updates)
            send_update();

        vTaskDelay(1);
    }
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
