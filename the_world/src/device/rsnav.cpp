#include "util/button.h"
#include "util/status_light.h"
#include "device.h"
#include "func.h"

struct Context
{
};

static Context context;

static void init(DeviceContext* device) {}
static void update(DeviceContext* device) {}
static void get_status(uint8_t* buffer) {}

const DeviceSpec spec = {
    .name = "RSNav Controller",
    .version = MAKE_VERSION(0, 1, 0),
    .type = DeviceType::GoldExperience,

    .num_lines = 0,
    .lines = {},

    .num_controls = 0,
    .controls = {},

    .init = init,
    .update = update,
    .get_status = get_status,

    .funcs =
        {
            nullptr, // LAST
        },
};
