
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

#include "mcp2515/mcp2515.h"

// TODO(patrik):
//   - Change the name of command_queue to command_buffer
//   - Refactoring

MCP2515 can0(spi0, 5, 3, 4, 2);

static uint8_t data_buffer[256];
static size_t current_data_offset = 0;

const uint8_t PORT_CMD = 0;
const uint8_t PORT_DEBUG = 1;

#define MAX_DEVICES 16
#define MAX_CONTROLS 8
#define MAX_LINES 8

struct Device
{
    uint32_t id;

    bool need_update;

    uint8_t controls;
    uint8_t lines;

    void init(uint32_t id) { this->id = id; }

    uint32_t control_id() { return this->id + 0x01; }
    uint32_t line_id() { return this->id + 0x02; }
};

const size_t NUM_DEVICES = 1;
static_assert(NUM_DEVICES <= MAX_DEVICES, "Too many devices");
static Device devices[NUM_DEVICES];

void init_devices() { devices[0].init(0x100); }

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

    can0.reset();
    can0.setBitrate(CAN_125KBPS, MCP_8MHZ);
    can0.setNormalMode();

    init_devices();
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
    Mcu_TheWorldOverHeaven,
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
    buffer[2] = Mcu_TheWorldOverHeaven;

    // Name
    memcpy(buffer + 3, name, strlen(name));

    send_success(buffer, sizeof(buffer));
}

void command_set(uint8_t var, uint32_t value)
{
    // Devices:
    //  Controls
    //    - Control relays
    //    - This is sent from main unit to the other units to control
    //      the devices connected
    //
    //  Lines
    //    - User/Sensor inputs from other units to the main unit

    // SET device_control_0 1
    // GET device_line_0 1

    uint32_t control_index = (value >> 8) & 0xff;
    switch (var)
    {
        case 0x00: /* push_output_command(&test_device, value > 0); */ break;

        default: {
            // TODO(patrik): Change error code
            uint8_t data[] = {ResError, 0xff};
            send_packet(PacketResponse, data, sizeof(data));
        }
        break;
    }
}

void command()
{
    uint8_t cmd = read_u8_from_data();
    switch (cmd)
    {
        case 0x00: {
            // CONFIGURE
            uint8_t send_updates = read_u8_from_data();
            printf("CONFIGURE SU: %d\n", send_updates > 0 ? true : false);
            send_success(nullptr, 0);
        }
        break;

        case 0x01: {
            // SET_DEVICE_CONTROLS
            uint8_t device_index = read_u8_from_data();
            uint8_t controls = read_u8_from_data();
            static_assert(MAX_CONTROLS <= sizeof(controls) * 8,
                          "Max 8 controls for now");

            if (device_index >= NUM_DEVICES)
            {
                send_error(0xb0);
                return;
            }

            Device* device = &devices[device_index];
            device->controls = controls;
            device->need_update = true;

            printf("SET_DEVICE_CONTROLS: 0x%x = 0x%x\n", device_index,
                   controls);

            send_success(nullptr, 0);
        }
        break;

        case 0x02: {
            // GET_DEVICE_CONTROLS
            uint8_t device_index = read_u8_from_data();

            if (device_index >= NUM_DEVICES)
            {
                send_error(0xb0);
                return;
            }

            Device* device = devices + device_index;
            uint8_t controls = device->controls;

            printf("GET_DEVICE_CONTROLS: 0x%x\n", device_index);
            uint8_t data[] = {controls};
            send_success(data, sizeof(data));
        }
        break;

        case 0x03: {
            // GET_DEVICE_LINES

            uint8_t device_index = read_u8_from_data();

            if (device_index >= NUM_DEVICES)
            {
                send_error(0xb0);
                return;
            }

            Device* device = devices + device_index;
            uint8_t lines = device->lines;

            printf("GET_DEVICE_LINES: 0x%x\n", device_index);
            uint8_t data[] = {lines};
            send_success(data, sizeof(data));
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

void can_bus_thread(void* ptr)
{
    // TODO(patrik): We need to read all the sensor data
    // TODO(patrik): Then we store the data so we can retrive it later
    // TODO(patrik): Then use the data to send updates

    // NOTE(patrik):
    // Device On the Can Bus Network
    // Every devices gets 4 ids allocated to them
    // ID Allocation:
    //  id + 0x00 - Communication ID
    //  id + 0x01 - Outputs (Control Relays)
    //  id + 0x02 - Inputs (State of the inputs, Send to main unit)
    //  id + 0x03 - Misc

    while (true)
    {
        can_frame frame;
        if (can0.readMessage(&frame) == MCP2515::ERROR_OK)
        {
            for (int i = 0; i < NUM_DEVICES; i++)
            {
                Device* device = devices + i;
                if (frame.can_id == device->line_id())
                {
                    if (frame.can_dlc == 2)
                    {
                        uint8_t current_lines = frame.data[0];
                        uint8_t toggle_lines = frame.data[1];
                        device->lines = toggle_lines;
                        printf("Lines: 0x%x 0x%x\n", current_lines,
                               toggle_lines);
                    }
                }
            }
        }

        for (int i = 0; i < NUM_DEVICES; i++)
        {
            Device* device = devices + i;
            if (device->need_update)
            {
                can_frame send;
                send.can_id = device->control_id();
                send.can_dlc = 1;
                send.data[0] = device->controls;
                can0.sendMessage(&send);

                device->need_update = false;
            }
        }

        vTaskDelay(1);
    }
}

static TaskHandle_t usb_thread_handle;
static TaskHandle_t test_thread_handle;
static TaskHandle_t can_bus_thread_handle;

int main()
{
    init_system();

    xTaskCreate(usb_thread, "USB Thread", configMINIMAL_STACK_SIZE, nullptr,
                tskIDLE_PRIORITY + 3, &usb_thread_handle);
    xTaskCreate(can_bus_thread, "Can Bus Thread", configMINIMAL_STACK_SIZE,
                nullptr, tskIDLE_PRIORITY + 2, &can_bus_thread_handle);
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
