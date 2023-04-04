#include "status_light.h"

#include <pico/stdlib.h>
#include <hardware/gpio.h>

void StatusLight::init(uint32_t pin)
{
    this->pin = pin;

    gpio_init(pin);
    gpio_set_dir(pin, GPIO_OUT);
    gpio_put(pin, false);
}

void StatusLight::update()
{
    uint32_t current_time = time_us_64();

    if (state_changed)
    {
        switch (state)
        {
            case StatusLightState::On: gpio_put(pin, true); break;
            case StatusLightState::Off: gpio_put(pin, false); break;

            case StatusLightState::Blink:
            case StatusLightState::BlinkCount: {
                last_time = current_time;
                gpio_put(pin, false);
            }
            break;
        }

        state_changed = false;
    }

    switch (state)
    {
        case StatusLightState::Blink: {
            if (current_time - last_time > blink_time)
            {
                blink_value = !blink_value;
                gpio_put(pin, blink_value);
                last_time = current_time;
            }
        }

        case StatusLightState::BlinkCount: {
            if (current_time - last_time > blink_time)
            {
                if (current_blink_count >= blink_count)
                {
                    gpio_put(pin, false);
                }
                else
                {
                    blink_value = !blink_value;
                    gpio_put(pin, blink_value);
                    printf("Before: %d\n", current_blink_count);
                    if (blink_value)
                        current_blink_count++;
                    printf("After: %d\n", current_blink_count);
                }

                last_time = current_time;
            }
        }

        break;

        default: break;
    }
}

void StatusLight::set_blink(uint64_t blink_time)
{
    state = StatusLightState::Blink;
    this->blink_time = blink_time;
}

void StatusLight::set_blink_toggle(uint64_t blink_time)
{
    if (state == StatusLightState::Blink)
        state = StatusLightState::Off;
    else
        set_blink(blink_time);
}

void StatusLight::set_blink_count(uint64_t blink_time, uint32_t blink_count)
{
    state = StatusLightState::BlinkCount;
    this->blink_time = blink_time;
    this->blink_count = blink_count;
    this->current_blink_count = 0;
}

void StatusLight::set_toggle_on_off()
{
    if (state == StatusLightState::Off)
        state = StatusLightState::On;
    else if (state == StatusLightState::On)
        state = StatusLightState::Off;
    else
        state = StatusLightState::On;

    state_changed = true;
}
