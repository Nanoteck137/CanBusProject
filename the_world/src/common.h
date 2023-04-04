#pragma once

#include <stdint.h>
#include <stdlib.h>

#include "tusk.h"

const uint8_t PORT_CMD = 0;
const uint8_t PORT_DEBUG = 1;

#define MAKE_VERSION(major, minor, patch)                                      \
    (((uint16_t)(major)) & 0x3f) << 10 | (((uint16_t)(minor)) & 0x3f) << 4 |   \
        ((uint16_t)(patch)) & 0xf
