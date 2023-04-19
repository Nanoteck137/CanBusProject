#include "com.h"

#include "device.h"

#include <class/cdc/cdc_device.h>

#include <FreeRTOS.h>
#include <task.h>

static uint8_t data_buffer[256];
static size_t current_data_offset = 0;

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
    packet.typ = (PacketType)typ;
    packet.data_len = data_len;
    packet.checksum = checksum;

    return packet;
}

void write_packet_header(PacketType type)
{
    write_u8(PACKET_START);
    write_u8(0);
    write_u8((uint8_t)type);
}

void send_packet(PacketType type, uint8_t* data, uint8_t len)
{
    write_packet_header(type);

    write_u8(len);

    // Data Length
    if (data && len > 0)
        write(data, len);

    // Checksum
    write_u16(0);

    tud_cdc_n_write_flush(PORT_CMD);
}

void send_packet_response(ResponseErrorCode error_code, uint8_t* data,
                          size_t len)
{
    write_packet_header(PacketType::Response);

    // NOTE(patrik): Length of data + error code
    write_u8(len + 1);
    write_u8((uint8_t)error_code);

    // Data Length
    if (data && len > 0)
        write(data, len);

    // Checksum
    write_u16(0);

    tud_cdc_n_write_flush(PORT_CMD);
}

void send_empty_packet(PacketType type) { send_packet(type, nullptr, 0); }

static bool send_updates = false;

static uint8_t temp[256];

void identify(DeviceContext* device)
{
    // TODO(patriK): Fix
    // NOTE(patrik): Bytes
    //  2 bytes - Version
    //  1 byte - Num Commands
    //  32 bytes - name
    uint8_t buffer[2 + 1 + 32] = {0};

    // Version
    buffer[0] = spec.version & 0xff;
    buffer[1] = (spec.version >> 8) & 0xff;

    // Num Commands
    buffer[2] = (uint8_t)device->num_cmds;

    // Name
    // TODO(patrik): Check for string length is not over 32
    memcpy(buffer + 3, spec.name, strlen(spec.name));

    send_packet_response(ResponseErrorCode::Success, buffer, sizeof(buffer));
}

void status()
{
    uint8_t buffer[STATUS_BUFFER_SIZE];
    memset(buffer, 0, sizeof(buffer));

    spec.get_status(buffer);

    send_packet_response(ResponseErrorCode::Success, buffer, sizeof(buffer));
}

void command(Packet* packet, DeviceContext* device)
{
    if (packet->data_len <= 0)
    {
        // TODO(patrik): Change error code
        send_packet_response(ResponseErrorCode::InvalidDevice, nullptr, 0);
        return;
    }

    uint8_t cmd_index = read_u8_from_data();

    if (cmd_index >= device->num_cmds)
    {
        send_packet_response(ResponseErrorCode::InvalidFunction, nullptr, 0);
        return;
    }

    size_t num_params = 0;
    if (packet->data_len > 0)
        num_params = packet->data_len - 1;

    CmdFunction cmd = spec.funcs[cmd_index];
    ResponseErrorCode error_code =
        cmd(data_buffer + current_data_offset, num_params);
    send_packet_response(error_code, nullptr, 0);
}

void ping() { send_packet_response(ResponseErrorCode::Success, nullptr, 0); }

void handle_packets(DeviceContext* device)
{
    if (tud_cdc_n_available(PORT_CMD) > 0)
    {
        uint8_t b = read_u8();
        if (b == PACKET_START)
        {
            Packet packet = parse_packet();

            switch (packet.typ)
            {
                case PacketType::Identify: identify(device); break;
                case PacketType::Status: status(); break;
                case PacketType::Command: command(&packet, device); break;
                case PacketType::Ping: ping(); break;

                default:
                    send_packet_response(ResponseErrorCode::InvalidPacketType,
                                         nullptr, 0);
                    break;
            }
        }
    }
}

void send_update() {}

void com_thread(void* ptr)
{
    DeviceContext* device = (DeviceContext*)ptr;
    while (1)
    {
        handle_packets(device);

        if (send_updates)
            send_update();

        vTaskDelay(1);
    }
}
