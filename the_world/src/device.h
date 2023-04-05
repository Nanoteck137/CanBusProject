#pragma once

#include "common.h"
#include "func.h"

const size_t STATUS_BUFFER_SIZE = 16;
const size_t MAX_LINES = 16;
const size_t MAX_CONTROLS = 16;
const size_t MAX_CMDS = 50;

class PhysicalLine
{
public:
    void init(uint32_t pin);

    bool get();

private:
    uint32_t m_pin = 0xffffffff;
};

class PhysicalControl
{
public:
    void init(uint32_t pin);

    void set(bool on);
    void toggle();

    bool is_on() const { return m_is_on; }

private:
    uint32_t m_pin = 0xffffffff;
    bool m_is_on = false;
};

struct DeviceContext
{
    size_t num_lines;
    PhysicalLine lines[MAX_LINES];

    size_t num_controls;
    PhysicalControl controls[MAX_CONTROLS];

    size_t num_cmds;
};

typedef void (*InitFunction)(DeviceContext* device);
typedef void (*UpdateFunction)(DeviceContext* device);
typedef void (*GetStatusFunction)(uint8_t* buffer);
typedef void (*OnCanMessageFunction)(uint32_t can_id, uint8_t* data,
                                     size_t len);

struct DeviceSpec
{
    const char* name;
    uint16_t version;

    size_t num_lines;
    uint32_t lines[MAX_LINES];

    size_t num_controls;
    uint32_t controls[MAX_CONTROLS];

    InitFunction init;
    UpdateFunction update;
    GetStatusFunction get_status;
    OnCanMessageFunction on_can_message;

    CmdFunction funcs[MAX_CMDS];
};

extern const DeviceSpec spec;
