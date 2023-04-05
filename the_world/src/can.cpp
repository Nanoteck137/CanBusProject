#include "can.h"

#include <string.h>
#include "device.h"

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

void can_update()
{
    can_frame frame;
    if (can0.readMessage(&frame) == MCP2515::ERROR_OK)
        spec.on_can_message(frame.can_id, frame.data, frame.can_dlc);
}

bool send_can_message(uint32_t can_id, uint8_t* data, size_t len)
{
    // TODO(patrik): Check len

    can_frame frame;
    frame.can_id = can_id;
    frame.can_dlc = len;
    if (data && len > 0)
        memcpy(frame.data, data, len);

    MCP2515::ERROR err = can0.sendMessage(&frame);
    return err == MCP2515::ERROR_OK;
}
