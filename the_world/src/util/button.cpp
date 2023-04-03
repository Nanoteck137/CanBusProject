#include "button.h"

Button::Button()
    : m_LastTransition(time_us_64()), m_State(ButtonState::Idle),
      m_New(false){};

void Button::update(bool pressed)
{
    m_New = false;

    if (!pressed && m_State == ButtonState::Idle)
        return;

    uint64_t now = time_us_64();
    int diff = now - m_LastTransition;

    ButtonState next = ButtonState::Idle;
    switch (m_State)
    {
        case ButtonState::Idle: next = check_idle(pressed, diff); break;
        case ButtonState::Debounce: next = check_debounce(pressed, diff); break;
        case ButtonState::Pressed: next = check_pressed(pressed, diff); break;
        case ButtonState::ClickUp: next = check_click_up(pressed, diff); break;
        case ButtonState::ClickIdle:
            next = check_click_idle(pressed, diff);
            break;
        case ButtonState::SingleClick:
            next = check_single_click(pressed, diff);
            break;
        case ButtonState::DoubleClickDebounce:
            next = check_double_click_debounce(pressed, diff);
            break;
        case ButtonState::DoubleClick:
            next = check_double_click(pressed, diff);
            break;
        case ButtonState::LongClick:
            next = check_long_click(pressed, diff);
            break;
        case ButtonState::OtherUp: next = check_other_up(pressed, diff); break;
    }

    if (next != m_State)
    {
        m_LastTransition = now;
        m_State = next;
        m_New = true;
    }
}

bool Button::is_click() const
{
    return m_New && (m_State == ButtonState::Pressed ||
                     m_State == ButtonState::DoubleClick);
}

bool Button::is_single_click()
{
    return m_New && m_State == ButtonState::SingleClick;
}

bool Button::is_double_click()
{
    return m_New && m_State == ButtonState::DoubleClick;
}

bool Button::is_long_click()
{
    return m_New && m_State == ButtonState::LongClick;
}

bool Button::is_released()
{
    return m_New &&
           (m_State == ButtonState::ClickUp || m_State == ButtonState::OtherUp);
}

ButtonState Button::check_idle(bool pressed, int diff)
{
    return pressed ? ButtonState::Debounce : ButtonState::Idle;
}

ButtonState Button::check_debounce(bool pressed, int diff)
{
    if (!pressed)
        return ButtonState::Idle;

    if (diff >= DEBOUNCE_DELAY)
        return ButtonState::Pressed;

    return ButtonState::Debounce;
}

ButtonState Button::check_pressed(bool pressed, int diff)
{
    if (!pressed)
        return ButtonState::ClickUp;

    if (diff >= LONGCLICK_DELAY)
        return ButtonState::LongClick;

    return ButtonState::Pressed;
}

ButtonState Button::check_click_up(bool pressed, int diff)
{
    return ButtonState::ClickIdle;
}

ButtonState Button::check_click_idle(bool pressed, int diff)
{
    if (pressed)
        return ButtonState::DoubleClickDebounce;

    if (diff >= SINGLECLICK_DELAY)
        return ButtonState::SingleClick;

    return ButtonState::ClickIdle;
}

ButtonState Button::check_single_click(bool pressed, int diff)
{
    return ButtonState::Idle;
}

ButtonState Button::check_double_click_debounce(bool pressed, int diff)
{
    if (!pressed)
        return ButtonState::ClickIdle;

    if (diff >= DEBOUNCE_DELAY)
        return ButtonState::DoubleClick;

    return ButtonState::DoubleClickDebounce;
}

ButtonState Button::check_double_click(bool pressed, int diff)
{
    if (!pressed)
        return ButtonState::OtherUp;

    return ButtonState::DoubleClick;
}

ButtonState Button::check_long_click(bool pressed, int diff)
{
    if (!pressed)
        return ButtonState::OtherUp;

    return ButtonState::LongClick;
}

ButtonState Button::check_other_up(bool pressed, int diff)
{
    return ButtonState::Idle;
}
