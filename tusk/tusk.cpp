#include "tusk.h"

#include <stdio.h>

extern "C" {
size_t tusk_get_encode_size(size_t data_length) { return data_length + 2; }

void tusk_encode(const uint8_t *input_buffer, size_t input_length,
                 uint8_t *output) {
  uint8_t code = 1;
  uint8_t *code_ptr = output;

  *output++ = 0;

  for (size_t i = 0, len = input_length; i < input_length; i++, len--) {
    uint8_t b = input_buffer[i];
    if (b != 0) {
      *output++ = b;
      code++;
    }

    if (b == 0) {
      *code_ptr = code;
      code_ptr = output;
      code = 1;

      *output++ = 0;
    }

    if (code == 0xff) {
      *code_ptr = code;
      code_ptr = output;

      if (len > 1) {
        code = 1;
      } else {
        code = 0;
      }

      *output++ = 0;
    }

    // if (len == 1) {
    //   code = 0;
    // }
  }

  *code_ptr = code;
  *output++ = 0;
}

void tusk_decode(const uint8_t *input_buffer, size_t input_length,
                 uint8_t *output) {

  uint8_t target = input_buffer[0];

  size_t index = 0;
  for (size_t i = 1; i < input_length; i++) {
    uint8_t b = input_buffer[i];

    if (index >= (size_t)(target - 1)) {
      target = b;
      if (target != 0 && index != 0xff - 1) {
        *output++ = 0;
      }
      index = 0;
    } else {
      *output++ = b;
      index++;
    }
  }
}

uint16_t tusk_checksum(const uint8_t *buffer, size_t length) {
  uint16_t sum1 = 0;
  uint16_t sum2 = 0;

  for (size_t i = 0; i < length; i++) {
    sum1 = (sum1 + buffer[i]) % 255;
    sum2 = (sum2 + sum1) % 255;
  }

  return sum2 << 8 | sum1;
}

void tusk_check_bytes(uint16_t sum, uint8_t *a, uint8_t *b) {
  uint8_t b0 = sum & 0xff;
  uint8_t b1 = (sum >> 8) & 0xff;
  *a = 0xff - ((b0 + b1) % 0xff);
  *b = 0xff - ((b0 + *a) % 0xff);
}
}
