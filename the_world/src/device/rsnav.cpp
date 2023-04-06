#include "util/button.h"
#include "util/status_light.h"
#include "device.h"
#include "func.h"
#include "can.h"

struct Status
{
    bool is_reverse_lights_on;
    bool is_trunk_lights_on;
    bool is_reverse_camera_on;
};

struct Context
{
    StatusLight light;

    Status status;
};

static Context context;

static void init(DeviceContext* device)
{
    context.light.init(device->controls + 0);
}

static void update(DeviceContext* device)
{
    context.light.update();
    if (context.status.is_reverse_camera_on)
    {
        context.light.blink_count(250 * 1000, 4);
    }
    else
    {
        context.light.set(context.status.is_reverse_lights_on);
    }
}

static void get_status(uint8_t* buffer)
{
    buffer[0] = (uint8_t)context.status.is_reverse_camera_on << 2 |
                (uint8_t)context.status.is_trunk_lights_on << 1 |
                (uint8_t)context.status.is_reverse_lights_on << 0;
}

static void on_can_message(uint32_t can_id, uint8_t* data, size_t len)
{
    // printf("Got CAN Message: 0x%x [", can_id);
    // for (int i = 0; i < len; i++)
    // {
    //     printf("0x%x, ", data[i]);
    // }
    // printf("]\n");

    context.status.is_reverse_lights_on = (data[0] & 0x1) > 0;
    context.status.is_reverse_camera_on = (data[0] & 0x2) > 0;
}

const DeviceSpec spec = {
    .name = "RSNav Controller",
    .version = MAKE_VERSION(0, 1, 0),

    .num_lines = 0,
    .lines = {},

    .num_controls = 1,
    .controls = {PICO_DEFAULT_LED_PIN},

    .init = init,
    .update = update,
    .get_status = get_status,
    .on_can_message = on_can_message,

    .funcs =
        {
            nullptr, // LAST
        },
};
