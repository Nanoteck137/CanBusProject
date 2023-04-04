#include "status_light.h"

#include <pico/stdlib.h>
#include <hardware/gpio.h>

void StatusLight::init(uint32_t pin)
{
    m_pin = pin;

    gpio_init(pin);
    gpio_set_dir(pin, GPIO_OUT);
    gpio_put(pin, false);
}

void StatusLight::update()
{
    uint32_t current_time = time_us_64();

    if (m_state_changed)
    {
        switch (m_state)
        {
            case StatusLightState::On: gpio_put(m_pin, true); break;
            case StatusLightState::Off: gpio_put(m_pin, false); break;

            case StatusLightState::Blink:
            case StatusLightState::BlinkCount: {
                m_last_time = current_time;
                gpio_put(m_pin, false);
            }
            break;
        }

        m_state_changed = false;
    }

    switch (m_state)
    {
        case StatusLightState::Blink: {
            if (current_time - m_last_time > m_blink_time)
            {
                m_blink_value = !m_blink_value;
                gpio_put(m_pin, m_blink_value);
                m_last_time = current_time;
            }
        }

        case StatusLightState::BlinkCount: {
            if (current_time - m_last_time > m_blink_time)
            {
                if (m_current_blink_count >= m_blink_count)
                {
                    gpio_put(m_pin, false);
                }
                else
                {
                    m_blink_value = !m_blink_value;
                    gpio_put(m_pin, m_blink_value);
                    if (m_blink_value)
                        m_current_blink_count++;
                }

                m_last_time = current_time;
            }
        }

        break;

        default: break;
    }
}

void StatusLight::blink(uint64_t blink_time)
{
    set_state(StatusLightState::Blink);
    m_blink_time = blink_time;
}

void StatusLight::blink_toggle(uint64_t blink_time)
{
    if (m_state == StatusLightState::Blink)
        off();
    else
        blink(blink_time);
}

void StatusLight::blink_count(uint64_t blink_time, uint32_t blink_count)
{
    set_state(StatusLightState::BlinkCount);
    m_blink_time = blink_time;
    m_blink_count = blink_count;
    m_current_blink_count = 0;
}

void StatusLight::on() { set_state(StatusLightState::On); }

void StatusLight::off() { set_state(StatusLightState::Off); }

void StatusLight::toggle()
{
    switch (m_state)
    {
        case StatusLightState::On: off();
        default: on();
    }
}

void StatusLight::set_state(StatusLightState new_state)
{
    m_state = new_state;
    m_state_changed = true;
}
