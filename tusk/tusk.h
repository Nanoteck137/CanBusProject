#pragma once

#include <stddef.h>
#include <stdint.h>

extern "C" {
const uint8_t PACKET_START = 0x4e;

enum PacketType : uint8_t {
  PacketIdentify,
  PacketCommand,
  PacketPing,
  PacketUpdate,
  PacketResponse
};

enum ResponseCode : uint8_t { ResSuccess, ResError };

size_t tusk_get_max_encode_buffer_size(size_t data_length);
size_t tusk_encode(const uint8_t *input_buffer, size_t input_length,
                   uint8_t *output, uint8_t delimiter);
size_t tusk_decode(const uint8_t *input_buffer, size_t input_length,
                   uint8_t *output, uint8_t delimiter);

uint16_t tusk_checksum(const uint8_t *buffer, size_t length);
void tusk_check_bytes(uint16_t sum, uint8_t *a, uint8_t *b);
}
