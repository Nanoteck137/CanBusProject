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
    uint64_t last;

    StatusLight light;

    Status status;
};

static Context context;

static void init(DeviceContext* device)
{
    context.light.init(device->controls + 0);
    context.light.blink(1000 * 1000);
}

static void update(DeviceContext* device)
{
    context.light.update();

    uint64_t current = time_us_64();
    if (current - context.last > 1000 * 1000)
    {
        // if (send_can_message(0x100, nullptr, 0))
        //     printf("Sent CAN Message: Success\n");
        // else
        //     printf("Sent CAN Message: Failed\n");

        context.last = current;
    }

    // device->controls->set(context.status.is_reverse_lights_on);
}
static void get_status(uint8_t* buffer) {}

static void on_can_message(uint32_t can_id, uint8_t* data, size_t len)
{
    // printf("Got CAN Message: 0x%x [", can_id);
    // for (int i = 0; i < len; i++)
    // {
    //     printf("0x%x, ", data[i]);
    // }
    // printf("]\n");

    uint8_t event = data[0];
    printf("Hello? %x\n", event);

    if (event == 0x01)
    {
        uint8_t which = data[1];
        uint8_t test = data[2];

        switch (which)
        {
            case 0x00: context.light.set(test); break;
            case 0x01: context.light.blink_toggle(250 * 1000); break;
        }
    }

    // context.status.is_reverse_lights_on = data[0] & 0x1;
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
