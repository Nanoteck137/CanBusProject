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
    //  id + 0x01 - Control
    //  id + 0x02 - Status
    //  id + 0x03 - Misc

    while (true)
    {
        can_frame frame;
        if (can0.readMessage(&frame) == MCP2515::ERROR_OK)
        {
            for (int i = 0; i < NUM_DEVICES; i++)
            {
                DeviceData* data = &device_data[i];
                Device* device = &data->device;

                // TODO(patrik): If we get a status message we need to save it

                if (frame.can_id == device->status_id() && frame.can_dlc == 5)
                {
                    // NOTE(patrik): Format
                    // Byte 0 - Control Status
                    // Byte 1 - Line Status
                    // Byte 2 - Extra Line Status
                    // Byte 3 - Toggled Line Status
                    // Byte 4 - Toggled Extra Line Status

                    uint8_t control_status = frame.data[0];

                    uint8_t line_status = frame.data[1];
                    uint8_t extra_line_status = frame.data[2];

                    uint8_t toggled_line_status = frame.data[3];
                    uint8_t toggled_extra_line_status = frame.data[4];

                    data->control_status = control_status;

                    data->line_status = line_status;
                    data->extra_line_status = extra_line_status;

                    data->toggled_line_status = toggled_line_status;
                    data->toggled_extra_line_status = toggled_extra_line_status;

                    // TODO(patrik): Here we need to update our controls if we
                    // have a desire to do that

                    printf("Status Update (0x%x): %x %x %x %x %x",
                           device->can_id, data->control_status,
                           data->line_status, data->extra_line_status,
                           data->toggled_line_status,
                           data->toggled_extra_line_status);
                }
            }
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
