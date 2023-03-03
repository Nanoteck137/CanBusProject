
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

static uint8_t temp[256];

void send_success(uint8_t* data, uint8_t len)
{
    // TODO(patrik): Check len
    temp[0] = ResSuccess;
    uint8_t data_length = 1;

    if (data && len > 0)
    {
        memcpy(temp + 1, data, len);
        data_length += len;
    }

    send_packet(PacketResponse, temp, data_length);
}

void send_error(uint8_t err)
{
    uint8_t data[] = {ResError, err};
    send_packet(PacketResponse, data, sizeof(data));
}

enum MCUType : uint8_t
{
    Mcu_TheWorldOverHeven,
};

#define MAKE_VERSION(major, minor, patch)                                      \
    (((uint16_t)(major)) & 0x3f) << 10 | (((uint16_t)(minor)) & 0x3f) << 4 |   \
        ((uint16_t)(patch)) & 0xf

const uint16_t VERSION = MAKE_VERSION(0, 1, 0);
static const char name[] = "DAAAAAAAAAAAAAAGHBBBBBBBBBBBBBBC";
static_assert(sizeof(name) <= 33, "32 character limit on the name");

void identify()
{
    // NOTE(patrik): Identify
    // - Version (0.1) (2-bytes) (MAJOR MINOR PATCH)
    // - Type (0x01) (The World over heaven) (1-byte)
    // - Name (Main Control Unit) (32-bytes MAX)

    uint8_t buffer[2 + 1 + 32] = {0};
    // Version
    // aaaaaabb bbbbcccc
    // 0: aaaaaabb
    // 1: bbbbcccc
    //   - a: Major
    //   - b: Minor
    //   - c: Patch
    buffer[0] = VERSION & 0xff;
    buffer[1] = (VERSION >> 8) & 0xff;

    // Type
    buffer[2] = Mcu_TheWorldOverHeven;

    // Name
    memcpy(buffer + 3, name, strlen(name));

    send_success(buffer, sizeof(buffer));
}

void command()
{
    uint8_t cmd = read_u8_from_data();
    switch (cmd)
    {
        case 0x00: {
            uint8_t var = read_u8_from_data();
            uint32_t value = read_u32_from_data();
            printf("SET: Var 0x%x = 0x%x\n", var, value);
            send_success(nullptr, 0);
        }
        break;
        case 0x01: {
            uint8_t var = read_u8_from_data();
            printf("GET: Var 0x%x\n", var);
            send_success(nullptr, 0);
        }
        break;
        case 0x02: {
            uint8_t send_updates = read_u8_from_data();
            printf("CONFIGURE SU: %d\n", send_updates > 0 ? true : false);
            send_success(nullptr, 0);
        }
        break;
        default: send_error(0xfd); break;
    }
}
void ping() { send_success(nullptr, 0); }
void update() { send_error(0xff); };
void response() { send_error(0xff); };

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
                case PacketIdentify: identify(); break;
                case PacketCommand: command(); break;
                case PacketPing: ping(); break;
                case PacketUpdate: update(); break;
                case PacketResponse: response(); break;

                default:
                    // TODO(patrik): Change the error
                    send_error(0xff);
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
