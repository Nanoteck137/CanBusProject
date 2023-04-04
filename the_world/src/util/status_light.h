#pragma once

#include "common.h"

enum class StatusLightState
{
    Off,
    On,
    Blink,
    BlinkCount,
};

struct StatusLight
{
    uint32_t pin = 0xffffffff;
    StatusLightState state = StatusLightState::Off;
    bool state_changed = false;

    uint64_t last_time = 0;
    uint64_t blink_time = 0;
    uint32_t blink_count = 0;
    uint32_t current_blink_count = 0;
    bool blink_value = false;

    void init(uint32_t pin);

    void update();

    void set_blink(uint64_t blink_time);
    void set_blink_toggle(uint64_t blink_time);
    void set_blink_count(uint64_t blink_time, uint32_t blink_count);

    // void on();
    // void off();
    void set_toggle_on_off();
};
