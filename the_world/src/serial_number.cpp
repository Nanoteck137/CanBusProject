#include "serial_number.h"

// NOTE(patrik): From
// https://github.com/raspberrypi/picoprobe/blob/164eaa5b80295ff9ef3a8ebf8949d5db4699f904/src/get_serial.c

char usb_serial_id[PICO_UNIQUE_BOARD_ID_SIZE_BYTES * 2 + 1];
pico_unique_board_id_t id;

void serial_number_init()
{
    pico_get_unique_board_id(&id);

    for (int i = 0; i < PICO_UNIQUE_BOARD_ID_SIZE_BYTES * 2; i++)
    {
        int byte_index = i / 2;

        uint8_t nibble = (id.id[byte_index] >> 4) & 0x0F;

        id.id[byte_index] <<= 4;

        usb_serial_id[i] = nibble < 10 ? nibble + '0' : nibble + 'A' - 10;
    }
}
