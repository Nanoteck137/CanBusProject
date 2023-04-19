#pragma once

#include "common.h"

struct Packet
{
    uint8_t pid;
    PacketType typ;
    uint8_t data_len;
    uint16_t checksum;
};

void com_thread(void* ptr);
