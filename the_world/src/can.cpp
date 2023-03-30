#include "can.h"

#include <FreeRTOS.h>
#include <task.h>

#include <mcp2515/mcp2515.h>

MCP2515 can0(spi0, 5, 3, 4, 2);

void can_init()
{
    can0.reset();
    can0.setBitrate(CAN_125KBPS, MCP_8MHZ);
    can0.setNormalMode();
}

void can_thread(void* ptr)
{
    // NOTE(patrik):
    // Device On the Can Bus Network
    // Every devices gets 4 ids allocated to them
    // ID Allocation:
    //  id + 0x00 - Communication ID
    //  id + 0x01 - Outputs (Control Relays)
    //  id + 0x02 - Inputs (State of the inputs, Send to main unit)
    //  id + 0x03 - Misc

    while (true)
    {
        can_frame frame;
        if (can0.readMessage(&frame) == MCP2515::ERROR_OK)
        {
            // for (int i = 0; i < NUM_DEVICES; i++)
            // {
            //     Device* device = devices + i;
            //     if (frame.can_id == device->line_id() && frame.can_dlc == 2)
            //     {
            //         uint8_t lines = frame.data[0];
            //         uint8_t toggled_lines = frame.data[1];
            //         device->lines = lines;
            //         device->toggled_lines = toggled_lines;
            //         printf("Wot: %d %d\n", lines, toggled_lines);
            //     }
            // }
        }

        // for (int i = 0; i < NUM_DEVICES; i++)
        // {
        //     Device* device = devices + i;
        //     if (device->need_update)
        //     {
        //         can_frame send;
        //         send.can_id = device->control_id();
        //         send.can_dlc = 1;
        //         send.data[0] = device->controls;
        //         can0.sendMessage(&send);
        //
        //         device->need_update = false;
        //     }
        // }

        vTaskDelay(1);
    }
}
