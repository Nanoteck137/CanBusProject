#pragma once

#include "common.h"

const size_t MAX_LINES = 16;
const size_t MAX_CONTROLS = 16;

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

    bool get() const { return m_is_on; }

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
};

extern const DeviceSpec spec;
