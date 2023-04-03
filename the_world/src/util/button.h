#pragma once

#include "pico/stdlib.h"

// NOTE(patrik): https://github.com/poelstra/arduino-multi-button

const int DEBOUNCE_DELAY = 50 * 1000;     // us
const int SINGLECLICK_DELAY = 350 * 1000; // us
const int LONGCLICK_DELAY = 500 * 1000;   // us

enum class ButtonState
{
    Idle,
    Debounce,
    Pressed,

    ClickUp,
    ClickIdle,

    SingleClick,

    DoubleClickDebounce,
    DoubleClick,

    LongClick,
    OtherUp,
};

class Button
{
public:
    Button();

    void update(bool pressed);

    bool is_click() const;
    bool is_released();

    bool is_single_click();
    bool is_double_click();
    bool is_long_click();

    ButtonState check_idle(bool pressed, int diff);

    ButtonState check_debounce(bool pressed, int diff);

    ButtonState check_pressed(bool pressed, int diff);

    ButtonState check_click_up(bool pressed, int diff);
    ButtonState check_click_idle(bool pressed, int diff);
    ButtonState check_single_click(bool pressed, int diff);

    ButtonState check_double_click_debounce(bool pressed, int diff);

    ButtonState check_double_click(bool pressed, int diff);
    ButtonState check_long_click(bool pressed, int diff);
    ButtonState check_other_up(bool pressed, int diff);

private:
    uint64_t m_LastTransition;
    ButtonState m_State;
    bool m_New;
};
