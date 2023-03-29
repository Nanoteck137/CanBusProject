
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

int main()
{
    init_system();

    uint outputs[] = {16, 17, 18, PICO_DEFAULT_LED_PIN};
    int num_outputs = sizeof(outputs) / sizeof(outputs[0]);

    // Initialize the outputs
    for (int i = 0; i < num_outputs; i++)
    {
        gpio_init(outputs[i]);
        gpio_set_dir(outputs[i], GPIO_OUT);
        gpio_put(outputs[i], false);
    }

    struct InputState
    {
        bool current_value = false;
        bool current_toggle_value = false;
        bool last_value = false;
        uint64_t time = 0;
    };

    const size_t NUM_INPUTS = 6;
    uint inputs[NUM_INPUTS] = {15, 14, 13, 19, 20, 21};
    InputState input_states[NUM_INPUTS];

    for (int i = 0; i < NUM_INPUTS; i++)
    {
        gpio_init(inputs[i]);
        gpio_set_dir(inputs[i], GPIO_IN);
        gpio_set_pulls(inputs[i], true, false);
    }

    uint64_t last = time_us_64();

    bool value = false;

    while (true)
    {
        for (int i = 0; i < NUM_INPUTS; i++)
        {
            bool res = gpio_get(inputs[i]);
            printf("%d, ", res);
        }

        printf("\n");

        uint64_t current = time_us_64();
        if (current - last > 1000 * 1000)
        {
            for (int i = 0; i < num_outputs; i++)
            {
                printf("Toggle\n");
                gpio_put(outputs[i], value);
            }
            value = !value;
            last = current;
        }

        // uint64_t current = time_us_64();
        //
        // can_frame frame;
        // if (can0.readMessage(&frame) == MCP2515::ERROR_OK)
        // {
        //     if (frame.can_id == CONTROL_ID && frame.can_dlc >= 1)
        //     {
        //         uint8_t controls = frame.data[0];
        //
        //         gpio_put(outputs[0], (controls & (1 << 0)) > 0);
        //         gpio_put(outputs[1], (controls & (1 << 1)) > 0);
        //         gpio_put(outputs[2], (controls & (1 << 2)) > 0);
        //         gpio_put(outputs[3], (controls & (1 << 3)) > 0);
        //     }
        // }
        //
        // for (int i = 0; i < NUM_INPUTS; i++)
        // {
        //     InputState* state = input_states + i;
        //     state->current_value = gpio_get(inputs[i]);
        //     if (state->current_value == 0 &&
        //         state->last_value != state->current_value &&
        //         (current - state->time) > 200 * 1000)
        //     {
        //         state->current_toggle_value = !state->current_toggle_value;
        //         state->time = current;
        //     }
        //
        //     state->last_value = state->current_value;
        // }
        //
        // if (current - last > 50 * 1000)
        // {
        //     uint8_t current_lines = 0;
        //     uint8_t current_toggle_lines = 0;
        //
        //     for (int i = 0; i < NUM_INPUTS; i++)
        //     {
        //         InputState* state = input_states + i;
        //         current_lines |= ((!state->current_value) << i);
        //         current_toggle_lines |= (state->current_toggle_value << i);
        //     }
        //
        //     // printf("Sending Line Message\n");
        //     can_frame send;
        //     send.can_id = LINE_ID;
        //     send.can_dlc = 2;
        //     send.data[0] = current_lines;        // CURRENT VALUES
        //     send.data[1] = current_toggle_lines; // TOGGLED VALUES
        //     if (can0.sendMessage(&send) != MCP2515::ERROR_OK)
        //     {
        //         printf("Error?\n");
        //     }
        //     last = current;
        // }
    }
}
