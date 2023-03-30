#pragma once

#include <pico/unique_id.h>

extern char usb_serial_id[];
extern pico_unique_board_id_t id;

void serial_number_init();
