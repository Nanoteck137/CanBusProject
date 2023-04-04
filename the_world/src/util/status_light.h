#pragma once

#include "common.h"
#include "device.h"

enum class StatusLightState
{
    Off,
    On,
    Blink,
    BlinkCount,
};

struct StatusLight
{
    void init(PhysicalControl* control);

    void update();

    void blink(uint64_t blink_time);
    void blink_toggle(uint64_t blink_time);
    void blink_count(uint64_t blink_time, uint32_t blink_count);

    void set(bool on);
    void toggle();

private:
    void set_state(StatusLightState new_state);

private:
    PhysicalControl* m_control = nullptr;
    StatusLightState m_state = StatusLightState::Off;
    bool m_state_changed = false;

    uint64_t m_last_time = 0;
    uint64_t m_blink_time = 0;
    uint32_t m_blink_count = 0;
    uint32_t m_current_blink_count = 0;
    bool m_blink_value = false;
};
