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

void send_packet_response(ErrorCode error_code, uint8_t* data, size_t len)
{
    write_packet_header(PacketType::Response);

    // NOTE(patrik): Length of data + error code
    write_u8(len + 1);
    write_u8((uint8_t)error_code);

    // Data Length
    if (data && len > 0)
    {
        write(data, len);
    }

    // Checksum
    write_u16(0);

    tud_cdc_n_write_flush(PORT_CMD);
}

void send_empty_packet(PacketType type) { send_packet(type, nullptr, 0); }

static bool send_updates = false;

static uint8_t temp[256];

void identify()
{
    // TODO(patriK): Fix
    uint8_t buffer[2 + 1 + 32] = {0};

    // Version
    buffer[0] = spec.version & 0xff;
    buffer[1] = (spec.version >> 8) & 0xff;

    // Type
    buffer[2] = (uint8_t)spec.type;

    // Name
    // TODO(patrik): Check for string length is not over 32
    memcpy(buffer + 3, spec.name, strlen(spec.name));

    send_packet_response(ErrorCode::Success, buffer, sizeof(buffer));
}

void status()
{
    uint8_t buffer[STATUS_BUFFER_SIZE];
    memset(buffer, 0, sizeof(buffer));

    spec.get_status(buffer);

    send_packet_response(ErrorCode::Success, buffer, sizeof(buffer));
}

void func() { send_packet_response(ErrorCode::Unknown, nullptr, 0); }

void ping() { send_packet_response(ErrorCode::Success, nullptr, 0); }

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
                case PacketType::Identify: identify(); break;
                case PacketType::Status: status(); break;
                case PacketType::ExecFunc: func(); break;
                case PacketType::Ping: ping(); break;

                default:
                    send_packet_response(ErrorCode::InvalidPacketType, nullptr,
                                         0);
                    break;
            }
        }
    }
}

void send_update() {}

void com_thread(void* ptr)
{
    while (1)
    {
        handle_packets();

        if (send_updates)
            send_update();

        vTaskDelay(1);
    }
}
