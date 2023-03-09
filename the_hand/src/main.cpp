
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pico/stdio.h"
#include "tusk.h"

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
const uint32_t STATUS_ID = DEVICE_ID + 0x02;
const uint32_t MISC_ID = DEVICE_ID + 0x03;

int main()
{
    init_system();

    uint outputs[] = {15, PICO_DEFAULT_LED_PIN};
    int num_outputs = sizeof(outputs) / sizeof(outputs[0]);

    // Initialize the outputs
    for (int i = 0; i < num_outputs; i++)
    {
        gpio_init(outputs[i]);
        gpio_set_dir(outputs[i], GPIO_OUT);
        gpio_put(outputs[i], false);
    }

    while (true)
    {
        can_frame frame;
        if (can0.readMessage(&frame) == MCP2515::ERROR_OK)
        {
            printf("Got frame\n");
            if (frame.can_id == CONTROL_ID && frame.can_dlc >= 1)
            {
                uint8_t controls = frame.data[0];

                printf("Control: 0x%x\n", controls);
                gpio_put(outputs[0], (controls & (1 << 0)) > 0);
                gpio_put(outputs[1], (controls & (1 << 1)) > 0);
            }
        }
    }
}
