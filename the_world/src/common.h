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
};

struct DeviceData
{
    Device device;

    // uint8_t controls;
    // uint8_t lines;
    // uint8_t toggled_lines;

    uint32_t control_id() { return device.can_id + 0x01; }
    uint32_t line_id() { return device.can_id + 0x02; }
};

struct Config
{
    const char* name;
    uint16_t version;
    DeviceType type;

    uint32_t can_id;
    bool has_can;

    Device devices[MAX_DEVICES];

    size_t num_inputs;
    uint inputs[MAX_IO];

    size_t num_outputs;
    uint outputs[MAX_IO];
};

const Config config = {
    .name = "RSNav",
    .version = MAKE_VERSION(0, 1, 0),
    .type = DeviceType::StarPlatinum,

    .can_id = 0x000,
    .has_can = false,

    .devices =
        {
            {
                .index = 0,
                .can_id = 0x100,
            },
        },

    .num_inputs = 0,
    .inputs = {},

    .num_outputs = 0,
    .outputs = {},
};

const DeviceData device_data[MAX_DEVICES] = {};
