#include "device.h"

#include <hardware/gpio.h>

// NOTE(patrik): PhysicalLine

void PhysicalLine::init(uint32_t pin)
{
    m_pin = pin;

    gpio_init(m_pin);
    gpio_set_dir(m_pin, GPIO_IN);
    gpio_pull_up(m_pin);
}

bool PhysicalLine::get() { return !gpio_get(m_pin); }

// NOTE(patrik): PhysicalControl

void PhysicalControl::init(uint32_t pin)
{
    m_pin = pin;

    gpio_init(pin);
    gpio_set_dir(pin, GPIO_OUT);
    gpio_put(pin, false);
}

void PhysicalControl::set(bool on)
{
    m_is_on = on;
    gpio_put(m_pin, m_is_on);
}

void PhysicalControl::toggle() { set(!m_is_on); }
