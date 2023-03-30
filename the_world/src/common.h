#pragma once

#include <stdint.h>
#include <stdlib.h>

#include "tusk.h"

const uint8_t PORT_CMD = 0;
const uint8_t PORT_DEBUG = 1;

const size_t NUM_DEVICES = 1;

#define MAKE_VERSION(major, minor, patch)                                      \
    (((uint16_t)(major)) & 0x3f) << 10 | (((uint16_t)(minor)) & 0x3f) << 4 |   \
        ((uint16_t)(patch)) & 0xf

struct Device
{
    uint32_t index;
    uint32_t can_id;

    uint32_t com_id() { return can_id + 0x00; }
    uint32_t control_id() { return can_id + 0x01; }
    uint32_t status_id() { return can_id + 0x02; }
    uint32_t misc_id() { return can_id + 0x03; }
};

struct DeviceData
{
    Device device;

    uint8_t control_status;

    uint8_t line_status;
    uint8_t extra_line_status;

    uint8_t toggled_line_status;
    uint8_t toggled_extra_line_status;
};

struct Config
{
    const char* name;
    uint16_t version;
    DeviceType type;

    uint32_t can_id;

    Device devices[MAX_DEVICES];

    size_t num_controls;
    uint controls[MAX_IO];

    size_t num_lines;
    uint lines[MAX_IO];
};

extern Config config;

extern DeviceData device_data[];
