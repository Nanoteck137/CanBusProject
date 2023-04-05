#include "util/button.h"
#include "util/status_light.h"
#include "device.h"
#include "func.h"
#include "can.h"

struct Context
{
    uint64_t last;
};

static Context context;

static void init(DeviceContext* device) {}
static void update(DeviceContext* device)
{
    uint64_t current = time_us_64();
    if (current - context.last > 1000 * 1000)
    {
        if (send_can_message(0x100, nullptr, 0))
            printf("Sent CAN Message: Success\n");
        else
            printf("Sent CAN Message: Failed\n");

        context.last = current;
    }
}
static void get_status(uint8_t* buffer) {}

static void on_can_message(uint32_t can_id, uint8_t* data, size_t len)
{
    printf("Got CAN Message: 0x%x\n", can_id);
}

const DeviceSpec spec = {
    .name = "RSNav Controller",
    .version = MAKE_VERSION(0, 1, 0),

    .num_lines = 0,
    .lines = {},

    .num_controls = 0,
    .controls = {},

    .init = init,
    .update = update,
    .get_status = get_status,
    .on_can_message = on_can_message,

    .funcs =
        {
            nullptr, // LAST
        },
};
