#pragma once

#include "common.h"
#include "func.h"

const size_t STATUS_BUFFER_SIZE = 16;
const size_t MAX_LINES = 16;
const size_t MAX_CONTROLS = 16;
const size_t MAX_FUNCS = 50;

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
    PhysicalLine lines[MAX_LINES];
    PhysicalControl controls[MAX_CONTROLS];
};

typedef void (*InitFunction)(DeviceContext* device);
typedef void (*UpdateFunction)(DeviceContext* device);
typedef void (*GetStatusFunction)(uint8_t* buffer);

struct DeviceSpec
{
    const char* name;
    uint16_t version;
    DeviceType type;

    size_t num_lines;
    uint32_t lines[MAX_LINES];

    size_t num_controls;
    uint32_t controls[MAX_CONTROLS];

    InitFunction init;
    UpdateFunction update;
    GetStatusFunction get_status;

    Func funcs[MAX_FUNCS];
};

extern const DeviceSpec spec;
