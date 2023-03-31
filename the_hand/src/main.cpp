
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pico/stdio.h"
// #include "tusk.h"

#include "hardware/gpio.h"
#include "hardware/timer.h"
#include "pico/stdio/driver.h"
#include "pico/stdlib.h"

#include "mcp2515/mcp2515.h"

// MCP2515 can0;
MCP2515 can0(spi0, 5, 3, 4, 2);

void init_system()
{
    stdio_init_all();

    can0.reset();
    can0.setBitrate(CAN_125KBPS, MCP_8MHZ);
    can0.setNormalMode();
}

const uint32_t DEVICE_ID = 0x100;
const uint32_t COM_ID = DEVICE_ID + 0x00;
const uint32_t CONTROL_ID = DEVICE_ID + 0x01;
const uint32_t LINE_ID = DEVICE_ID + 0x02;
const uint32_t MISC_ID = DEVICE_ID + 0x03;

struct LineState
{
    bool current;
    bool last;
    bool state;
    bool toggle;

    uint64_t debounce_timer;
};

int main()
{
    init_system();

    uint controls[] = {16, 17, 18, PICO_DEFAULT_LED_PIN};
    int num_controls = sizeof(controls) / sizeof(controls[0]);

    // Initialize the outputs
    for (int i = 0; i < num_controls; i++)
    {
        gpio_init(controls[i]);
        gpio_set_dir(controls[i], GPIO_OUT);
        gpio_put(controls[i], false);
    }

    const size_t NUM_LINES = 3;
    uint lines[NUM_LINES] = {19, 20, 21};
    LineState line_states[NUM_LINES] = {};

    for (int i = 0; i < NUM_LINES; i++)
    {
        gpio_init(lines[i]);
        gpio_set_dir(lines[i], GPIO_IN);
        gpio_set_pulls(lines[i], true, false);
    }

    uint64_t last_time = time_us_64();

    uint32_t index = 0;

    while (true)
    {
        for (int i = 0; i < NUM_LINES; i++)
        {
            LineState* state = line_states + i;
            bool current = !gpio_get(lines[i]);
            current = !gpio_get(lines[i]);
            state->current = current;
        }

        uint64_t current_time = time_us_64();

        for (int i = 0; i < NUM_LINES; i++)
        {
            LineState* line = line_states + i;
            if (line->current != line->last)
                line->debounce_timer = current_time;

            if (current_time - line->debounce_timer > 50 * 1000)
            {
                if (line->current != line->state)
                {
                    line->state = line->current;

                    if (line->state)
                    {
                        line->toggle = !line->toggle;
                    }
                }
            }
        }

        if (current_time - last_time > 200 * 1000)
        {
            for (int i = 0; i < NUM_LINES; i++)
            {
                LineState* state = line_states + i;
                printf("Line %d: C: %d L: %d\n", i, state->state,
                       state->toggle);
            }
            printf("\n");

            last_time = current_time;
        }

        for (int i = 0; i < NUM_LINES; i++)
        {
            LineState* state = line_states + i;
            state->last = state->current;
        }
    }
}
